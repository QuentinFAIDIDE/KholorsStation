#include "GpuTextureDrawingBackend.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/FftDrawingBackend.h"

#include "../OpenGL/OpenGlShaders.h"
#include "StationApp/OpenGL/GLInfoLogger.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

GpuTextureDrawingBackend::GpuTextureDrawingBackend(TrackInfoStore &tis)
    : FftDrawingBackend(tis), viewPosition(0), viewScale(150), bpm(120), ignoreNewData(true)
{
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    secondTileNextIndex = 0;
    startTimer(CPU_IMAGE_FFT_BACKEND_UPDATE_INTERVAL_MS);
}

GpuTextureDrawingBackend::~GpuTextureDrawingBackend()
{
}

void GpuTextureDrawingBackend::paint(juce::Graphics &)
{
}

void GpuTextureDrawingBackend::updateViewPosition(uint32_t samplePosition)
{
    viewPosition = samplePosition;
    shaderUniformUpdateThreadWrapper(false);
}

void GpuTextureDrawingBackend::updateViewScale(uint32_t samplesPerPixel)
{
    viewScale = samplesPerPixel;
    shaderUniformUpdateThreadWrapper(false);
}

void GpuTextureDrawingBackend::newOpenGLContextCreated()
{
    spdlog::info("Initializing OpenGL context...");
    // Instanciate an instance of OpenGLShaderProgram
    texturedPositionedShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    // Compile and link the shader
    if (buildShaders())
    {
        spdlog::info("Sucessfully compiled OpenGL shaders");

        texturedPositionedShader->use();
        texturedPositionedShader->setUniform("ourTexture", 0);

        shaderUniformUpdateThreadWrapper(true);

        // log some info about openGL version and all
        logOpenGLInfoCallback(openGLContext);

        // enable the error logging
        enableOpenGLErrorLogging();

        // initialize background openGL objects
        background.registerGlObjects();
    }
    else
    {
        spdlog::error("FATAL: Unable to compile OpenGL Shaders");
        throw std::runtime_error("FATAL: Unable to compile OpenGL Shaders");
    }
    ignoreNewData = false;
}

bool GpuTextureDrawingBackend::buildShaders()
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

bool GpuTextureDrawingBackend::buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, std::string vertexShader,
                                           std::string fragmentShader)
{
    return sh->addVertexShader(vertexShader) && sh->addFragmentShader(fragmentShader) && sh->link();
}

void GpuTextureDrawingBackend::shaderUniformUpdateThreadWrapper(bool fromGlThread)
{
    // send the new view positions to opengl thread
    if (!fromGlThread)
    {
        openGLContext.executeOnGLThread([this](juce::OpenGLContext &) { updateShadersViewAndGridUniforms(); }, false);
    }
    else
    {
        updateShadersViewAndGridUniforms();
    }
}

void GpuTextureDrawingBackend::updateShadersViewAndGridUniforms()
{

    texturedPositionedShader->use();
    texturedPositionedShader->setUniform("viewPosition", (GLfloat)viewPosition);
    texturedPositionedShader->setUniform("viewWidth", (GLfloat)(getLocalBounds().getWidth() * viewScale));

    computeShadersGridUniformsVars();

    backgroundGridShader->use();
    backgroundGridShader->setUniform("grid0PixelShift", (GLint)grid0PixelShift);
    backgroundGridShader->setUniform("grid0PixelWidth", (GLfloat)grid0PixelWidth);

    backgroundGridShader->setUniform("grid1PixelShift", (GLint)grid1PixelShift);
    backgroundGridShader->setUniform("grid1PixelWidth", (GLfloat)grid1PixelWidth);

    backgroundGridShader->setUniform("grid2PixelShift", (GLint)grid2PixelShift);
    backgroundGridShader->setUniform("grid2PixelWidth", (GLfloat)grid2PixelWidth);

    // BUG: could there be a race condition here with getLocalBounds with the Juce message manager thread (we're from
    // openGL thread here)
    backgroundGridShader->setUniform("viewHeightPixels", (GLfloat)(getLocalBounds().getHeight()));
}

void GpuTextureDrawingBackend::computeShadersGridUniformsVars()
{
    int framesPerMinutes = (60 * VISUAL_SAMPLE_RATE);

    grid0FrameWidth = (float(framesPerMinutes) / bpm);
    grid1FrameWidth = (float(framesPerMinutes) / (bpm * 4));
    grid2FrameWidth = (float(framesPerMinutes) / (bpm * 16));

    grid2PixelWidth = grid2FrameWidth / float(viewScale);
    grid1PixelWidth = grid2PixelWidth * 4;
    grid0PixelWidth = grid1PixelWidth * 4;

    grid0PixelShift = (int(grid0FrameWidth + 0.5f) - (viewPosition % int(grid0FrameWidth + 0.5f))) / viewScale;
    grid1PixelShift = int(grid0PixelShift + 0.5f) % int(grid1PixelWidth + 0.5f);
    grid2PixelShift = int(grid1PixelShift + 0.5f) % int(grid2PixelWidth + 0.5f);
}

void GpuTextureDrawingBackend::renderOpenGL()
{
    std::lock_guard lock(imageAccessMutex);
    if (ignoreNewData)
    {
        return;
    }
    // enable the damn blending
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);

    juce::gl::glClearColor(0.078f, 0.078f, 0.078f, 1.0f);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

    backgroundGridShader->use();
    background.drawGlObjects();

    texturedPositionedShader->use();

    spdlog::debug("Drawing OpenGL textured meshes...");

    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
    {
        if (secondTilesRingBuffer[i].tileIndexPosition >= 0)
        {
            secondTilesRingBuffer[i].mesh->drawGlObjects();
        }
    }
}

void GpuTextureDrawingBackend::openGLContextClosing()
{
    std::lock_guard lock(imageAccessMutex);
    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
    {
        secondTilesRingBuffer[i].mesh->freeGlObjects();
    }
    background.freeGlObjects();
    ignoreNewData = true;
}

void GpuTextureDrawingBackend::timerCallback()
{
    if (ignoreNewData)
    {
        return;
    }

    if (imageAccessMutex.try_lock())
    {
        // if some textures have been updated, re-upload textures to GPU
        if (lastDrawTilesNonce != tilesNonce)
        {
            openGLContext.executeOnGLThread(
                [this](juce::OpenGLContext &) {
                    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
                    {
                        if (secondTilesRingBuffer[i].tileIndexPosition >= 0)
                        {
                            spdlog::debug("Trying to update GPU texture");
                            secondTilesRingBuffer[i].mesh->refreshGpuTextureIfChanged();
                        }
                    }
                },
                true);

            lastDrawTilesNonce = tilesNonce;
            repaint();
        }

        imageAccessMutex.unlock();
    }
}

int64_t GpuTextureDrawingBackend::getTileIndexIfExists(uint64_t trackIdentifier, int64_t secondTileIndex)
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

size_t GpuTextureDrawingBackend::createSecondTile(uint64_t trackIdentifier, int64_t secondTileIndex)
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
        openGLContext.executeOnGLThread(
            [this, newTileIndex](juce::OpenGLContext &) {
                secondTilesRingBuffer[newTileIndex].mesh->registerGlObjects();
            },
            true);
    }
    // if the ring buffer is full, remove nextItem, clear its index and replace it with cleared one before returning it
    else
    {
        newTileIndex = secondTileNextIndex;
        // remove the indexing by track id and position for the previous tile
        tileIndexByTrackIdAndPosition.erase(std::pair<uint64_t, int64_t>(
            secondTilesRingBuffer[newTileIndex].trackIdentifer, secondTilesRingBuffer[newTileIndex].tileIndexPosition));
        // clear signal from the previous object
        for (size_t i = 0; i < SECOND_TILE_WIDTH; i++)
        {
            for (size_t j = 0; j < SECOND_TILE_HEIGHT; j++)
            {
                secondTilesRingBuffer[newTileIndex].mesh->setPixelAt(i, j, 0.0f);
            }
        }
    }
    // initialize the new tile metadata and tracking in sets
    secondTilesRingBuffer[newTileIndex].tileIndexPosition = secondTileIndex;
    secondTilesRingBuffer[newTileIndex].samplePosition = secondTileIndex * VISUAL_SAMPLE_RATE;
    secondTilesRingBuffer[newTileIndex].trackIdentifer = trackIdentifier;
    auto optionalColor = trackInfoStore.getTrackColor(trackIdentifier);
    juce::Colour col = COLOR_WHITE;
    if (optionalColor.has_value())
    {
        col.fromFloatRGBA(optionalColor->red, optionalColor->green, optionalColor->blue, optionalColor->alpha);
    }
    openGLContext.executeOnGLThread(
        [this, newTileIndex, col](juce::OpenGLContext &) {
            secondTilesRingBuffer[newTileIndex].mesh->changeColor(col);
            secondTilesRingBuffer[newTileIndex].mesh->setPosition(secondTilesRingBuffer[newTileIndex].samplePosition,
                                                                  VISUAL_SAMPLE_RATE);
        },
        true);
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

void GpuTextureDrawingBackend::setTilePixelIntensity(size_t tileRingBufferIndex, int x, int y, float intensity)
{
    if (ignoreNewData)
    {
        return;
    }
    secondTilesRingBuffer[tileRingBufferIndex].mesh->setPixelAt(x, y, intensity);
}