#pragma once

#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/OpenGL/SolidRectangle.h"
#include "StationApp/OpenGL/TexturedRectangle.h"
#include "juce_opengl/juce_opengl.h"
#include <memory>

#define CPU_IMAGE_FFT_BACKEND_UPDATE_INTERVAL_MS 100

class GpuTextureDrawingBackend : public FftDrawingBackend, public juce::Timer, public juce::OpenGLRenderer
{
  public:
    GpuTextureDrawingBackend(TrackInfoStore &tis);
    ~GpuTextureDrawingBackend();

    struct TrackSecondTile
    {
        TrackSecondTile()
        {
            mesh = std::make_shared<TexturedRectangle>(SECOND_TILE_WIDTH, SECOND_TILE_HEIGHT, COLOR_WHITE);
            tileIndexPosition = -1;
        }
        std::shared_ptr<TexturedRectangle> mesh;
        uint64_t trackIdentifer;   /**< Identifier of the track this tile is for */
        int64_t samplePosition;    /**< Position of the tile in samples */
        int64_t tileIndexPosition; /**< Position of the tile in second-tile index */
    };

    void paint(juce::Graphics &g) override;

    /**
     * @brief Move the view so that the position at the component left
     * matches the samplePosition (audio sample offset of the song).
     *
     * @param samplePosition audio sample position (audio sample offset of the song) to match
     */
    void updateViewPosition(uint32_t samplePosition) override;

    /**
     * @brief Scale the view
     * @param samplesPerPixel number of audio samples per pixel to display in the viewer
     */
    void updateViewScale(uint32_t samplesPerPixel) override;

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
     * @brief Called every X ms. We use it here to know when displays need updating.
     * See https://docs.juce.com/master/classTimer.html
     */
    void timerCallback() override;

  private:
    /**
     * @brief Get index of the tile in the tile ring buffer if it exists.
     *
     * @param trackIdentifier identifier of the track
     * @param secondTileIndex index in seconds of the tile position
     * @return tile index of exists, -1 otherwise
     */
    int64_t getTileIndexIfExists(uint64_t trackIdentifier, int64_t secondTileIndex) override;

    /**
     * @brief Create a Second Tile object in the secondTilesRingBuffer ring buffer, eventually overwriting/deleting
     * a previous tile, and clear the tile. Return a pointer to the tile.
     *
     * @throws std::invalid_argument when the tile already exist for this track at that position
     *
     * @param trackIdentifier identifier of the track this tile will be for
     * @param secondTileIndex index of the tile in seconds this tile is positioned at
     * @return index of the new tile in the second-tile ring buffer
     */
    size_t createSecondTile(uint64_t trackIdentifier, int64_t secondTileIndex) override;

    /**
     * @brief Set a pixel inside an already existing tile
     *
     * @param tileRingBufferIndex index of the tile in the ring buffer of tiles
     * @param x horizontal position in pixels
     * @param y vertical position in pixels
     * @param intensity intensity of the FFT at this position, between 0 and 1
     */
    void setTilePixelIntensity(size_t tileRingBufferIndex, int x, int y, float intensity) override;

    /**
     * @brief build the opengl shaders programs
     *
     * @return true shaders have been sucessfully built
     * @return false shaders have failed to build
     */
    bool buildShaders();

    /**
     * @brief Build a specific shader program.
     *
     * @param sh the shader program to build into
     * @param vertexShader text of the vertex shader
     * @param fragmentShader text of the fragment shader
     * @return true
     * @return false
     */
    bool buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, std::string vertexShader,
                     std::string fragmentShader);

    /**
     * @brief wrap the updateShadersViewAndGridUniforms function with a boolean
     * that tells if necessary to switch to switch to using the openGL thread.
     * The wrapped function upload the uniform (opengl variables) that are used
     * by the shaders to draw background or FFT tiles.
     *
     * @param fromGlThread if false, will wrap call to updateShadersViewAndGridUniforms with executeOnGlThread, if true,
     * called directly
     */
    void shaderUniformUpdateThreadWrapper(bool fromGlThread);

    /**
     * @brief Recompute and send uniform (opengl variables) that are used
     * by the shaders to draw background or FFT tiles.
     */
    void updateShadersViewAndGridUniforms();

    /**
     * @brief Recomputes the uniform variables to draw the beat grid.
     */
    void computeShadersGridUniformsVars();

    std::vector<TrackSecondTile> secondTilesRingBuffer; /**< Array of tiles that represent one second of track signal */
    size_t secondTileNextIndex; /**< Index of the next tile to create in the secondTilesRingBuffer */
    std::map<std::pair<uint64_t, int64_t>, size_t>
        tileIndexByTrackIdAndPosition; /**< Index of tiles in secondTilesRingBuffer per track id and second tile index
                                        std::pair(track_id, tile_index) */

    int64_t lastDrawTilesNonce; /**< The last nonce tilesNonce drawn */

    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> texturedPositionedShader; /**< shader to draw ffts */
    std::unique_ptr<juce::OpenGLShaderProgram> backgroundGridShader;     /**< Shader to draw grids on background */

    SolidRectangle background; /**< OpenGL Mesh for background */

    std::atomic<int64_t>
        viewPosition; /**< position of the view in samples (using FftDrawingBackend VISUAL_SAMPLE_RATE) */
    std::atomic<int64_t>
        viewScale; /**< zoom level of the view in samples per pixels (using FftDrawingBackend VISUAL_SAMPLE_RATE) */

    std::atomic<float> bpm;          /**< beats per minutes of the DAW */
    std::atomic<bool> ignoreNewData; /**< after the openGL thread closes, prevent access to openGL resources */

    float grid0FrameWidth, grid1FrameWidth, grid2FrameWidth; /**< width of the viewer grid levels in audio samples */
    float grid0PixelWidth, grid1PixelWidth, grid2PixelWidth; /**< width of the viewer grid levels in pixels */
    int grid0PixelShift, grid1PixelShift, grid2PixelShift;   /**< Pixel shift from left side in pixels of grid levels */
};