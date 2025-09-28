#include "FftOverTimeGraph.h"
#include "StationApp/GUI/AudioConstants.h"
#include "StationApp/GUI/Graphs/FrequencyLinesDrawer.h"
#include "StationApp/OpenGL/OpenGlShaders.h"
#include "StationApp/OpenGL/ShaderHelpers.h"
#include "spdlog/spdlog.h"

FftOverTimeGraph::FftOverTimeGraph(TrackInfoStore &tis, NormalizedUnitTransformer &ft, NormalizedUnitTransformer &it)
    : BaseOverTimeGraph(tis), freqTransformer(ft), intensityTransformer(it), tilesNonce(0),
      freqLines(ft, VISUAL_SAMPLE_RATE >> 1), backgroundColor(KHOLORS_COLOR_BACKGROUND), tmpFreqTransformer(ft),
      tmpIntensityTransformer(it), trackDrawOrderNonce(1), lastTrackDrawOrderNonce(0), secondTileNextIndex(0),
      convolutionId(GpuConvolutionId::Emboss), needToResetTiles(false), renderOpenGlIter(0)
{
    setInterceptsMouseClicks(false, false);
    addAndMakeVisible(freqLines);
}

FftOverTimeGraph::~FftOverTimeGraph()
{
}

void FftOverTimeGraph::clear()
{
    std::lock_guard lock(tilesResetMutex);
    needToResetTiles = true;
}

std::vector<ClearTrackInfoRange> FftOverTimeGraph::getClearedTrackRanges()
{
    std::lock_guard lock(clearedRangesMutex);
    std::vector<ClearTrackInfoRange> response;
    response.reserve(clearedRanges.size());
    while (clearedRanges.size() > 0)
    {
        response.push_back(clearedRanges.front());
        clearedRanges.pop();
    }
    return response;
}

void FftOverTimeGraph::setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm)
{
    {
        std::lock_guard lock(selectedTrackMutex);
        currentlySelectedTrack = selectedTrack;
    }
}

void FftOverTimeGraph::displayNewFftData(std::shared_ptr<NewFftDataTask> fftData,
                                         std::shared_ptr<ProcessingTimerWaitgroup> procTimeWg)
{
    int fftSize = fftData->fftData->size() / fftData->noFFTs;
    int64_t fftSampleWidth = (int64_t)fftData->segmentSampleLength / (int64_t)fftData->noFFTs;
    // for each fft in the received set
    for (size_t i = 0; i < fftData->noFFTs; i++)
    {
        // pointer to the raw data for this fft
        float *fftDataPointer = fftData->fftData->data() + ((size_t)fftSize * i);
        // compute its position and tile index
        int64_t startSample = fftData->segmentStartSample + ((int64_t)i * fftSampleWidth);
        int64_t endSample = startSample + fftSampleWidth;
        if (fftData->sampleRate != VISUAL_SAMPLE_RATE)
        {
            float sampleRateRatio = float(VISUAL_SAMPLE_RATE) / float(fftData->sampleRate);
            startSample = float(startSample) * sampleRateRatio;
            endSample = float(endSample) * sampleRateRatio;
        }
        // in order to align the FFTs to the grid we shift forward by a predefined number of samples
        startSample += FFT_POSITION_FORWARD_SAMPLE_SHIFT;
        endSample += FFT_POSITION_FORWARD_SAMPLE_SHIFT;

        int64_t secondTileIndexStartSample = startSample / VISUAL_SAMPLE_RATE;
        int64_t secondTileIndexEndSample = endSample / VISUAL_SAMPLE_RATE;
        // NOTE: we suppose that single FFT will never be larger than one second (tile width)

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
            int channelIndex = 2;              // 0 for left, 1 for right, 2 for both
            if (fftData->totalNoChannels == 2) // if there are two channel (not mono), this is for one specific
            {
                if (fftData->channelIndex == 0)
                {
                    channelIndex = 0;
                }
                else
                {
                    channelIndex = 1;
                }
            }
            procTimeWg->add();
            drawFftOnTile(fftData->trackIdentifier, j, tileStartSample, tileEndSample, fftSize, fftDataPointer,
                          channelIndex, fftData->sampleRate, fftData->getTaskingManager(), procTimeWg);
        }
    }
}

void FftOverTimeGraph::setTrackColor(uint64_t trackIdentifier, juce::Colour col)
{
    std::pair<uint64_t, juce::Colour> colorToPush(trackIdentifier, col);
    std::lock_guard lock(openGlThreadColorsMutex);
    colorUpdatesToApply.push(colorToPush);
}

int64_t FftOverTimeGraph::getTileIndexIfExists(uint64_t trackIdentifier, int64_t secondTileIndex)
{
    if (shouldIgnoreData())
    {
        return -1;
    }
    auto searchTerms = std::pair<uint64_t, int64_t>(trackIdentifier, secondTileIndex);
    auto tileSearched = tileIndexByTrackIdAndPosition.find(searchTerms);
    if (tileSearched != tileIndexByTrackIdAndPosition.end())
    {
        return (int64_t)tileSearched->second;
    }
    else
    {
        return -1;
    }
}

size_t FftOverTimeGraph::createSecondTile(uint64_t trackIdentifier, int64_t secondTileIndex)
{
    // if we're aborting, it doesn't really matter which id we return as we will ignore writing
    if (shouldIgnoreData())
    {
        return 0;
    }
    // throw invalid argument error if the tile already exist at that position for this track
    auto existingTile =
        tileIndexByTrackIdAndPosition.find(std::pair<uint64_t, int64_t>(trackIdentifier, secondTileIndex));
    if (existingTile != tileIndexByTrackIdAndPosition.end())
    {
        throw std::invalid_argument("called createSecondTile for an already existing tile");
    }
    size_t newTileIndex;
    // if the ring buffer is not full, expand it with a new datum, clear it and return it
    if (secondTilesRingBuffer.size() < IMAGES_RING_BUFFER_SIZE)
    {
        secondTilesRingBuffer.emplace_back();
        if (secondTilesRingBuffer.size() - 1 != secondTileNextIndex)
        {
            throw std::runtime_error(
                "expanding secondTilesRingBuffer but secondTileNextIndex does not match vector size");
        }
        newTileIndex = secondTileNextIndex;
        // register the vertice, textures and triangle ids against openGL
        secondTilesRingBuffer[newTileIndex].mesh->registerGlObjects();
    }
    // if the ring buffer is full, remove nextItem, clear its index and replace it with cleared one before returning
    // it
    else
    {
        newTileIndex = secondTileNextIndex;
        // remove the indexing by track id and position for the previous tile
        tileIndexByTrackIdAndPosition.erase(std::pair<uint64_t, int64_t>(
            secondTilesRingBuffer[newTileIndex].trackIdentifer, secondTilesRingBuffer[newTileIndex].tileIndexPosition));
        // notify FreqView that a range was cleared for a track
        // in order for related components to keep up with what's on screen
        ClearTrackInfoRange clearedRange;
        clearedRange.startSample = VISUAL_SAMPLE_RATE * secondTilesRingBuffer[newTileIndex].tileIndexPosition;
        clearedRange.length = VISUAL_SAMPLE_RATE;
        clearedRange.trackIdentifier = secondTilesRingBuffer[newTileIndex].trackIdentifer;
        {
            std::lock_guard lock(clearedRangesMutex);
            clearedRanges.push(clearedRange);
        }
        // remove the tile from the drawing order tracking
        removeTrackTileFromDrawingOrder(secondTilesRingBuffer[newTileIndex].trackIdentifer, newTileIndex);
        // clear signal from the previous object
        secondTilesRingBuffer[newTileIndex].mesh->clearAllData();
        secondTilesRingBuffer[newTileIndex].mesh->refreshGpuTextureIfChanged();
    }
    // initialize the new tile metadata and tracking in sets
    secondTilesRingBuffer[newTileIndex].tileIndexPosition = secondTileIndex;
    secondTilesRingBuffer[newTileIndex].samplePosition = secondTileIndex * VISUAL_SAMPLE_RATE;
    secondTilesRingBuffer[newTileIndex].trackIdentifer = trackIdentifier;
    addTrackTileToDrawingOrder(secondTilesRingBuffer[newTileIndex].trackIdentifer, newTileIndex);

    juce::Colour col = KHOLORS_COLOR_WHITE;
    auto optionalColor = knownTrackColors.find(trackIdentifier);
    if (optionalColor != knownTrackColors.end())
    {
        col = optionalColor->second;
    }

    secondTilesRingBuffer[newTileIndex].mesh->changeColor(col);
    secondTilesRingBuffer[newTileIndex].mesh->setPosition(secondTilesRingBuffer[newTileIndex].samplePosition,
                                                          VISUAL_SAMPLE_RATE, trackIdentifier);

    auto newIndex = std::pair<uint64_t, int64_t>(trackIdentifier, secondTileIndex);
    auto newSetEntry = std::pair<std::pair<uint64_t, int64_t>, size_t>(newIndex, newTileIndex);
    tileIndexByTrackIdAndPosition.insert(newSetEntry);
    // increment the index of the next to be allocated
    secondTileNextIndex++;
    if (secondTileNextIndex == IMAGES_RING_BUFFER_SIZE)
    {
        secondTileNextIndex = 0;
    }
    return newTileIndex;
}

void FftOverTimeGraph::drawFftOnTile(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t begin, int64_t end,
                                     int fftSize, float *data, int channel, uint32_t sampleRate, TaskingManager *tm,
                                     std::shared_ptr<ProcessingTimerWaitgroup> procTimeWg)
{
    auto newFftToDraw = getIdleFftToDrawStruct();

    // set the data of the FFT to draw
    newFftToDraw->repurpose(trackIdentifier, secondTileIndex, begin, end, fftSize, data, channel, sampleRate,
                            procTimeWg);

    // push to the queue of FFT to draw for the openGL thread to pick it
    {
        std::lock_guard lock(fftsToDrawMutex);
        fftsToDrawQueue.push(newFftToDraw);
    }
}

void FftOverTimeGraph::drawFftToGpuTexture(std::shared_ptr<FftToDraw> fftData)
{
    syncUnitTransformers();
    size_t tileIndexToDrawIn = getTextureTileAtTrackPosition(fftData->trackIdentifier, fftData->secondTileIndex);
    float sampleRateRatio = computeAudioToVisualSampleRateRatio(fftData->sampleRate);

    float startSecond = (float(fftData->begin) / float(VISUAL_SAMPLE_RATE));
    float endSecond = (float(fftData->end) / float(VISUAL_SAMPLE_RATE));
    size_t startPixel = (size_t)juce::jlimit(0, SECOND_TILE_WIDTH - 1, (int)(startSecond * float(SECOND_TILE_WIDTH)));
    size_t endPixel = (size_t)juce::jlimit(0, SECOND_TILE_WIDTH - 1, (int)(endSecond * float(SECOND_TILE_WIDTH)));

    float *baseIntensitiesPointer = getFftPixelIntensitiesLine(fftData, sampleRateRatio);

    if (!shouldIgnoreData())
    {
        secondTilesRingBuffer[tileIndexToDrawIn].mesh->setRepeatedVerticalHalfLine(fftData->channel, startPixel,
                                                                                   endPixel, baseIntensitiesPointer);
    }

    fftData->procTimeWg->recordCompletion();
}

void FftOverTimeGraph::addTrackTileToDrawingOrder(uint64_t trackIdentifier, size_t tileIndexInRingBuffer)
{
    trackDrawOrderNonce++;

    for (auto it = trackTilesInDrawingOrder.begin(); it != trackTilesInDrawingOrder.end(); ++it)
    {
        // if current track tiles list is ours, we append our tile index to it
        if (it->first == trackIdentifier)
        {
            it->second.insert(it->second.begin(), tileIndexInRingBuffer);
            return;
        }
        // if the current track tiles list track id is larger than ours, we insert our track tiles list before it
        if (it->first > trackIdentifier)
        {
            std::pair<uint64_t, std::list<size_t>> newTrackTilesList;
            newTrackTilesList.first = trackIdentifier;
            newTrackTilesList.second.insert(newTrackTilesList.second.begin(), tileIndexInRingBuffer);
            trackTilesInDrawingOrder.insert(it, newTrackTilesList);
            return;
        }
    }
    // if our track was not already in list and if no track id was larger than ours
    // we will insert our track tiles list at the end
    std::pair<uint64_t, std::list<size_t>> newTrackTilesList;
    newTrackTilesList.first = trackIdentifier;
    newTrackTilesList.second.insert(newTrackTilesList.second.begin(), tileIndexInRingBuffer);
    trackTilesInDrawingOrder.insert(trackTilesInDrawingOrder.end(), newTrackTilesList);
}

void FftOverTimeGraph::removeTrackTileFromDrawingOrder(uint64_t trackIdentifier, size_t tileIndexInRingBuffer)
{
    trackDrawOrderNonce++;

    for (auto it = trackTilesInDrawingOrder.begin(); it != trackTilesInDrawingOrder.end(); ++it)
    {
        if (it->first == trackIdentifier)
        {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            {
                if (*it2 == tileIndexInRingBuffer)
                {
                    it->second.erase(it2);
                    return;
                }
            }
        }
    }
    spdlog::warn("Trying to remove a tile that does not exist from drawing order!");
}

void FftOverTimeGraph::ensureTrackTilesDrawOrderIsUpToDate()
{
    if (trackDrawOrderNonce != lastTrackDrawOrderNonce)
    {
        trackTilesDrawOrder.resize(0);
        for (auto it = trackTilesInDrawingOrder.begin(); it != trackTilesInDrawingOrder.end(); ++it)
        {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            {
                trackTilesDrawOrder.push_back(*it2);
            }
        }
        lastTrackDrawOrderNonce = trackDrawOrderNonce;
    }
}

void FftOverTimeGraph::clearAllTilesIfNeeded()
{
    std::lock_guard lock(tilesResetMutex);
    if (needToResetTiles)
    {
        {
            std::lock_guard lock2(clearedRangesMutex);
            while (clearedRanges.size() > 0)
            {
                clearedRanges.pop();
            }
        }

        for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
        {
            if (secondTilesRingBuffer[i].tileIndexPosition >= 0)
            {
                secondTilesRingBuffer[i].mesh->clearAllData();
                secondTilesRingBuffer[i].mesh->refreshGpuTextureIfChanged();
            }
        }
        needToResetTiles = false;
    }
}

void FftOverTimeGraph::drawQueuedFftsToTextures()
{
    std::vector<std::shared_ptr<FftToDraw>> currentFftsToDraw;
    {
        std::lock_guard lock(fftsToDrawMutex);
        while (!fftsToDrawQueue.empty())
        {
            currentFftsToDraw.push_back(fftsToDrawQueue.front());
            fftsToDrawQueue.pop();
        }
    }
    for (size_t i = 0; i < currentFftsToDraw.size(); i++)
    {
        drawFftToGpuTexture(currentFftsToDraw[i]);
        {
            std::lock_guard lock(idleFftToDrawStructMutex);
            idleFftToDrawStructs.push(currentFftsToDraw[i]);
        }
    }
}

void FftOverTimeGraph::eventuallyRefreshGPUTextures()
{
    if (renderOpenGlIter % 5 == 0)
    {
        for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
        {
            secondTilesRingBuffer[i].mesh->refreshGpuTextureIfChanged();
        }
    }
    renderOpenGlIter++;
}

void FftOverTimeGraph::applyQueuedColorUpdates()
{
    std::vector<std::pair<uint64_t, juce::Colour>> colorUpdates;
    {
        std::lock_guard lock(openGlThreadColorsMutex);
        while (colorUpdatesToApply.size() > 0)
        {
            colorUpdates.push_back(colorUpdatesToApply.front());
            colorUpdatesToApply.pop();
        }
    }
    for (size_t i = 0; i < colorUpdates.size(); i++)
    {
        auto existingTrackColor = knownTrackColors.find(colorUpdates[i].first);
        if (existingTrackColor == knownTrackColors.end() || existingTrackColor->second != colorUpdates[i].second)
        {
            knownTrackColors[colorUpdates[i].first] = colorUpdates[i].second;
            for (size_t j = 0; j < secondTilesRingBuffer.size(); j++)
            {
                if (secondTilesRingBuffer[j].trackIdentifer == colorUpdates[i].first)
                {
                    secondTilesRingBuffer[j].mesh->changeColor(colorUpdates[i].second);
                }
            }
        }
    }
}

void FftOverTimeGraph::drawGlFftTextures()
{
    std::optional<uint64_t> selection;
    {
        std::lock_guard lock(selectedTrackMutex);
        selection = currentlySelectedTrack;
    }

    texturedPositionedShader->use();
    ensureTrackTilesDrawOrderIsUpToDate();

    for (size_t i = 0; i < trackTilesDrawOrder.size(); i++)
    {
        if (secondTilesRingBuffer[trackTilesDrawOrder[i]].tileIndexPosition >= 0 &&
            (selection == std::nullopt ||
             selection.value() == secondTilesRingBuffer[trackTilesDrawOrder[i]].trackIdentifer))
        {
            secondTilesRingBuffer[trackTilesDrawOrder[i]].mesh->drawGlObjects();
        }
    }
}

std::shared_ptr<FftOverTimeGraph::FftToDraw> FftOverTimeGraph::getIdleFftToDrawStruct()
{
    std::lock_guard lock(idleFftToDrawStructMutex);
    if (idleFftToDrawStructs.empty())
    {
        return std::make_shared<FftToDraw>();
    }
    auto fft = idleFftToDrawStructs.front();
    idleFftToDrawStructs.pop();
    return fft;
}

void FftOverTimeGraph::syncUnitTransformers()
{
    if (tmpFreqTransformer.getNonce() != freqTransformer.getNonce())
    {
        tmpFreqTransformer.copyTransformer(freqTransformer);
    }
    if (tmpIntensityTransformer.getNonce() != intensityTransformer.getNonce())
    {
        tmpIntensityTransformer.copyTransformer(intensityTransformer);
    }
}

size_t FftOverTimeGraph::getTextureTileAtTrackPosition(uint64_t trackIdentifier, int64_t secondTileIndex)
{
    auto existingTrackTile = getTileIndexIfExists(trackIdentifier, secondTileIndex);
    return existingTrackTile >= 0 ? (size_t)existingTrackTile : createSecondTile(trackIdentifier, secondTileIndex);
}

// TODO: rename to something more meaningfull
float *FftOverTimeGraph::getFftPixelIntensitiesLine(std::shared_ptr<FftToDraw> fftData, float sampleRateRatio)
{
    float rateAdjustedNoFreqBins = float(fftData->fftData.size()) / sampleRateRatio;
    size_t halfTileHeight = (SECOND_TILE_HEIGHT >> 1);
    float verticalPosStrafe = 1.0f / (float(halfTileHeight) - 1.0f);

    fftIntensitiesBuffer.reserve(halfTileHeight);
    float *baseIntensitiesPointer = fftIntensitiesBuffer.data();
    float *nextIntensityToWrite = baseIntensitiesPointer;

    float vposFloat = 0.0f;
    for (size_t verticalPos = 0; verticalPos < halfTileHeight; verticalPos++)
    {
        size_t frequencyBinIndex = rateAdjustedNoFreqBins * tmpFreqTransformer.transformInv(vposFloat);
        vposFloat += verticalPosStrafe;

        float intensityDb = (frequencyBinIndex < 0 || frequencyBinIndex >= fftData->fftData.size())
                                ? MIN_DB
                                : fftData->fftData[frequencyBinIndex];

        float intensityNormalized = (-MIN_DB + intensityDb) / (-MIN_DB);
        *nextIntensityToWrite++ = tmpIntensityTransformer.transform(intensityNormalized);
    }
    return baseIntensitiesPointer;
}

float FftOverTimeGraph::computeAudioToVisualSampleRateRatio(uint32_t sampleRate)
{
    if (sampleRate == VISUAL_SAMPLE_RATE)
    {
        return 1.0f;
    }
    else
    {
        return float(sampleRate) / float(VISUAL_SAMPLE_RATE);
    }
}

void FftOverTimeGraph::resizeChildrenComponents()
{
    freqLines.setBounds(getLocalBounds());
}

void FftOverTimeGraph::resetJuceOpenGLShaders(juce::OpenGLContext &openGLContext)
{
    texturedPositionedShader.reset(new juce::OpenGLShaderProgram(openGLContext));
}

void FftOverTimeGraph::setShadersUniformsAtOpenGlInit()
{
    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("sfftTexture", 0);
}

void FftOverTimeGraph::loadGlObjectsAtInit()
{
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
}

bool FftOverTimeGraph::buildShadersAtInit()
{
    bool builtTexturedShader = ShaderHelpers::buildShader(texturedPositionedShader, fftVertexShader, fftFragmentShader);
    if (!builtTexturedShader)
    {
        spdlog::error("Failed to build textured positioned shaders");
        return false;
    }
    spdlog::info("Built FFt customn shader");
    return builtTexturedShader;
}

void FftOverTimeGraph::uploadAdditionalShadersUniforms()
{
    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("viewPosition", (GLfloat)getViewPositionLockFree());
    texturedPositionedShader->setUniform("viewWidth", (GLfloat)(getViewWidthLockFree() * getViewScaleLockFree()));
    texturedPositionedShader->setUniform("convolutionId", (GLint)convolutionId);
}

void FftOverTimeGraph::glLoopPreDraw()
{
    clearAllTilesIfNeeded();
    drawQueuedFftsToTextures();
    eventuallyRefreshGPUTextures();
    applyQueuedColorUpdates();
}

void FftOverTimeGraph::glLoopDrawOverGrid()
{
    drawGlFftTextures();
}

void FftOverTimeGraph::deallocateOpenGlResources()
{
    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
    {
        secondTilesRingBuffer[i].mesh->freeGlObjects();
    }
    texturedPositionedShader->release();
}