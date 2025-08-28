#include "FreqOverTimeGraph.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/ProcessingTimerWaitgroup.h"
#include "StationApp/GUI/AudioConstants.h"
#include "StationApp/GUI/Graphs/GraphBorders.h"
#include "StationApp/OpenGL/BeatGridMesh.h"
#include "StationApp/OpenGL/GLInfoLogger.h"
#include "StationApp/OpenGL/OpenGlShaders.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

FreqOverTimeGraph::FreqOverTimeGraph(TrackInfoStore &tis, NormalizedUnitTransformer &ft, NormalizedUnitTransformer &it)
    : trackInfoStore(tis), freqTransformer(ft), intensityTransformer(it), playCursorPosition(0),
      freqLines(ft, VISUAL_SAMPLE_RATE >> 1), tmpFreqTransformer(ft), tmpIntensityTransformer(it),
      timeSignatureGrid(false), topBeatGrid(true), ignoreNewData(true), viewPosition(0), viewScale(150),
      convolutionId(GpuConvolutionId::Emboss), bpm(120), needToResetTiles(false)
{
    setInterceptsMouseClicks(false, false);
    addAndMakeVisible(freqLines);

    timeSignature = 4;
    lastAppliedTimeSignature = 4;
    backgroundColor = KHOLORS_COLOR_BACKGROUND;
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    secondTileNextIndex = 0;
    mouseOnComponent = false;
    lastMouseX = 0;
    lastMouseY = 0;
    lastTrackDrawOrderNonce = 0;
    trackDrawOrderNonce = 1;
}

FreqOverTimeGraph::~FreqOverTimeGraph()
{
}

void FreqOverTimeGraph::paint(juce::Graphics &g)
{

    int64_t viewPositionCopy, viewScaleCopy;
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewPositionCopy = viewPosition;
        viewScaleCopy = viewScale;
    }
    int playCursorStartPixel = 0;
    {
        std::lock_guard lock(playCursorMutex);
        playCursorStartPixel = (float)(playCursorPosition - viewPositionCopy) / (float)viewScaleCopy;
    }
    // we won't draw the playhead cursor when it's to the 0 position, it's just ugly
    if (playCursorStartPixel > 2)
    {
        auto areaRightToCursor = getLocalBounds().withTrimmedLeft(playCursorStartPixel);
        auto playCursorBounds = areaRightToCursor.withWidth(PLAY_CURSOR_WIDTH);

        g.setColour(KHOLORS_COLOR_WHITE);
        g.fillRect(playCursorBounds);
    }

    // we draw mouse cursor horizontal and vertical lines
    if (mouseOnComponent)
    {
        auto horizontalLine = getLocalBounds().withHeight(1).withY(lastMouseY);
        auto verticalLine = getLocalBounds().withWidth(1).withX(lastMouseX);
        g.setColour(KHOLORS_COLOR_WHITE);
        g.fillRect(horizontalLine);
        g.fillRect(verticalLine);
    }
}

void FreqOverTimeGraph::paintOverChildren(juce::Graphics &g)
{
    drawGraphBorders(g, getLocalBounds(), true);
}

void FreqOverTimeGraph::resized()
{
    freqLines.setBounds(getLocalBounds());

    std::lock_guard lock(glThreadUniformsMutex);
    viewHeight = getLocalBounds().getHeight();
    viewWidth = getLocalBounds().getWidth();
    glThreadUniformsNonce++;
}

void FreqOverTimeGraph::updateViewPosition(uint32_t samplePosition)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewPosition = samplePosition;
        glThreadUniformsNonce++;
    }
}

void FreqOverTimeGraph::updateViewScale(uint32_t samplesPerPixel)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewScale = samplesPerPixel;
        glThreadUniformsNonce++;
    }
}

void FreqOverTimeGraph::updateBpm(float nbpm, TaskingManager *tm)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        bpm = nbpm;
        glThreadUniformsNonce++;
    }
}

void FreqOverTimeGraph::timeSignatureNumeratorUpdate(int numerator)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        timeSignature = numerator;
    }
}

void FreqOverTimeGraph::newOpenGLContextCreated()
{
    spdlog::info("Initializing OpenGL context...");
    // Instanciate an instance of OpenGLShaderProgram
    texturedPositionedShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    // Compile and link the shader
    if (buildShaders())
    {
        spdlog::info("Sucessfully compiled OpenGL shaders");

        backgroundGridShader->use();
        backgroundGridShader->setUniform("gridTexture", 0);

        // initialize background openGL objects
        timeSignatureGrid.registerGlObjects();
        timeSignatureGrid.generateBeatGridTexture(4);
        timeSignatureGrid.refreshGpuTexture();

        topBeatGrid.registerGlObjects();
        topBeatGrid.generateBeatGridTexture(4);
        topBeatGrid.refreshGpuTexture();

        texturedPositionedShader->use();
        texturedPositionedShader->setUniform("sfftTexture", 0);

        uploadShadersUniforms();

        // log some info about openGL version and all
        logOpenGLInfoCallback(openGLContext);

        // enable the error logging
        enableOpenGLErrorLogging();

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
    else
    {
        spdlog::error("FATAL: Unable to compile OpenGL Shaders");
        throw std::runtime_error("FATAL: Unable to compile OpenGL Shaders");
    }
    ignoreNewData = false;
}

bool FreqOverTimeGraph::buildShaders()
{
    bool builtTexturedShader = buildShader(texturedPositionedShader, fftVertexShader, fftFragmentShader);
    if (!builtTexturedShader)
    {
        std::cerr << "Failed to build textured positioned shaders" << std::endl;
        return false;
    }
    bool builtColoredShader =
        buildShader(backgroundGridShader, gridBackgroundVertexShader, gridBackgroundFragmentShader);
    if (!builtColoredShader)
    {
        std::cerr << "Failed to build coloured positioned shaders" << std::endl;
        return false;
    }
    return true;
}

bool FreqOverTimeGraph::buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, std::string vertexShader,
                                    std::string fragmentShader)
{
    return sh->addVertexShader(vertexShader) && sh->addFragmentShader(fragmentShader) && sh->link();
}

void FreqOverTimeGraph::uploadShadersUniforms()
{
    std::lock_guard lock(glThreadUniformsMutex);
    if (lastUsedGlThreadUnifNonce != glThreadUniformsNonce)
    {

        texturedPositionedShader->use();
        texturedPositionedShader->setUniform("viewPosition", (GLfloat)viewPosition);
        texturedPositionedShader->setUniform("viewWidth", (GLfloat)(viewWidth * viewScale));
        texturedPositionedShader->setUniform("convolutionId", (GLint)convolutionId);

        backgroundGridShader->use();
        timeSignatureGrid.updateGridPosition(viewPosition, viewScale, viewWidth, bpm);
        topBeatGrid.setTimeSignatureNumerator(timeSignature);
        topBeatGrid.updateGridPosition(viewPosition, viewScale, viewWidth, bpm);

        lastUsedGlThreadUnifNonce = glThreadUniformsNonce;
    }
}

void FreqOverTimeGraph::renderOpenGL()
{
    // if necessary, clear all tiles first
    {
        std::lock_guard lock(tilesResetMutex);
        if (needToResetTiles)
        {
            {
                // clear the queues of track ranges to clear as
                // everything will be cleared in other components anyway
                // when a full clearing is performed here
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

    // process queue of ffts to draw inside tiles
    std::vector<std::shared_ptr<FftToDraw>> currentFftsToDraw;
    {
        std::lock_guard lock(fftsToDrawMutex);
        while (fftsToDrawQueue.size() > 0)
        {
            currentFftsToDraw.push_back(fftsToDrawQueue.front());
            fftsToDrawQueue.pop();
        }
    }
    for (size_t i = 0; i < currentFftsToDraw.size(); i++)
    {
        drawFftOnOpenGlThread(currentFftsToDraw[i]);
        {
            std::lock_guard lock(idleFftToDrawStructMutex);
            idleFftToDrawStructs.push(currentFftsToDraw[i]);
        }
    }

    // upload texture that changed, but only half of the time
    if (renderOpenGlIter % 5 == 0)
    {
        for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
        {
            secondTilesRingBuffer[i].mesh->refreshGpuTextureIfChanged();
        }
    }
    renderOpenGlIter++;

    // apply color updates
    std::vector<std::pair<uint64_t, juce::Colour>> colorUpdates;
    {
        std::lock_guard lock(openGlThreadColorsMutex);
        while (colorUpdatesToApply.size() > 0)
        {
            colorUpdates.push_back(colorUpdatesToApply.front());
            colorUpdatesToApply.pop();
        }
    }
    // for each color update to apply, check if indeed is new and apply to all track tiles
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

    // update GLSL uniforms if necessary (component height, view position or zoom) updates
    uploadShadersUniforms();

    // enable the damn blending
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);

    // clear screen
    juce::gl::glClearColor(backgroundColor.getFloatRed(), backgroundColor.getFloatGreen(),
                           backgroundColor.getFloatBlue(), 1.0f);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

    // draw background
    backgroundGridShader->use();

    // update time signature of the beat grid if necessary
    int newTimeSignature;
    {
        std::lock_guard lock(glThreadUniformsMutex);
        newTimeSignature = timeSignature;
    }
    if (newTimeSignature != lastAppliedTimeSignature)
    {
        topBeatGrid.generateBeatGridTexture(newTimeSignature);
        topBeatGrid.refreshGpuTexture();
        lastAppliedTimeSignature = newTimeSignature;
    }

    int viewScaleCopy;
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewScaleCopy = viewScale;
    }
    timeSignatureGrid.setVisible(viewScaleCopy < MAX_TIME_SIGNATURE_GRID_VIEW_SCALE);
    topBeatGrid.setVisible(viewScaleCopy >= MAX_TIME_SIGNATURE_GRID_VIEW_SCALE);
    timeSignatureGrid.drawGlObjects();
    topBeatGrid.drawGlObjects();

    std::optional<uint64_t> selection;
    {
        std::lock_guard lock(selectedTrackMutex);
        selection = currentlySelectedTrack;
    }

    // draw tiles with FFTs
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

void FreqOverTimeGraph::setTrackColor(uint64_t trackIdentifier, juce::Colour col)
{
    std::pair<uint64_t, juce::Colour> colorToPush(trackIdentifier, col);
    std::lock_guard lock(openGlThreadColorsMutex);
    colorUpdatesToApply.push(colorToPush);
}

void FreqOverTimeGraph::clearDisplayedFFTs()
{
    std::lock_guard lock(tilesResetMutex);
    needToResetTiles = true;
}

void FreqOverTimeGraph::openGLContextClosing()
{
    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
    {
        secondTilesRingBuffer[i].mesh->freeGlObjects();
    }
    timeSignatureGrid.freeGlObjects();
    topBeatGrid.freeGlObjects();
    backgroundGridShader->release();
    texturedPositionedShader->release();
    ignoreNewData = true;
}

void FreqOverTimeGraph::drawFftOnTile(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t begin, int64_t end,
                                      int fftSize, float *data, int channel, uint32_t sampleRate, TaskingManager *,
                                      std::shared_ptr<ProcessingTimerWaitgroup> procTimeWg)
{

    // either allocate or pick idle FftToDraw struct
    std::shared_ptr<FftToDraw> newFftToDraw;
    {
        std::lock_guard lock(idleFftToDrawStructMutex);
        if (idleFftToDrawStructs.size() == 0)
        {
            newFftToDraw = std::make_shared<FftToDraw>();
        }
        else
        {
            newFftToDraw = idleFftToDrawStructs.front();
            idleFftToDrawStructs.pop();
        }
    }

    // set the data of the FFT to draw
    newFftToDraw->repurpose(trackIdentifier, secondTileIndex, begin, end, fftSize, data, channel, sampleRate,
                            procTimeWg);

    // push to the queue of FFT to draw for the openGL thread to pick it
    {
        std::lock_guard lock(fftsToDrawMutex);
        fftsToDrawQueue.push(newFftToDraw);
    }
}

void FreqOverTimeGraph::drawFftOnOpenGlThread(std::shared_ptr<FftToDraw> fftData)
{
    // if necessary, update the frequency transformers
    if (tmpFreqTransformer.getNonce() != freqTransformer.getNonce())
    {
        tmpFreqTransformer.copyTransformer(freqTransformer);
    }
    if (tmpIntensityTransformer.getNonce() != intensityTransformer.getNonce())
    {
        tmpIntensityTransformer.copyTransformer(intensityTransformer);
    }

    // if the tile does not exists, create it
    size_t tileToDrawIn;
    auto existingTrackTile = getTileIndexIfExists(fftData->trackIdentifier, fftData->secondTileIndex);
    if (existingTrackTile >= 0)
    {
        tileToDrawIn = (size_t)existingTrackTile;
    }
    else
    {
        tileToDrawIn = createSecondTile(fftData->trackIdentifier, fftData->secondTileIndex);
    }
    // we need to correct frequencies if the sample rates differs
    float sampleRateRatio = 1.0f;
    if (fftData->sampleRate != VISUAL_SAMPLE_RATE)
    {
        sampleRateRatio = float(fftData->sampleRate) / float(VISUAL_SAMPLE_RATE);
    }
    // draw the fft inside the tile
    float startSecond = (float(fftData->begin) / float(VISUAL_SAMPLE_RATE));
    float endSecond = (float(fftData->end) / float(VISUAL_SAMPLE_RATE));
    size_t startPixel = (size_t)juce::jlimit(0, SECOND_TILE_WIDTH - 1, (int)(startSecond * float(SECOND_TILE_WIDTH)));
    size_t endPixel = (size_t)juce::jlimit(0, SECOND_TILE_WIDTH - 1, (int)(endSecond * float(SECOND_TILE_WIDTH)));

    // it is necessary to adjust the frequency bin fetching to potentially different sample rates
    float rateAdjustedNoFreqBins = float(fftData->fftData.size()) / sampleRateRatio;

    size_t halfTileHeight = (SECOND_TILE_HEIGHT >> 1);
    float floatHalfTileHeight = float(halfTileHeight);

    float verticalPosStrafe = 1.0f / (floatHalfTileHeight - 1.0f);

    fftIntensitiesBuffer.reserve(halfTileHeight);
    float *baseIntensitiesPointer = fftIntensitiesBuffer.data();
    float *nextIntensityToWrite = baseIntensitiesPointer;

    float vposFloat = 0.0f;
    // iterate from center towards borders
    for (size_t verticalPos = 0; verticalPos < halfTileHeight; verticalPos++)
    {
        size_t frequencyBinIndex = rateAdjustedNoFreqBins * tmpFreqTransformer.transformInv(vposFloat);
        vposFloat += verticalPosStrafe;

        float intensityDb;
        if (frequencyBinIndex < 0 || frequencyBinIndex >= fftData->fftData.size())
        {
            intensityDb = MIN_DB;
        }
        else
        {
            intensityDb = fftData->fftData[frequencyBinIndex];
        }

        float intensityNormalized = (-MIN_DB + intensityDb) / (-MIN_DB);
        intensityNormalized = tmpIntensityTransformer.transform(intensityNormalized);

        *nextIntensityToWrite = intensityNormalized;
        nextIntensityToWrite++;
    }

    if (!ignoreNewData)
    {
        secondTilesRingBuffer[tileToDrawIn].mesh->setRepeatedVerticalHalfLine(fftData->channel, startPixel, endPixel,
                                                                              baseIntensitiesPointer);
    }

    fftData->procTimeWg->recordCompletion();
}

int64_t FreqOverTimeGraph::getTileIndexIfExists(uint64_t trackIdentifier, int64_t secondTileIndex)
{
    if (ignoreNewData)
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

size_t FreqOverTimeGraph::createSecondTile(uint64_t trackIdentifier, int64_t secondTileIndex)
{
    // if we're aborting, it doesn't really matter which id we return as we will ignore writing
    if (ignoreNewData)
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

void FreqOverTimeGraph::addTrackTileToDrawingOrder(uint64_t trackIdentifer, size_t tileRingBufferIndex)
{
    trackDrawOrderNonce++;

    for (auto it = trackTilesInDrawingOrder.begin(); it != trackTilesInDrawingOrder.end(); ++it)
    {
        // if current track tiles list is ours, we append our tile index to it
        if (it->first == trackIdentifer)
        {
            it->second.insert(it->second.begin(), tileRingBufferIndex);
            return;
        }
        // if the current track tiles list track id is larger than ours, we insert our track tiles list before it
        if (it->first > trackIdentifer)
        {
            std::pair<uint64_t, std::list<size_t>> newTrackTilesList;
            newTrackTilesList.first = trackIdentifer;
            newTrackTilesList.second.insert(newTrackTilesList.second.begin(), tileRingBufferIndex);
            trackTilesInDrawingOrder.insert(it, newTrackTilesList);
            return;
        }
    }
    // if our track was not already in list and if no track id was larger than ours
    // we will insert our track tiles list at the end
    std::pair<uint64_t, std::list<size_t>> newTrackTilesList;
    newTrackTilesList.first = trackIdentifer;
    newTrackTilesList.second.insert(newTrackTilesList.second.begin(), tileRingBufferIndex);
    trackTilesInDrawingOrder.insert(trackTilesInDrawingOrder.end(), newTrackTilesList);
}

void FreqOverTimeGraph::removeTrackTileFromDrawingOrder(uint64_t trackIdentifier, size_t tileRingBufferIndex)
{
    trackDrawOrderNonce++;

    for (auto it = trackTilesInDrawingOrder.begin(); it != trackTilesInDrawingOrder.end(); ++it)
    {
        if (it->first == trackIdentifier)
        {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            {
                if (*it2 == tileRingBufferIndex)
                {
                    it->second.erase(it2);
                    return;
                }
            }
        }
    }
    spdlog::warn("Trying to remove a tile that does not exist from drawing order!");
}

void FreqOverTimeGraph::ensureTrackTilesDrawOrderIsUpToDate()
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

void FreqOverTimeGraph::setTilePixelIntensity(size_t tileRingBufferIndex, int x, int y, float intensity)
{
    if (ignoreNewData)
    {
        return;
    }
    secondTilesRingBuffer[tileRingBufferIndex].mesh->setPixelAt(x, y, intensity);
}

std::vector<ClearTrackInfoRange> FreqOverTimeGraph::getClearedTrackRanges()
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

void FreqOverTimeGraph::setMouseCursor(bool onComponent, int x, int y)
{
    mouseOnComponent = onComponent;
    lastMouseX = x;
    lastMouseY = y;
}

void FreqOverTimeGraph::setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm)
{
    {
        std::lock_guard lock(selectedTrackMutex);
        currentlySelectedTrack = selectedTrack;
    }
}

int64_t FreqOverTimeGraph::getPlayCursorPosition()
{
    std::lock_guard lock(playCursorMutex);
    return playCursorPosition;
}

/**
 * @brief Submit a new play cursor position to the drawing backend, which
 * may or may not accept it. It will first be converted to a position in
 * with a sample rate of VISUAL_SAMPLE_RATE.
 *
 * @param samplePosition sample position of the play cursor
 * @param sampleRate sample rate in which the cursor position is given
 */
void FreqOverTimeGraph::submitNewPlayCursorPosition(int64_t samplePosition, uint32_t sampleRate)
{
    std::lock_guard lock(playCursorMutex);
    int64_t newPlayCursorPos = samplePosition;
    if (sampleRate != VISUAL_SAMPLE_RATE)
    {
        newPlayCursorPos = (int64_t)(float(samplePosition) * ((float)VISUAL_SAMPLE_RATE / (float)sampleRate));
    }

    // only update the play cursor pos if it's bigger than previous one,
    // or if it's smaller and beyond a certain distance
    if (newPlayCursorPos > playCursorPosition ||
        std::abs(playCursorPosition - newPlayCursorPos) > MIN_SAMPLE_PLAY_CURSOR_BACKWARD_MOVEMENT)
    {
        playCursorPosition = newPlayCursorPos;
    }
}

/**
 * @brief Add the fft data inside the task struct to the currently displayed data.
 *
 * @param fftData struct containing the FFt data position, length, channel info and data
 */
void FreqOverTimeGraph::displayNewFftData(std::shared_ptr<NewFftDataTask> fftData,
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