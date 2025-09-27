#pragma once

#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/Graphs/GraphMouseCursor.h"
#include "StationApp/GUI/Graphs/GraphPlayCursor.h"
#include "StationApp/OpenGL/BeatGridMesh.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_opengl/juce_opengl.h"

class VolumeOverTimeGraph : public juce::Component, juce::OpenGLRenderer
{
  public:
    VolumeOverTimeGraph(TrackInfoStore &tis);
    ~VolumeOverTimeGraph();

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
     * @brief clears on screen data.
     * In this openGL version, queue clearing to be done by openGL Thread.
     */
    void clearDisplayedSections();

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
     * @brief Setting the currently selected track highlighted on screen.
     *
     * @param selectedTrack Optional, being if something is selected the identifier of the track.
     */
    void setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm);

    /**
     * @brief Clear track data from specified range.
     */
    void clearTrackFromRange(uint64_t trackIdentifier, int64_t startSample, int64_t length);

    /**
     * @brief Clear all data.
     */
    void clear();

    /**
     * @brief Set the color of a track.
     */
    void setTrackColor(uint64_t trackIdentifier, juce::Colour col);

    void submitNewPlayCursorPosition(int64_t samplePosition, uint32_t sampleRate);

  private:
    TrackInfoStore &trackInfoStore;

    GraphPlayCursor playCursor;   /**< Manages play cursor position and rendering */
    GraphMouseCursor mouseCursor; /**< Manages mouse cursor crosshair rendering */

    juce::Colour backgroundColor;

    int timeSignature, lastAppliedTimeSignature;

    std::unique_ptr<juce::OpenGLShaderProgram> backgroundGridShader; /**< Shader to draw grids on background */
    juce::OpenGLContext openGLContext;

    BeatGridMesh timeSignatureGrid, topBeatGrid; /**< OpenGL Mesh for background */

    std::atomic<bool> ignoreNewData; /**< after the openGL thread closes, prevent access to openGL resources */
    int64_t viewPosition, viewScale, viewHeight, viewWidth; /*< read by gl thread to update uniforms and view */
    float bpm;                         /**< values read by openGL thread to update uniforms and view */
    std::mutex glThreadUniformsMutex;  /**< to lock modifications of position, scale or bpm */
    int64_t glThreadUniformsNonce;     /**< to know if we need to update position and  */
    int64_t lastUsedGlThreadUnifNonce; /**< the last nonce value when the uniforms got updated*/

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
     * @brief Build a specific shader program.
     */
    bool buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, std::string vertexShader,
                     std::string fragmentShader);

    /**
     * @brief Upload shader uniforms to GPU.
     */
    void uploadShadersUniforms();

    /**
     * @brief Clear OpenGL view and set up blending.
     */
    void clearGlView();

    /**
     * @brief Draw background beat grid.
     */
    void drawGlBackgroundBeatgrid();
};