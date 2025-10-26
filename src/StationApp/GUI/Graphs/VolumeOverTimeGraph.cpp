#include "VolumeOverTimeGraph.h"
#include "StationApp/GUI/AudioConstants.h"
#include "StationApp/GUI/Graphs/SamplePositionUtils.h"
#include "StationApp/OpenGL/OpenGlShaders.h"
#include "StationApp/OpenGL/ShaderHelpers.h"
#include "juce_opengl/opengl/juce_gl.h"
#include "spdlog/spdlog.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

VolumeOverTimeGraph::VolumeOverTimeGraph(TrackInfoStore &tis) : BaseOverTimeGraph(tis)
{
    shouldClear = false;
    for (size_t i = 0; i < MAX_NUM_TILES; i++)
    {
        secondTilesRingBuffer[i] = std::make_shared<SecondTile>();
        freeSecondTilesIndexes.push(i);
    }
}

VolumeOverTimeGraph::~VolumeOverTimeGraph()
{
}

void VolumeOverTimeGraph::clearTracksFromRange(int64_t startSample, int64_t length)
{
    if (length < 1)
    {
        throw std::runtime_error("VolumeOverTimeGraph::clearTracksFromRange: length < 1");
    }

    int64_t endSample = startSample + (length - 1);

    int64_t secondTileIndexStartSample = startSample / VISUAL_SAMPLE_RATE;
    int64_t secondTileIndexEndSample = endSample / VISUAL_SAMPLE_RATE;

    for (int64_t j = secondTileIndexStartSample; j <= secondTileIndexEndSample; j++)
    {
        if (j < 0)
        {
            continue;
        }
        queueTileRemoval(j);
    }
}

void VolumeOverTimeGraph::clear()
{
    shouldClear = true;
}

void VolumeOverTimeGraph::queueTileRemoval(int64_t tileIndex)
{
    std::lock_guard lock(tileRemovalQueueMutex);

    if (tilesToRemoveSet.find(tileIndex) != tilesToRemoveSet.end())
    {
        return;
    }

    if (tileRemovalQueue.size() > MAX_QUEUED_TILE_REMOVAL)
    {
        spdlog::warn("VolumeOverTimeGraph::queueTileRemoval: tileRemovalQueue.size() > MAX_QUEUED_TILE_REMOVAL");
        tileRemovalQueue.pop();
    }
    tileRemovalQueue.push(tileIndex);
    tilesToRemoveSet.insert(tileIndex);
}

void VolumeOverTimeGraph::setTrackColor(uint64_t trackIdentifier, juce::Colour col)
{
    // TODO: Implement track color setting functionality
}

void VolumeOverTimeGraph::setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm)
{
    // TODO: Implement track selection functionality
}

void VolumeOverTimeGraph::displayNewVolumeData(std::shared_ptr<NewTrackVolumeDataTask> volumeData)
{
    if (volumeData->segmentSampleLength < 1)
    {
        throw std::runtime_error("VolumeOverTimeGraph::displayNewVolumeData: segmentSampleLength < 1");
    }

    int64_t sectionSampleWidth = (int64_t)volumeData->segmentSampleLength;
    int64_t startSample = volumeData->segmentStartSample;
    int64_t endSample = startSample + (sectionSampleWidth - 1);

    SamplePositionUtils::toVisualSampleRate(startSample, endSample, (int64_t)volumeData->sampleRate);

    int64_t secondTileIndexStartSample = startSample / VISUAL_SAMPLE_RATE;
    int64_t secondTileIndexEndSample = endSample / VISUAL_SAMPLE_RATE;

    // fill the tile (one or two if overlaps) with the fft data
    for (int64_t j = secondTileIndexStartSample; j <= secondTileIndexEndSample; j++)
    {
        if (j < 0)
        {
            continue;
        }
        // if it's the first tile, the start sample is the modulo of the startSample
        // otherwise it's the tile start
        int64_t tileStartSample = 0;
        if (j == secondTileIndexStartSample)
        {
            tileStartSample = startSample % VISUAL_SAMPLE_RATE;
        }
        // same reasoning for the end sample within the tile
        int64_t tileEndSample = VISUAL_SAMPLE_RATE - 1;
        if (j == secondTileIndexEndSample)
        {
            tileEndSample = endSample % VISUAL_SAMPLE_RATE;
        }
        // we can now write the fft into the tile (and eventually create it)
        int channelIndex = 2;                 // 0 for left, 1 for right, 2 for both
        if (volumeData->totalNoChannels == 2) // if there are two channel (not mono), this is for one specific
        {
            if (volumeData->channelIndex == 0)
            {
                channelIndex = 0;
            }
            else
            {
                channelIndex = 1;
            }
        }
        queueVolumeForDrawing(volumeData->trackIdentifier, j, tileStartSample, tileEndSample, volumeData->volume,
                              channelIndex);
    }
}

void VolumeOverTimeGraph::queueVolumeForDrawing(uint64_t trackIdentifier, int64_t secondTileIndex,
                                                int64_t tileStartSample, int64_t tileEndSample, float volume,
                                                int channelIndex)
{
    TrackVolumeData volumeData;
    volumeData.trackIdentifier = trackIdentifier;
    volumeData.secondTileIndex = secondTileIndex;
    volumeData.tileStartSample = tileStartSample;
    volumeData.tileEndSample = tileEndSample;
    volumeData.volumeLevel = volume;
    volumeData.channelIdentifier = channelIndex;

    std::lock_guard<std::mutex> lock(volumeUpdateQueueMutex);
    if (volumeUpdateQueue.size() > MAX_QUEUED_VOLUME_DRAW)
    {
        spdlog::warn("VolumeOverTimeGraph::queueVolumeForDrawing: volumeUpdateQueue.size() > MAX_QUEUED_VOLUME_DRAW");
        volumeUpdateQueue.pop();
    }
    volumeUpdateQueue.push(volumeData);
}

void VolumeOverTimeGraph::resetJuceOpenGLShaders(juce::OpenGLContext &openGLContext)
{
    texturedPositionedShader.reset(new juce::OpenGLShaderProgram(openGLContext));
}

void VolumeOverTimeGraph::setShadersUniformsAtOpenGlInit()
{
    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("volumeBarsTexture", 0);
}

void VolumeOverTimeGraph::loadGlObjectsAtInit()
{
    // load tiles textures
    texturedPositionedShader->use();
    for (size_t i = 0; i < MAX_NUM_TILES; i++)
    {
        secondTilesRingBuffer[i]->mesh->registerGlObjects();
    }
    spdlog::debug("Tiles textures loaded");
}

bool VolumeOverTimeGraph::buildShadersAtInit()
{
    bool builtTexturedShader =
        ShaderHelpers::buildShader(texturedPositionedShader, volumesVertexShader, volumesFragmentShader);
    if (!builtTexturedShader)
    {
        spdlog::error("Failed to build textured positioned shaders");
        return false;
    }
    spdlog::info("Built FFt customn shader");
    return builtTexturedShader;
}

void VolumeOverTimeGraph::uploadAdditionalShadersUniforms()
{
    float zoomFactor = (float)(TILE_PIXEL_HEIGHT >> 1) / (float)getMaxPixelDistDrawnInView();

    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("viewPosition", (GLfloat)getViewPositionLockFree());
    texturedPositionedShader->setUniform("viewWidth", (GLfloat)(getViewWidthLockFree() * getViewScaleLockFree()));
    texturedPositionedShader->setUniform("convolutionId", (GLint)0);
    texturedPositionedShader->setUniform("zoomFactor", (GLfloat)zoomFactor);
}

int64_t VolumeOverTimeGraph::getMaxPixelDistDrawnInView()
{
    int64_t sectionSampleWidth = getViewWidthLockFree() * getViewScaleLockFree();
    int64_t startSample = getViewPositionLockFree();
    int64_t endSample = startSample + (sectionSampleWidth - 1);

    int64_t secondTileIndexStartSample = startSample / VISUAL_SAMPLE_RATE;
    int64_t secondTileIndexEndSample = endSample / VISUAL_SAMPLE_RATE;

    int64_t maxDistFromCenter = 0;

    for (int64_t j = secondTileIndexStartSample; j <= secondTileIndexEndSample; j++)
    {
        if (j < 0)
        {
            continue;
        }

        auto existingTile = secondTilesIndexMap.find(j);
        if (existingTile != secondTilesIndexMap.end())
        {
            maxDistFromCenter = std::max(maxDistFromCenter, secondTilesRingBuffer[existingTile->second]->maxDrawnPixel);
        }
    }

    if (maxDistFromCenter == 0)
    {
        return TILE_PIXEL_HEIGHT >> 1;
    }
    else
    {
        return maxDistFromCenter;
    }
}

void VolumeOverTimeGraph::deleteTilesQueuedForDeletion()
{
    {
        std::lock_guard lock(tileRemovalQueueMutex);
        std::swap(tileRemovalQueue, tileRemovalReadQueue);
        std::swap(tilesToRemoveSet, tilesToRemoveReadSet);
    }
    if (shouldClear)
    {
        shouldClear = false;
        for (const auto &entry : secondTilesIndexMap)
        {
            size_t tileIndex = entry.second;
            secondTilesRingBuffer[tileIndex]->tileIndexPosition = -1;
            freeSecondTilesIndexes.push(tileIndex);
        }
        secondTilesIndexMap.clear();

        while (!tileRemovalReadQueue.empty())
        {
            tileRemovalReadQueue.pop();
        }
        tilesToRemoveReadSet.clear();
    }
    else
    {
        while (tileRemovalReadQueue.size() > 0)
        {
            int64_t tileIndex = tileRemovalReadQueue.front();
            tileRemovalReadQueue.pop();
            tilesToRemoveReadSet.erase(tileIndex);

            auto tileIdentifier = secondTilesIndexMap.find(tileIndex);
            if (tileIdentifier != secondTilesIndexMap.end())
            {
                secondTilesIndexMap.erase(tileIdentifier);
                secondTilesRingBuffer[tileIdentifier->second]->tileIndexPosition = -1;
                freeSecondTilesIndexes.push(tileIdentifier->second);
            }
        }
    }
}

void VolumeOverTimeGraph::drawQueuedVolumesToTextures()
{
    // TODO: do not drain the queue at every refresh to group
    // bar redrawings together and reduce overlapping work
    {
        std::lock_guard lock(volumeUpdateQueueMutex);
        std::swap(volumeUpdateQueue, volumeUpdateReadQueue);
    }
    while (volumeUpdateReadQueue.size() > 0)
    {
        TrackVolumeData volumeData = volumeUpdateReadQueue.front();
        volumeUpdateReadQueue.pop();
        size_t tileIndex = getOrCreateSecondTile(volumeData.secondTileIndex);
        addVolumeOnTile(tileIndex, volumeData);
    }
    drawUpdatedTileBars();
}

size_t VolumeOverTimeGraph::getOrCreateSecondTile(int64_t secondTileIndex)
{
    auto existingTile = secondTilesIndexMap.find(secondTileIndex);
    if (existingTile != secondTilesIndexMap.end())
    {
        return existingTile->second;
    }

    size_t freeIndex = 0;
    if (freeSecondTilesIndexes.size() > 0)
    {
        freeIndex = freeSecondTilesIndexes.front();
        freeSecondTilesIndexes.pop();
    }
    else
    {
        spdlog::error("VolumeOverTimeGraph::getOrCreateSecondTile: ran out of free SecondTile");
        throw std::runtime_error("VolumeOverTimeGraph::getOrCreateSecondTile: ran out of free SecondTile");
    }

    secondTilesRingBuffer[freeIndex]->tileIndexPosition = secondTileIndex;
    secondTilesRingBuffer[freeIndex]->maxDrawnPixel = 0;
    secondTilesRingBuffer[freeIndex]->trackVolumes.clear();
    secondTilesRingBuffer[freeIndex]->mesh->setPosition(secondTileIndex * VISUAL_SAMPLE_RATE);
    secondTilesRingBuffer[freeIndex]->mesh->clearAllData();
    secondTilesIndexMap[secondTileIndex] = freeIndex;

    return freeIndex;
}

void VolumeOverTimeGraph::addVolumeOnTile(size_t tileIndex, TrackVolumeData &volData)
{
    // for each volume bar
    size_t firstBarIndex = (size_t)(volData.tileStartSample / TILE_BAR_SAMPLE_WIDTH);
    size_t lastBarIndex = (size_t)(volData.tileEndSample / TILE_BAR_SAMPLE_WIDTH);

    SecondTile &tile = *secondTilesRingBuffer[tileIndex];

    float volumeToSet = volData.volumeLevel;
    if (volumeToSet > MAX_SHOWABLE_RMS_VOLUME)
    {
        volumeToSet = MAX_SHOWABLE_RMS_VOLUME;
    }

    int chan = volData.channelIdentifier; // 0 for left, 1 for right, 2 for both

    // get or create trackIdentifier within tile
    auto existingTrackData = tile.trackVolumes.find(volData.trackIdentifier);
    if (existingTrackData == tile.trackVolumes.end())
    {
        tile.trackVolumes[volData.trackIdentifier] = std::array<float, BARS_PER_TILE * 2>();
        auto &trackData = tile.trackVolumes[volData.trackIdentifier];
        trackData.fill(0.0f);
    }

    auto &trackData = tile.trackVolumes.find(volData.trackIdentifier)->second;

    if (chan == 0 || chan == 2)
    {
        for (size_t i = firstBarIndex; i <= lastBarIndex; i++)
        {
            trackData[i] = volumeToSet;
        }
    }

    if (chan == 1 || chan == 2)
    {
        for (size_t i = firstBarIndex; i <= lastBarIndex; i++)
        {
            trackData[BARS_PER_TILE + i] = volumeToSet;
        }
    }
    secondTilesToDraw.insert(tileIndex);
}

void VolumeOverTimeGraph::drawUpdatedTileBars()
{
    // These arrays keep track of the top value of each stacked bar.
    // Each top or bottom side have TILE_PIXEL_HEIGHT/2 pixels
    // to draw the bars, and this keep track of the pixel height away
    // from the center that the last track volume bar peaked at.
    // So if bar 3 had 2 stacked-track-bars drawn already of height 10 and 20
    // in pixels, then lastStackedValueTop[2] = 30, and the next drawn
    // stacked bar for bar 3 will start being drawn 30 pixels away fron center.

    std::array<int, BARS_PER_TILE> lastStackedValueTop;
    std::array<int, BARS_PER_TILE> lastStackedValueBottom;

    // We use a system of coordinates where the (0, 0) is in the (left, upper) corner.

    for (const auto &tileIndex : secondTilesToDraw)
    {
        SecondTile &tile = *secondTilesRingBuffer[tileIndex];
        tile.mesh->clearAllData();

        lastStackedValueTop.fill(0);
        lastStackedValueBottom.fill(0);

        tile.maxDrawnPixel = 0;

        for (const auto &trackData : tile.trackVolumes)
        {

            // TODO: study the chances that waiting on the trackcolor lock might
            // create a lock chain that will ultimately wait for the openGL thread
            // and create a deadlock.
            auto col = trackInfoStore.getTrackColor(trackData.first);
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
            if (col.has_value())
            {
                r = ((float)(col->red) / 255.0f);
                g = ((float)(col->green) / 255.0f);
                b = ((float)(col->blue) / 255.0f);
            }

            for (size_t i = 0; i < BARS_PER_TILE; i++)
            {
                int barX = i * TILE_BAR_PIXEL_WIDTH;
                int barWidth = TILE_BAR_PIXEL_WIDTH;

                // left channel (drawn in top half)
                if (trackData.second[i] > MIN_SHOWABLE_RMS_VOLUME)
                {
                    int barPixelHeight = (int)((float)(MAX_SECOND_TILE_TRACK_PIXEL_SIZE) *
                                               (trackData.second[i] / MAX_SHOWABLE_RMS_VOLUME));
                    if (barPixelHeight > 0)
                    {
                        int barYStart = (TILE_PIXEL_HEIGHT / 2) - lastStackedValueTop[i];
                        if (barYStart < 0)
                        {
                            // we skip this track bar if it goes over drawable limits
                            continue;
                        }
                        int barYStop = barYStart - (barPixelHeight - 1);
                        if (barYStop < 0)
                        {
                            // we skip this track bar if it goes over drawable limits
                            continue;
                        }
                        lastStackedValueTop[i] += barPixelHeight;
                        tile.mesh->setRectangle(barX, barYStop, barWidth, barPixelHeight, r, g, b, a);
                    }
                }

                if (trackData.second[BARS_PER_TILE + i] > MIN_SHOWABLE_RMS_VOLUME)
                {
                    int barPixelHeight = (int)((float)(MAX_SECOND_TILE_TRACK_PIXEL_SIZE) *
                                               (trackData.second[BARS_PER_TILE + i] / MAX_SHOWABLE_RMS_VOLUME));
                    if (barPixelHeight > 0)
                    {
                        int barYStart = (TILE_PIXEL_HEIGHT / 2) + lastStackedValueBottom[i];
                        if (barYStart >= TILE_PIXEL_HEIGHT)
                        {
                            // we skip this track bar if it goes over drawable limits
                            continue;
                        }
                        int barYStop = barYStart + barPixelHeight - 1;
                        if (barYStop >= TILE_PIXEL_HEIGHT)
                        {
                            // we skip this track bar if it goes over drawable limits
                            continue;
                        }
                        lastStackedValueBottom[i] += barPixelHeight;
                        tile.mesh->setRectangle(barX, barYStart, barWidth, barPixelHeight, r, g, b, a);
                    }
                }

                // TODO: keep track of the tile"s highest pixel from center
            }
        }

        for (size_t i = 0; i < BARS_PER_TILE; i++)
        {
            int64_t maxStackedValue = std::max(lastStackedValueTop[i], lastStackedValueBottom[i]);
            tile.maxDrawnPixel = std::max(tile.maxDrawnPixel, maxStackedValue);
        }
    }
    secondTilesToDraw.clear();
}

void VolumeOverTimeGraph::applyQueuedColorUpdates()
{
    // TODO: Implement color updates
}

void VolumeOverTimeGraph::eventuallyRefreshGPUTextures()
{
    if (glIterCount % 5 == 0)
    {
        for (const auto &entry : secondTilesIndexMap)
        {
            size_t tileIndex = entry.second;
            secondTilesRingBuffer[tileIndex]->mesh->refreshGpuTextureIfChanged();
        }
    }
    glIterCount++;
}

void VolumeOverTimeGraph::drawGlMeshes()
{
    texturedPositionedShader->use();

    for (const auto &entry : secondTilesIndexMap)
    {
        size_t tileIndex = entry.second;
        if (secondTilesRingBuffer[tileIndex]->tileIndexPosition >= 0)
        {
            secondTilesRingBuffer[tileIndex]->mesh->drawGlObjects();
        }
    }
}

void VolumeOverTimeGraph::glLoopPreDraw()
{
    deleteTilesQueuedForDeletion();
    drawQueuedVolumesToTextures();
    applyQueuedColorUpdates();
    eventuallyRefreshGPUTextures();
}

void VolumeOverTimeGraph::glLoopDrawOverGrid()
{
    drawGlMeshes();
}

void VolumeOverTimeGraph::deallocateOpenGlResources()
{
    for (size_t i = 0; i < MAX_NUM_TILES; i++)
    {
        secondTilesRingBuffer[i]->mesh->freeGlObjects();
    }
    texturedPositionedShader->release();
}