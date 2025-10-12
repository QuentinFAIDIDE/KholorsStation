#include "VolumeOverTimeGraph.h"
#include "StationApp/GUI/AudioConstants.h"
#include "StationApp/GUI/Graphs/SamplePositionUtils.h"
#include "StationApp/GUI/TrackList.h"
#include "spdlog/spdlog.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

VolumeOverTimeGraph::VolumeOverTimeGraph(TrackInfoStore &tis) : BaseOverTimeGraph(tis)
{
    for (size_t i = 0; i < MAX_NUM_TILES; i++)
    {
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
    // TODO: stack the tiles into a queue of volumes to draw
    // Q: do we get the colour from here and push to the thread, or we get the color from the openGL thread ?
    // Q: We need to keep track of which trackid is shown at each position and redraw all stacked data each time a
    // datum arrives NOTE: We need to use second-tiles just like the fft worker, but with a maximum summed volume
    // heights, and we zoom in or out based on which maximum is on screen NOTE: In a first iteration, we will not
    // zoom in or out

    if (volumeData->segmentSampleLength < 1)
    {
        throw std::runtime_error("VolumeOverTimeGraph::displayNewVolumeData: segmentSampleLength < 1");
    }

    int64_t sectionSampleWidth = (int64_t)volumeData->segmentSampleLength;
    int64_t startSample = volumeData->segmentStartSample;
    int64_t endSample = startSample + (sectionSampleWidth - 1);

    SamplePositionUtils::toVisualSampleRate(startSample, endSample, (int64_t)volumeData->sampleRate);
    SamplePositionUtils::shiftToAlignWithOrigin(startSample, endSample);

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

    spdlog::info("Volume Recv: {}", volume);

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
    /*
    texturedPositionedShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    */
}

void VolumeOverTimeGraph::setShadersUniformsAtOpenGlInit()
{
    /*
    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("sfftTexture", 0);
    */
}

void VolumeOverTimeGraph::loadGlObjectsAtInit()
{
    /*
    // load tiles textures
    texturedPositionedShader->use();
    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
    {
        if (secondTilesRingBuffer[i].tileIndexPosition >= 0)
        {
            secondTilesRingBuffer[i].mesh->registerGlObjects();
        }
    }
    spdlog::debug("Tiles textures loaded");
    */
}

bool VolumeOverTimeGraph::buildShadersAtInit()
{
    /*
    bool builtTexturedShader = ShaderHelpers::buildShader(texturedPositionedShader, fftVertexShader, fftFragmentShader);
    if (!builtTexturedShader)
    {
        spdlog::error("Failed to build textured positioned shaders");
        return false;
    }
    spdlog::info("Built FFt customn shader");
    return builtTexturedShader;
    */
}

void VolumeOverTimeGraph::uploadAdditionalShadersUniforms()
{
    /*
    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("viewPosition", (GLfloat)getViewPositionLockFree());
    texturedPositionedShader->setUniform("viewWidth", (GLfloat)(getViewWidthLockFree() * getViewScaleLockFree()));
    texturedPositionedShader->setUniform("convolutionId", (GLint)convolutionId);
    */
}

void VolumeOverTimeGraph::deleteTilesQueuedForDeletion()
{
    {
        std::lock_guard lock(tileRemovalQueueMutex);
        std::swap(tileRemovalQueue, tileRemovalReadQueue);
        std::swap(tilesToRemoveSet, tilesToRemoveReadSet);
    }
    while (tileRemovalReadQueue.size() > 0)
    {
        int64_t tileIndex = tileRemovalReadQueue.front();
        tileRemovalReadQueue.pop();
        tilesToRemoveReadSet.erase(tileIndex);

        auto tileIdentifier = secondTilesIndexMap.find(tileIndex);
        if (tileIdentifier != secondTilesIndexMap.end())
        {
            secondTilesIndexMap.erase(tileIdentifier);
            secondTilesRingBuffer[tileIdentifier->second].tileIndexPosition = -1;
            freeSecondTilesIndexes.push(tileIdentifier->second);
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
    }

    secondTilesRingBuffer[freeIndex].tileIndexPosition = secondTileIndex;
    secondTilesRingBuffer[freeIndex].maxHeightRatio = 1.0f;
    secondTilesRingBuffer[freeIndex].trackVolumes.clear();
    secondTilesRingBuffer[freeIndex].mesh->clearAllData();
    secondTilesIndexMap[secondTileIndex] = freeIndex;

    return freeIndex;
}

void VolumeOverTimeGraph::addVolumeOnTile(size_t tileIndex, TrackVolumeData &volData)
{
    // for each volume bar
    size_t firstBarIndex = (size_t)(volData.tileStartSample / TILE_BAR_SAMPLE_WIDTH);
    size_t lastBarIndex = (size_t)(volData.tileEndSample / TILE_BAR_SAMPLE_WIDTH);

    SecondTile &tile = secondTilesRingBuffer[tileIndex];

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

    auto &trackData = existingTrackData->second;

    if (chan == 0 || chan == 2)
    {
        for (size_t i = 0; i < BARS_PER_TILE; i++)
        {
            if (i >= firstBarIndex && i <= lastBarIndex)
            {
                trackData[i] = volumeToSet;
            }
            else
            {
                trackData[i] = 0.0f;
            }
        }
    }

    if (chan == 1 || chan == 2)
    {
        for (size_t i = 0; i < BARS_PER_TILE; i++)
        {
            if (i >= firstBarIndex && i <= lastBarIndex)
            {
                trackData[BARS_PER_TILE + i] = volumeToSet;
            }
            else
            {
                trackData[BARS_PER_TILE + i] = 0.0f;
            }
        }
    }
    secondTilesToDraw.insert(tileIndex);
}

void VolumeOverTimeGraph::drawUpdatedTileBars()
{
    // this keep track of the top value of each stacked bar
    std::array<int, MAX_NUM_TRACKS_PER_TILE> lastStackedValue;
    lastStackedValue.fill(0.0f);

    for (const auto &tileIndex : secondTilesToDraw)
    {
        SecondTile &tile = secondTilesRingBuffer[tileIndex];
        tile.mesh->clearAllData();
        for (const auto &trackData : tile.trackVolumes)
        {

            // TODO: complete this implementation of drawing
            // and entire tile of bars for both channels

            // drawing top half (left channel)
            for (size_t i = 0; i < BARS_PER_TILE; i++)
            {
                // coordinate system: [0, ]
                int barTopPixel = lastStackedValue[i] + (MAX_SECOND_TILE_TRACK_PIXEL_SIZE * trackData.second[i]);
                int barBottomPixel = lastStackedValue[i];
                int barLeftPixel = i * TILE_BAR_PIXEL_WIDTH;
                int barRightPixel = barLeftPixel + (TILE_BAR_PIXEL_WIDTH - 1);

                int x = barLeftPixel;
                int y = barBottomPixel;

                // tile.mesh->setRectangle(x, y, width, height, color);
            }

            lastStackedValue.fill(0.0f);
        }
    }
}

void VolumeOverTimeGraph::applyQueuedColorUpdates()
{
    // TODO: Implement color updates
}

void VolumeOverTimeGraph::eventuallyRefreshGPUTextures()
{
    // TODO: Implement GPU texture refresh
}

void VolumeOverTimeGraph::drawGlMeshes()
{
    // TODO: Implement OpenGL mesh drawing
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
    /*
    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
    {
        secondTilesRingBuffer[i].mesh->freeGlObjects();
    }
    texturedPositionedShader->release();
    */
}