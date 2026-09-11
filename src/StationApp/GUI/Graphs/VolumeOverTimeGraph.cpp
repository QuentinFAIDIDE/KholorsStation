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
#include <optional>

VolumeOverTimeGraph::VolumeOverTimeGraph(TrackInfoStore &tis) : BaseOverTimeGraph(tis)
{
    lastZoomFactorUpdateTimeMs = 0;
    lastClearTimeMs = juce::Time::currentTimeMillis();
    lastZoomFactor = 1.0;
    shouldClear = false;
    shouldRedrawTiles = false;
    for (size_t i = 0; i < DEFAULT_NUM_TILES; i++)
    {
        secondTilesRingBuffer.push_back(std::make_shared<SecondTile>());
        freeSecondTilesIndexes.push(i);
    }
}

VolumeOverTimeGraph::~VolumeOverTimeGraph()
{
}

void VolumeOverTimeGraph::clearTrackFromRange(uint64_t trackIdentifier, int64_t startSample, int64_t length)
{
    if (length < 1)
    {
        throw std::runtime_error("VolumeOverTimeGraph::clearTrackFromRange: length < 1");
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
        queueTileRemoval(trackIdentifier, j);
    }
}

void VolumeOverTimeGraph::clear()
{
    shouldClear = true;
}

void VolumeOverTimeGraph::queueTileRemoval(uint64_t trackIdentifier, int64_t tileIndex)
{
    std::lock_guard lock(tileRemovalQueueMutex);

    if (tileRemovalQueue.size() > MAX_QUEUED_TILE_REMOVAL)
    {
        spdlog::warn("VolumeOverTimeGraph::queueTileRemoval: tileRemovalQueue.size() > MAX_QUEUED_TILE_REMOVAL");
        tileRemovalQueue.pop();
    }
    tileRemovalQueue.push({trackIdentifier, tileIndex});
}

void VolumeOverTimeGraph::setTrackColor(uint64_t trackIdentifier, juce::Colour col)
{
    // PERF: It would be good to cache color and update our local cache. This way
    // we rely less on foreign track info object that takes a lock.
    // In that case we would update the cache here.
    shouldRedrawTiles = true;
}

void VolumeOverTimeGraph::setSelectedTrack(std::optional<uint64_t> trackToSelect, TaskingManager *tm)
{
    {
        std::lock_guard lock(selectedTrackMutex);
        bool existenceDiffers = trackToSelect.has_value() != selectedTrack.has_value();
        if (existenceDiffers ||
            (!existenceDiffers && trackToSelect.has_value() && trackToSelect.value() != selectedTrack.value()))
        {
            selectedTrack = trackToSelect;
            shouldRedrawTiles = true;
        }
    }
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
    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
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

    float zoomFactor;
    if (glIterCount % 5 == 0)
    {
        int64_t currentTime = juce::Time::currentTimeMillis();
        if (currentTime - lastZoomFactorUpdateTimeMs > ZOOM_FACTOR_UPDATE_INTERVAL_MS)
        {
            float desiredZoomFactor = (float)(TILE_PIXEL_HEIGHT >> 1) / (float)getMaxPixelDistDrawnInView();
            float dist = desiredZoomFactor - lastZoomFactor;
            float distSign = dist < 0 ? -1 : 1;
            float increment;
            if (distSign < 0)
            {
                // we reduce zooming
                increment = -(std::min(std::abs(dist), ZOOM_FACTOR_UPDATE_INCREMENT * ZOOMOUT_BOOST_FACTOR));
            }
            else
            {
                // we increase zooming
                if ((currentTime - lastClearTimeMs) < RECENT_CLEAR_ZOOMIN_BOOST_TIME_MS)
                {
                    increment = +(std::min(std::abs(dist), ZOOM_FACTOR_UPDATE_INCREMENT * ZOOMOUT_BOOST_FACTOR));
                }
                else
                {
                    increment = +(std::min(std::abs(dist), ZOOM_FACTOR_UPDATE_INCREMENT));
                }
            }

            zoomFactor = lastZoomFactor + increment;

            lastZoomFactorUpdateTimeMs = currentTime;
            lastZoomFactor = zoomFactor;
        }
        else
        {
            zoomFactor = lastZoomFactor;
        }
    }
    else
    {
        zoomFactor = lastZoomFactor;
    }

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
        lastClearTimeMs = juce::Time::currentTimeMillis();
    }
    else
    {
        while (tileRemovalReadQueue.size() > 0)
        {
            TrackTileRemoval removal = tileRemovalReadQueue.front();
            tileRemovalReadQueue.pop();

            auto tileIdentifier = secondTilesIndexMap.find(removal.tileIndex);
            if (tileIdentifier != secondTilesIndexMap.end())
            {
                size_t ringBufferIndex = tileIdentifier->second;
                SecondTile &tile = *secondTilesRingBuffer[ringBufferIndex];
                if (tile.trackVolumes.erase(removal.trackIdentifier) > 0)
                {
                    if (tile.trackVolumes.empty())
                    {
                        secondTilesIndexMap.erase(tileIdentifier);
                        tile.tileIndexPosition = -1;
                        secondTilesToDraw.erase(ringBufferIndex);
                        freeSecondTilesIndexes.push(ringBufferIndex);
                    }
                    else
                    {
                        secondTilesToDraw.insert(ringBufferIndex);
                    }
                }
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
        if (secondTilesRingBuffer.size() <= MAX_NUM_TILES)
        {
            secondTilesRingBuffer.push_back(std::make_shared<SecondTile>());
            freeIndex = secondTilesRingBuffer.size() - 1;
            secondTilesRingBuffer[freeIndex]->mesh->registerGlObjects();
        }
        else
        {
            spdlog::error("VolumeOverTimeGraph::getOrCreateSecondTile: ran out of free SecondTile");
            throw std::runtime_error("VolumeOverTimeGraph::getOrCreateSecondTile: ran out of free SecondTile");
        }
    }

    secondTilesRingBuffer[freeIndex]->tileIndexPosition = secondTileIndex;
    secondTilesRingBuffer[freeIndex]->maxDrawnPixel = 0;
    secondTilesRingBuffer[freeIndex]->trackVolumes.clear();
    secondTilesRingBuffer[freeIndex]->mesh->setPosition(secondTileIndex * VISUAL_SAMPLE_RATE);
    secondTilesRingBuffer[freeIndex]->mesh->clearAllData();
    secondTilesRingBuffer[freeIndex]->mesh->refreshGpuTextureIfChanged();
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

    std::optional<uint64_t> localSelectedTrack;
    {
        std::lock_guard lock(selectedTrackMutex);
        localSelectedTrack = selectedTrack;
    }

    // We use a system of coordinates where the (0, 0) is in the (left, upper) corner.

    for (const auto &tileIndex : secondTilesToDraw)
    {
        SecondTile &tile = *secondTilesRingBuffer[tileIndex];

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
            if (localSelectedTrack.has_value() && localSelectedTrack.value() != trackData.first)
            {
                a = 0.2;
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
            }
        }

        for (size_t i = 0; i < BARS_PER_TILE; i++)
        {
            // count the max
            int64_t maxStackedValue = std::max(lastStackedValueTop[i], lastStackedValueBottom[i]);
            tile.maxDrawnPixel = std::max(tile.maxDrawnPixel, maxStackedValue);

            // overwrite the remaining space on top of the bars
            int barX = i * TILE_BAR_PIXEL_WIDTH;
            int barWidth = TILE_BAR_PIXEL_WIDTH;
            // draw the top alpha region
            int barHeight = (TILE_PIXEL_HEIGHT / 2) - lastStackedValueTop[i];
            if (barHeight > 0)
            {
                tile.mesh->setRectangle(barX, 0, barWidth, barHeight, 0, 0, 0, 0);
            }
            // draw the bottom alpha region
            int barYStart = (TILE_PIXEL_HEIGHT / 2) + lastStackedValueBottom[i];
            barHeight = TILE_PIXEL_HEIGHT - barYStart;
            if (barHeight > 0)
            {
                tile.mesh->setRectangle(barX, barYStart, barWidth, barHeight, 0, 0, 0, 0);
            }
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
    // PERF: avoid markUniformsAsStale at every frame. We currently do it
    // because when the view does not scroll, the desired
    // zooming is not uploaded due to glUniformNonce not being
    // updated when this happens (it is when dashboard obj updates viewPosition and such).
    // The way to go would be to factor out the ideal zoom level
    // computation into this main opengl loop, and only call markUniformsAsStale
    // if the actual value needs adjustment.
    // Although as view constantly scrolls and these uniforms are just very
    // little data, I decided to leave that for later.
    markUniformsAsStale();

    // This is triggered when a color update was received for a track.
    // In that case we redraw all tiles as we pick colors from the color store.
    if (shouldRedrawTiles)
    {
        shouldRedrawTiles = false;
        for (const auto &entry : secondTilesIndexMap)
        {
            secondTilesToDraw.insert(entry.second);
        }
    }

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
    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
    {
        secondTilesRingBuffer[i]->mesh->freeGlObjects();
    }
    texturedPositionedShader->release();
}
