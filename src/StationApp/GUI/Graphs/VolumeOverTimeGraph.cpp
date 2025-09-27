#include "VolumeOverTimeGraph.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/Graphs/Constants.h"
#include "StationApp/GUI/Graphs/GraphBorders.h"
#include "StationApp/OpenGL/BeatGridMesh.h"
#include "StationApp/OpenGL/GLInfoLogger.h"
#include "StationApp/OpenGL/OpenGlShaders.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_opengl/juce_opengl.h"
#include <optional>
#include <spdlog/spdlog.h>

VolumeOverTimeGraph::VolumeOverTimeGraph(TrackInfoStore &tis)
    : trackInfoStore(tis), timeSignatureGrid(false), topBeatGrid(true), ignoreNewData(true), viewPosition(0),
      viewScale(150), bpm(120), needToResetTiles(false)
{
    setInterceptsMouseClicks(false, false);

    timeSignature = 4;
    lastAppliedTimeSignature = 4;
    backgroundColor = KHOLORS_COLOR_BACKGROUND;
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    mouseOnComponent = false;
    lastMouseX = 0;
    lastMouseY = 0;
    renderOpenGlIter = 0;
    glThreadUniformsNonce = 0;
    lastUsedGlThreadUnifNonce = 0;
}

VolumeOverTimeGraph::~VolumeOverTimeGraph()
{
}

void VolumeOverTimeGraph::paint(juce::Graphics &g)
{
    int64_t viewPositionCopy, viewScaleCopy;
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewPositionCopy = viewPosition;
        viewScaleCopy = viewScale;
    }

    playCursor.paint(g, getLocalBounds(), viewPositionCopy, viewScaleCopy);
    mouseCursor.paint(g, getLocalBounds());
}

void VolumeOverTimeGraph::paintOverChildren(juce::Graphics &g)
{
    drawGraphBorders(g, getLocalBounds(), true);
}

void VolumeOverTimeGraph::resized()
{
    std::lock_guard lock(glThreadUniformsMutex);
    viewHeight = getLocalBounds().getHeight();
    viewWidth = getLocalBounds().getWidth();
    glThreadUniformsNonce++;
}

void VolumeOverTimeGraph::updateViewPosition(uint32_t samplePosition)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewPosition = samplePosition;
        glThreadUniformsNonce++;
    }
}

void VolumeOverTimeGraph::updateViewScale(uint32_t samplesPerPixel)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewScale = samplesPerPixel;
        glThreadUniformsNonce++;
    }
}

void VolumeOverTimeGraph::updateBpm(float nbpm, TaskingManager *tm)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        bpm = nbpm;
        glThreadUniformsNonce++;
    }
}

void VolumeOverTimeGraph::timeSignatureNumeratorUpdate(int numerator)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        timeSignature = numerator;
    }
}

void VolumeOverTimeGraph::clearDisplayedSections()
{
    // TODO: Clear displayed sections
}

void VolumeOverTimeGraph::newOpenGLContextCreated()
{
    spdlog::info("Initializing OpenGL context...");
    // Instanciate an instance of OpenGLShaderProgram
    backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    // Compile and link the shader
    if (buildAllShaders())
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

        uploadShadersUniforms();

        // log some info about openGL version and all
        logOpenGLInfoCallback(openGLContext);

        // enable the error logging
        enableOpenGLErrorLogging();
    }
    else
    {
        spdlog::error("FATAL: Unable to compile OpenGL Shaders");
        throw std::runtime_error("FATAL: Unable to compile OpenGL Shaders");
    }
    ignoreNewData = false;
}

bool VolumeOverTimeGraph::buildAllShaders()
{
    bool builtBackgroundShader =
        buildShader(backgroundGridShader, gridBackgroundVertexShader, gridBackgroundFragmentShader);
    if (!builtBackgroundShader)
    {
        std::cerr << "Failed to build grid shaders" << std::endl;
        return false;
    }
    return true;
}

bool VolumeOverTimeGraph::buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, std::string vertexShader,
                                      std::string fragmentShader)
{
    return sh->addVertexShader(vertexShader) && sh->addFragmentShader(fragmentShader) && sh->link();
}

void VolumeOverTimeGraph::uploadShadersUniforms()
{
    std::lock_guard lock(glThreadUniformsMutex);
    if (lastUsedGlThreadUnifNonce != glThreadUniformsNonce)
    {
        backgroundGridShader->use();
        timeSignatureGrid.updateGridPosition(viewPosition, viewScale, viewWidth, bpm);
        topBeatGrid.setTimeSignatureNumerator(timeSignature);
        topBeatGrid.updateGridPosition(viewPosition, viewScale, viewWidth, bpm);

        lastUsedGlThreadUnifNonce = glThreadUniformsNonce;
    }
}

void VolumeOverTimeGraph::renderOpenGL()
{
    uploadShadersUniforms();
    clearGlView();
    drawGlBackgroundBeatgrid();
}

void VolumeOverTimeGraph::clearGlView()
{
    // enable the damn blending
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);

    // clear screen
    juce::gl::glClearColor(backgroundColor.getFloatRed(), backgroundColor.getFloatGreen(),
                           backgroundColor.getFloatBlue(), 1.0f);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);
}

void VolumeOverTimeGraph::drawGlBackgroundBeatgrid()
{
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
}

void VolumeOverTimeGraph::openGLContextClosing()
{
    timeSignatureGrid.freeGlObjects();
    topBeatGrid.freeGlObjects();
    backgroundGridShader->release();
    ignoreNewData = true;
}

void VolumeOverTimeGraph::setMouseCursor(bool onComponent, int x, int y)
{
    mouseCursor.setMouseCursor(onComponent, x, y);
}

void VolumeOverTimeGraph::setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm)
{
    // TODO: handle highlight of selected track
}

void VolumeOverTimeGraph::submitNewPlayCursorPosition(int64_t samplePosition, uint32_t sampleRate)
{
    playCursor.submitNewPlayCursorPosition(samplePosition, sampleRate);
}

void VolumeOverTimeGraph::clearTrackFromRange(uint64_t trackIdentifier, int64_t startSample, int64_t length)
{
    // TODO: Clear track from range
}

void VolumeOverTimeGraph::clear()
{
    // TODO: Clear all data
}

void VolumeOverTimeGraph::setTrackColor(uint64_t trackIdentifier, juce::Colour col)
{
    // TODO: Set track color
}