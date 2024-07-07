#pragma once

#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/OpenGL/SolidRectangle.h"
#include "StationApp/OpenGL/TexturedRectangle.h"
#include "juce_opengl/juce_opengl.h"
#include <cstdint>
#include <memory>

class GpuTextureDrawingBackend : public FftDrawingBackend, public juce::OpenGLRenderer
{
  public:
    GpuTextureDrawingBackend(TrackInfoStore &tis, NormalizedUnitTransformer &ft, NormalizedUnitTransformer &it);
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

    struct FftToDraw
    {
        FftToDraw(uint64_t _trackIdentifier, int64_t _secondTileIndex, int64_t _begin, int64_t _end, int fftSize,
                  float *data, int _channel)
        {
            fftData.resize((size_t)fftSize);
            for (size_t i = 0; i < (size_t)fftSize; i++)
            {
                fftData[i] = data[i];
            }
            channel = _channel;
            trackIdentifier = _trackIdentifier;
            secondTileIndex = _secondTileIndex;
            begin = _begin;
            end = _end;
        }
        std::vector<float> fftData; /**< data to draw inside the tile */
        uint64_t trackIdentifier;   /**< identifier of the track tied to the track */
        int64_t secondTileIndex;    /**< index of the tile to draw in */
        int64_t begin, end;         /**< begin and end horizontal pixel coordinates */
        int channel;                /**< 0 for left, 1 for right, 2 for both */
    };

    /**
     * @brief An identifier for the GPU convolution
     * performed at openGL rendering time.
     */
    enum GpuConvolution
    {
        Identity = 0,
        Edge0 = 1,
        Edge1 = 2,
        Edge2 = 3,
        Sharpen = 4,
        BoxBlur = 5,
        GaussianBlur = 6,
        Emboss = 7,
    };

    void paint(juce::Graphics &g) override;
    void resized() override;

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
     * @brief Update the bpm.
     *
     * @param newBpm new bpm value to use to draw the grid
     */
    void updateBpm(float newBpm) override;

    /**
     * @brief clears on screen data.
     * In this openGL version, queue clearing to be done by openGL Thread.
     */
    void clearDisplayedFFTs() override;

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
     * @brief Return a list of ranges where specific tracks have been
     * cleared from.
     * @return std::vector<ClearTrackInfoRange> vector of ranges to clear with trackIdentifiers.
     */
    std::vector<ClearTrackInfoRange> getClearedTrackRanges() override;

  private:
    /**
     * @brief Get index of the tile in the tile ring buffer if it exists.
     * Called only from the OpenGL thread.
     *
     * @param trackIdentifier identifier of the track
     * @param secondTileIndex index in seconds of the tile position
     * @return tile index of exists, -1 otherwise
     */
    int64_t getTileIndexIfExists(uint64_t trackIdentifier, int64_t secondTileIndex);

    /**
     * @brief Create a Second Tile object in the secondTilesRingBuffer ring buffer, eventually overwriting/deleting
     * a previous tile, and clear the tile. Return a pointer to the tile.
     * Called only from the OpenGL thread.
     *
     * @throws std::invalid_argument when the tile already exist for this track at that position
     *
     * @param trackIdentifier identifier of the track this tile will be for
     * @param secondTileIndex index of the tile in seconds this tile is positioned at
     * @return index of the new tile in the second-tile ring buffer
     */
    size_t createSecondTile(uint64_t trackIdentifier, int64_t secondTileIndex);

    /**
     * @brief Set a pixel inside an already existing tile.
     * Called only from the OpenGL thread.
     *
     * @param tileRingBufferIndex index of the tile in the ring buffer of tiles
     * @param x horizontal position in pixels
     * @param y vertical position in pixels
     * @param intensity intensity of the FFT at this position, between 0 and 1
     */
    void setTilePixelIntensity(size_t tileRingBufferIndex, int x, int y, float intensity);

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
     * @brief Recompute and send uniform (opengl variables) that are used
     * by the shaders to draw background or FFT tiles.
     * MUST BE CALLED FROM THE OPENGL THREAD.
     */
    void uploadShadersUniforms();

    /**
     * @brief Draws the provided FFT (there's only one) on the TrackSecondTile.
     * In this GPU version of the drawing backend, we just push a task on a queue.
     * This function is not called from the openGL thread.
     * The queue is then readed by the openGL thread which will call drawFftOnOpenGlThread
     *
     * @param trackIdentifier identifier of the track this fft is for
     * @param secondTileIndex index of the second-tile (in seconds starting at zero)
     * @param begin start sample in the tile
     * @param end end sample in the tile
     * @param fftSize number of frequency bins in the provided fft
     * @param data pointer to the floats containing fft bins intensities in decibels
     * @param channel 0 for left, 1 for right, 2 for both
     */
    void drawFftOnTile(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t begin, int64_t end, int fftSize,
                       float *data, int channel) override;

    /**
     * @brief Called by the openGL thread to draw an fft isnide a GPU texture tile.
     *
     * @param fftData Struct with the FFt and position data.
     */
    void drawFftOnOpenGlThread(std::shared_ptr<FftToDraw> fftData);

    /**
     * @brief Set the color of a track. Or rather push a color update to the queue
     * so that the openGL Renderer thread can pick it.
     *
     * @param trackIdentifier identifier of the track to change color of
     * @param col color to apply to the track
     */
    void setTrackColor(uint64_t trackIdentifier, juce::Colour col) override;

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

    std::atomic<bool> ignoreNewData; /**< after the openGL thread closes, prevent access to openGL resources */

    int64_t viewPosition, viewScale, viewHeight, viewWidth; /*< read by gl thread to update uniforms and view */
    GpuConvolution convolutionId;                           /**< Identifier of the GLSL convolution to apply with GPU */
    float bpm;                         /**< values read by openGL thread to update uniforms and view */
    std::mutex glThreadUniformsMutex;  /**< to lock modifications of position, scale or bpm */
    int64_t glThreadUniformsNonce;     /**< to know if we need to update position and  */
    int64_t lastUsedGlThreadUnifNonce; /**< the last nonce value when the uniforms got updated*/

    std::queue<std::shared_ptr<FftToDraw>>
        fftsToDrawQueue;        /**< queue of FFT to be drawn. Depends on the fftsToDrawLock */
    std::mutex fftsToDrawMutex; /**< protect concurrent acces to the queue of ffts to draw */

    std::mutex openGlThreadColorsMutex; /**< Locking color updates queues and color map for the openGL thread */
    std::queue<std::pair<uint64_t, juce::Colour>>
        colorUpdatesToApply; /**< track colors to be applied by openGL thread, need the lock */
    std::map<uint64_t, juce::Colour> knownTrackColors; /**< track colors known to the openGL thread */

    bool needToResetTiles;      /**< true when the openGL thread is expected to clear all tiles */
    std::mutex tilesResetMutex; /**< mutex to protect acces to clearAllTiles */

    float grid0FrameWidth, grid1FrameWidth, grid2FrameWidth; /**< width of the viewer grid levels in audio samples */
    float grid0PixelWidth, grid1PixelWidth, grid2PixelWidth; /**< width of the viewer grid levels in pixels */
    int grid0PixelShift, grid1PixelShift, grid2PixelShift;   /**< Pixel shift from left side in pixels of grid levels */

    std::mutex clearedRangesMutex;
    std::queue<ClearTrackInfoRange> clearedRanges; /**< A list of ranges on which specific tracks were cleared. Here to
                                                      prevent TrackList from showing info about deleted data. */
};