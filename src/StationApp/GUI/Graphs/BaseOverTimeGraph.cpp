#include "BaseOverTimeGraph.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/Graphs/Constants.h"
#include "StationApp/GUI/Graphs/GraphBorders.h"
#include "StationApp/OpenGL/BeatGridMesh.h"
#include "StationApp/OpenGL/OpenGLHelpers.h"
#include "StationApp/OpenGL/OpenGlShaders.h"
#include "StationApp/OpenGL/ShaderHelpers.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_opengl/juce_opengl.h"
#include <spdlog/spdlog.h>

BaseOverTimeGraph::BaseOverTimeGraph(TrackInfoStore &tis)
    : trackInfoStore(tis), timeSignatureGrid(false), topBeatGrid(true), ignoreNewData(true), viewPosition(0),
      viewScale(150), bpm(120), needToResetTiles(false)
{
    setInterceptsMouseClicks(false, false);

    timeSignature = 4;
    lastAppliedTimeSignature = 4;
    backgroundColor = KHOLORS_COLOR_BACKGROUND;
    openGLContext.setRenderer(this);
    // The dashboard shaders use GLSL 3.30. Request a core profile that
    // supports it instead of JUCE's default legacy context on macOS.
    openGLContext.setOpenGLVersionRequired(juce::OpenGLContext::openGL4_1);
    openGLContext.attachTo(*this);
    mouseOnComponent = false;
    lastMouseX = 0;
    lastMouseY = 0;
}

BaseOverTimeGraph::~BaseOverTimeGraph()
{
}

void BaseOverTimeGraph::paint(juce::Graphics &g)
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

void BaseOverTimeGraph::paintOverChildren(juce::Graphics &g)
{
    graphBorders.draw(g, getLocalBounds(), true);
}

void BaseOverTimeGraph::resized()
{
    resizeChildrenComponents();

    std::lock_guard lock(glThreadUniformsMutex);
    viewHeight = getLocalBounds().getHeight();
    viewWidth = getLocalBounds().getWidth();
    glThreadUniformsNonce++;
}

void BaseOverTimeGraph::updateViewPosition(uint32_t samplePosition)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewPosition = samplePosition;
        glThreadUniformsNonce++;
    }
}

void BaseOverTimeGraph::updateViewScale(uint32_t samplesPerPixel)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        viewScale = samplesPerPixel;
        glThreadUniformsNonce++;
    }
}

void BaseOverTimeGraph::updateBpm(float nbpm, TaskingManager *tm)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        bpm = nbpm;
        glThreadUniformsNonce++;
    }
}

void BaseOverTimeGraph::timeSignatureNumeratorUpdate(int numerator)
{
    {
        std::lock_guard lock(glThreadUniformsMutex);
        timeSignature = numerator;
    }
}

void BaseOverTimeGraph::newOpenGLContextCreated()
{
    spdlog::info("Initializing OpenGL context...");
    // Instanciate an instance of OpenGLShaderProgram
    backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
    resetJuceOpenGLShaders(openGLContext);
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

        setShadersUniformsAtOpenGlInit();

        uploadShadersUniforms();

        // log some info about openGL version and all
        OpenGLHelpers::logOpenGLInfo(openGLContext);

        // enable the error logging
        OpenGLHelpers::enableOpenGLErrorLogging();

        loadGlObjectsAtInit();
        spdlog::info("user openGl data loaded");
    }
    else
    {
        spdlog::error("FATAL: Unable to compile OpenGL Shaders");
        throw std::runtime_error("FATAL: Unable to compile OpenGL Shaders");
    }
    ignoreNewData = false;
}

bool BaseOverTimeGraph::buildAllShaders()
{
    bool builtBackgroundShader =
        ShaderHelpers::buildShader(backgroundGridShader, gridBackgroundVertexShader, gridBackgroundFragmentShader);
    if (!builtBackgroundShader)
    {
        spdlog::error("Failed to build grid shaders");
        return false;
    }
    spdlog::info("Built grid shaders");
    return builtBackgroundShader && buildShadersAtInit();
}

void BaseOverTimeGraph::uploadShadersUniforms()
{
    std::lock_guard lock(glThreadUniformsMutex);
    if (lastUsedGlThreadUnifNonce != glThreadUniformsNonce)
    {
        uploadAdditionalShadersUniforms();

        backgroundGridShader->use();
        timeSignatureGrid.updateGridPosition(viewPosition, viewScale, viewWidth, bpm);
        topBeatGrid.setTimeSignatureNumerator(timeSignature);
        topBeatGrid.updateGridPosition(viewPosition, viewScale, viewWidth, bpm);

        lastUsedGlThreadUnifNonce = glThreadUniformsNonce;
    }
}

void BaseOverTimeGraph::renderOpenGL()
{
    glLoopPreDraw();
    uploadShadersUniforms();
    OpenGLHelpers::clearGlView(backgroundColor);
    drawGlBackgroundBeatgrid();
    glLoopDrawOverGrid();
}

void BaseOverTimeGraph::drawGlBackgroundBeatgrid()
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

void BaseOverTimeGraph::openGLContextClosing()
{
    deallocateOpenGlResources();
    timeSignatureGrid.freeGlObjects();
    topBeatGrid.freeGlObjects();
    backgroundGridShader->release();
    ignoreNewData = true;
}

void BaseOverTimeGraph::setMouseCursor(bool onComponent, int x, int y)
{
    mouseCursor.setMouseCursor(onComponent, x, y);
}

void BaseOverTimeGraph::submitNewPlayCursorPosition(int64_t samplePosition, uint32_t sampleRate)
{
    playCursor.submitNewPlayCursorPosition(samplePosition, sampleRate);
}

void BaseOverTimeGraph::markUniformsAsStale()
{
    std::lock_guard lock(glThreadUniformsMutex);
    glThreadUniformsNonce++;
}
