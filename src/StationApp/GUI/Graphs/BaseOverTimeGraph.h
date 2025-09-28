#pragma once

#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/Graphs/GraphMouseCursor.h"
#include "StationApp/GUI/Graphs/GraphPlayCursor.h"
#include "StationApp/OpenGL/BeatGridMesh.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_opengl/juce_opengl.h"

class BaseOverTimeGraph : public juce::Component, juce::OpenGLRenderer
{
  public:
    BaseOverTimeGraph(TrackInfoStore &tis);
    ~BaseOverTimeGraph();

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;

    /**
     * @brief Move the view so that the position at the component left
     * matches the samplePosition (audio sample offset of the song).
     *
     * @param samplePosition audio sample position (audio sample offset of the song) to match
     */
    void updateViewPosition(uint32_t samplePosition);

    /**
     * @brief Scale the view
     * @param samplesPerPixel number of audio samples per pixel to display in the viewer
     */
    void updateViewScale(uint32_t samplesPerPixel);

    /**
     * @brief Update the bpm.
     *
     * @param newBpm new bpm value to use to draw the grid
     */
    void updateBpm(float newBpm, TaskingManager *tm);

    /**
     * @brief Update the time signature of the beat grid.
     *
     * @param timeSignatureNumerator number at numerator of the time signature fraction.
     */
    void timeSignatureNumeratorUpdate(int timeSignatureNumerator);

    /**
     * @brief      Called when opengl context is created.
     */
    void newOpenGLContextCreated() override;

    /**
     * @brief      Renders openGL context
     */
    void renderOpenGL() override;

    /**
     * @brief      Called when opengl context is closed.
     */
    void openGLContextClosing() override;

    /**
     * @brief Set the mouse cursor position on component, as it is intercepted
     * by parent and is not received. We could let the mouse events through
     * but we're with this patterN;
     *
     * @param onComponent is the mouse over this compoennt ?
     * @param x mouse x
     * @param y mouse y
     */
    void setMouseCursor(bool onComponent, int x, int y);

    /**
     * @brief Clear all data that is visible on the graph.
     */
    virtual void clear() {};

    void submitNewPlayCursorPosition(int64_t samplePosition, uint32_t sampleRate);

    /**
     * @brief Get current play cursor position.
     */
    int64_t getPlayCursorPosition()
    {
        return playCursor.getPlayCursorPosition();
    }

  protected:
    TrackInfoStore &trackInfoStore;
    GraphPlayCursor playCursor;   /**< Manages play cursor position and rendering */
    GraphMouseCursor mouseCursor; /**< Manages mouse cursor crosshair rendering */

    /**
     * @brief Check if new data should be ignored (e.g., during OpenGL context shutdown).
     */
    bool shouldIgnoreData() const
    {
        return ignoreNewData;
    }

    /**
     * @brief Get current view position.
     */
    int64_t getViewPosition()
    {
        std::lock_guard lock(glThreadUniformsMutex);
        return viewPosition;
    }

    /**
     * @brief Get current view scale.
     */
    int64_t getViewScale()
    {
        std::lock_guard lock(glThreadUniformsMutex);
        return viewScale;
    }

    /**
     * @brief Get current view width.
     */
    int64_t getViewWidth()
    {
        std::lock_guard lock(glThreadUniformsMutex);
        return viewWidth;
    }

    /**
     * @brief Get current view position without locking (use when mutex is already locked).
     */
    int64_t getViewPositionLockFree()
    {
        return viewPosition;
    }

    /**
     * @brief Get current view scale without locking (use when mutex is already locked).
     */
    int64_t getViewScaleLockFree()
    {
        return viewScale;
    }

    /**
     * @brief Get current view width without locking (use when mutex is already locked).
     */
    int64_t getViewWidthLockFree()
    {
        return viewWidth;
    }

  private:
    juce::Colour backgroundColor;

    int timeSignature, lastAppliedTimeSignature;

    std::unique_ptr<juce::OpenGLShaderProgram> backgroundGridShader; /**< Shader to draw grids on background */
    juce::OpenGLContext openGLContext;

    BeatGridMesh timeSignatureGrid, topBeatGrid; /**< OpenGL Mesh for background */

    std::atomic<bool> ignoreNewData; /**< after the openGL thread closes, prevent access to openGL resources */
    int64_t viewPosition, viewScale, viewHeight, viewWidth; /*< read by gl thread to update uniforms and view */
    float bpm;                                /**< values read by openGL thread to update uniforms and view */
    mutable std::mutex glThreadUniformsMutex; /**< to lock modifications of position, scale or bpm */
    int64_t glThreadUniformsNonce;            /**< to know if we need to update position and  */
    int64_t lastUsedGlThreadUnifNonce;        /**< the last nonce value when the uniforms got updated*/

    std::mutex openGlThreadColorsMutex; /**< Locking color updates queues and color map for the openGL thread */
    std::queue<std::pair<uint64_t, juce::Colour>>
        colorUpdatesToApply; /**< track colors to be applied by openGL thread, need the lock */
    std::map<uint64_t, juce::Colour> knownTrackColors; /**< track colors known to the openGL thread */

    bool needToResetTiles;      /**< true when the openGL thread is expected to clear all tiles */
    std::mutex tilesResetMutex; /**< mutex to protect acces to clearAllTiles */

    int lastMouseX, lastMouseY;
    bool mouseOnComponent;

    int64_t renderOpenGlIter; /**< a simple counter which is iterated at each render to track even/odd rendering */

    /**
     * @brief Build all OpenGL shader programs.
     */
    bool buildAllShaders();

    /**
     * @brief Upload shader uniforms to GPU.
     */
    void uploadShadersUniforms();

    /**
     * @brief Draw background beat grid.
     */
    void drawGlBackgroundBeatgrid();

    /**
     * @brief handle updates when the component is resized.
     * Nore specifically, made to propagate the resize to the children.
     */
    virtual void resizeChildrenComponents() {};

    /**
     * @brief Must implement to reset your openGL contex.
     * Typically: backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
     *
     * @param openGLContext
     */
    virtual void resetJuceOpenGLShaders(juce::OpenGLContext &openGLContext) {};

    /**
     * @brief Must implement to set the uniforms of your shaders when openGl context
     * is initialized.
     *
     * Typical use:
     *  texturedPositionedShader->use();
     *  texturedPositionedShader->setUniform("sfftTexture", 0);
     */
    virtual void setShadersUniformsAtOpenGlInit() {};

    /**
     * @brief Called on openGL context init. You must implement it to load
     * your textures to the GPU. Typical use:
     *
     *  texturedPositionedShader->use();
     *  for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
     *  {
     *      if (secondTilesRingBuffer[i].tileIndexPosition >= 0)
     *      {
     *          secondTilesRingBuffer[i].mesh->registerGlObjects();
     *      }
     *  }
     *
     */
    virtual void loadGlObjectsAtInit() {};

    /**
     * @brief Called at openGL init, you are supposed to
     * implement it if you have shaders to implement,
     * must return false if there was a failure.
     * Typical implementation:
     *
     *     bool builtBackgroundShader =
     *         buildShader(backgroundGridShader, gridBackgroundVertexShader, gridBackgroundFragmentShader);
     *     if (!builtBackgroundShader)
     *     {
     *         std::cerr << "Failed to build grid shaders" << std::endl;
     *         return false;
     *     }
     *
     */
    virtual bool buildShadersAtInit()
    {
        return true;
    };

    /**
     * @brief Implement to upload additional uniforms to the shaders you implenent.
     * Typically:
     *
     *   texturedPositionedShader->use();
     *   texturedPositionedShader->setUniform("viewPosition", (GLfloat)viewPosition);
     *   texturedPositionedShader->setUniform("viewWidth", (GLfloat)(viewWidth * viewScale));
     *   texturedPositionedShader->setUniform("convolutionId", (GLint)convolutionId);
     *
     */
    virtual void uploadAdditionalShadersUniforms() {};

    /**
     * @brief implement to perform actions in the openGL loop
     * before any type of drawing or gl update is done.
     * Typically, you consume your queue of update events.
     *
     */
    virtual void glLoopPreDraw() {};

    /**
     * @brief implement your openGL drawing logic here.
     */
    virtual void glLoopDrawOverGrid() {};

    /**
     * @brief Here you should free the gl objects you have initialized.
     * Example:
     *
     * for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
     * {
     *     secondTilesRingBuffer[i].mesh->freeGlObjects();
     * }
     * texturedPositionedShader->release();
     */
    virtual void deallocateOpenGlResources() {};
};