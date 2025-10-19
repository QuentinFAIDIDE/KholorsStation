#pragma once

#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/ProcessingTimerWaitgroup.h"
#include "StationApp/GUI/ClearTrackInfoRange.h"
#include "StationApp/GUI/Graphs/BaseOverTimeGraph.h"
#include "StationApp/GUI/Graphs/FrequencyLinesDrawer.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "StationApp/OpenGL/TexturedMonochromeRectangle.h"

#define IMAGES_RING_BUFFER_SIZE 128
// Dimensions of a one-second tile.
// Warning! It is hardcoded as well in the openGL shaders
// for now, but I should soon rely on textureSize instead
// and remove this comment. If I forget well, do it!
#define SECOND_TILE_WIDTH 64
#define SECOND_TILE_HEIGHT 512

class FftOverTimeGraph : public BaseOverTimeGraph
{
  public:
    FftOverTimeGraph(TrackInfoStore &tis, NormalizedUnitTransformer &ft, NormalizedUnitTransformer &it);
    ~FftOverTimeGraph();

    struct TrackSecondTile
    {
        TrackSecondTile()
        {
            mesh = std::make_shared<TexturedMonochromeRectangle>(SECOND_TILE_WIDTH, SECOND_TILE_HEIGHT,
                                                                 KHOLORS_COLOR_WHITE);
            tileIndexPosition = -1;
        }
        std::shared_ptr<TexturedMonochromeRectangle> mesh;
        uint64_t trackIdentifer;   /**< Identifier of the track this tile is for */
        int64_t samplePosition;    /**< Position of the tile in samples */
        int64_t tileIndexPosition; /**< Position of the tile in second-tile index */
    };

    struct FftToDraw
    {
        FftToDraw()
        {
        }

        FftToDraw(uint64_t _trackIdentifier, int64_t _secondTileIndex, int64_t _begin, int64_t _end, int fftSize,
                  float *data, int _channel, uint32_t _sampleRate,
                  std::shared_ptr<ProcessingTimerWaitgroup> _procTimeWg)
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
            sampleRate = _sampleRate;
            procTimeWg = _procTimeWg;
        }

        void repurpose(uint64_t _trackIdentifier, int64_t _secondTileIndex, int64_t _begin, int64_t _end, int fftSize,
                       float *data, int _channel, uint32_t _sampleRate,
                       std::shared_ptr<ProcessingTimerWaitgroup> _procTimeWg)
        {
            fftData.resize((size_t)fftSize);
            float *srcData = data;
            float *dstData = fftData.data();
            for (size_t i = 0; i < (size_t)fftSize; ++i)
            {
                *dstData = *srcData;
                dstData++;
                srcData++;
            }
            channel = _channel;
            trackIdentifier = _trackIdentifier;
            secondTileIndex = _secondTileIndex;
            begin = _begin;
            end = _end;
            sampleRate = _sampleRate;
            procTimeWg = _procTimeWg;
        }

        std::vector<float> fftData; /**< data to draw inside the tile */
        uint64_t trackIdentifier;   /**< identifier of the track tied to the track */
        int64_t secondTileIndex;    /**< index of the tile to draw in */
        int64_t begin, end;         /**< begin and end horizontal pixel coordinates */
        int channel;                /**< 0 for left, 1 for right, 2 for both */
        uint32_t sampleRate;        /**< sample rate of the fft tile */
        std::shared_ptr<ProcessingTimerWaitgroup> procTimeWg;
    };

    /**
     * @brief An identifier for the GPU convolution
     * performed at openGL rendering time.
     */
    enum GpuConvolutionId
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

    /**
     * @brief clears on screen data.
     * In this openGL version, queue clearing to be done by openGL Thread.
     */
    void clear() override;

    /**
     * @brief Return a list of ranges where specific tracks have been
     * cleared from.
     * @return std::vector<ClearTrackInfoRange> vector of ranges to clear with trackIdentifiers.
     */
    std::vector<ClearTrackInfoRange> getClearedTrackRanges();

    /**
     * @brief Setting the currently selected track highlighted on screen.
     *
     * @param selectedTrack Optional, being if something is selected the identifier of the track.
     */
    void setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm);

    /**
     * @brief Add the fft data inside the task struct to the currently displayed data.
     *
     * @param fftData struct containing the FFt data position, length, channel info and data
     */
    void displayNewFftData(std::shared_ptr<NewFftDataTask> fftData,
                           std::shared_ptr<ProcessingTimerWaitgroup> procTimeWg);

    /**
     * @brief Set the color of a track. Or rather push a color update to the queue
     * so that the openGL Renderer thread can pick it.
     *
     * @param trackIdentifier identifier of the track to change color of
     * @param col color to apply to the track
     */
    void setTrackColor(uint64_t trackIdentifier, juce::Colour col);

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
     * @param sampleRate sample rate of data that was passed through fft
     * @param tm a tasking manager (used to check for shutdown and preevent deadlock with emssage thread)
     * @param procTimeWg a waitgroup to be used to notify when work is done, no need to call add, parent already did
     */
    void queueFftForDrawing(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t begin, int64_t end, int fftSize,
                            float *data, int channel, uint32_t sampleRate, TaskingManager *tm,
                            std::shared_ptr<ProcessingTimerWaitgroup> procTimeWg);

    /**
     * @brief Called by the openGL thread to draw an fft isnide a GPU texture tile.
     *
     * @param fftData Struct with the FFt and position data.
     */
    void drawFftToGpuTexture(std::shared_ptr<FftToDraw> fftData);

    /**
     * @brief Add this tile to the track drawing order.
     *
     * @param trackIdentifier
     * @param tileIndexInRingBuffer
     */
    void addTrackTileToDrawingOrder(uint64_t trackIdentifier, size_t tileIndexInRingBuffer);

    /**
     * @brief Remove this tile from the track drawing order.
     *
     * @param trackIdentifier
     * @param tileIndexInRingBuffer
     */
    void removeTrackTileFromDrawingOrder(uint64_t trackIdentifier, size_t tileIndexInRingBuffer);

    /**
     * @brief Called before using the trackTilesDrawOrder vector to ensure it's
     * up to date.
     */
    void ensureTrackTilesDrawOrderIsUpToDate();

    /**
     * @brief Clear all tiles if reset is needed. Called from OpenGL thread.
     */
    void clearAllTilesIfNeeded();

    /**
     * @brief Process and draw all queued FFTs. Called from OpenGL thread.
     */
    void drawQueuedFftsToTextures();

    /**
     * @brief Refresh GPU textures every 5 iterations. Called from OpenGL thread.
     */
    void eventuallyRefreshGPUTextures();

    /**
     * @brief Apply queued color updates to track tiles. Called from OpenGL thread.
     */
    void applyQueuedColorUpdates();

    /**
     * @brief Draw FFT textures for visible tracks. Called from OpenGL thread.
     */
    void drawGlFftTextures();

    /**
     * @brief Get an idle FftToDraw struct from pool or create new one.
     */
    std::shared_ptr<FftToDraw> getIdleFftToDrawStruct();

    /**
     * @brief Sync unit transformers if their nonces have changed.
     */
    void syncUnitTransformers();

    /**
     * @brief Get texture tile at track position, creating if needed.
     */
    size_t getTextureTileAtTrackPosition(uint64_t trackIdentifier, int64_t secondTileIndex);

    /**
     * @brief Generate pixel intensities line from FFT data.
     */
    float *getFftPixelIntensitiesLine(std::shared_ptr<FftToDraw> fftData, float sampleRateRatio);

    float computeAudioToVisualSampleRateRatio(uint32_t sampleRate);

    /**
     * @brief handle updates when the component is resized.
     * Nore specifically, made to propagate the resize to the children.
     */
    void resizeChildrenComponents() override;

    /**
     * @brief Must implement to reset your openGL contex.
     * Typically: backgroundGridShader.reset(new juce::OpenGLShaderProgram(openGLContext));
     *
     * @param openGLContext
     */
    void resetJuceOpenGLShaders(juce::OpenGLContext &openGLContext) override;

    /**
     * @brief Must implement to set the uniforms of your shaders when openGl context
     * is initialized.
     *
     * Typical use:
     *  texturedPositionedShader->use();
     *  texturedPositionedShader->setUniform("sfftTexture", 0);
     */
    void setShadersUniformsAtOpenGlInit() override;

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
    void loadGlObjectsAtInit() override;

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
    bool buildShadersAtInit() override;

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
    void uploadAdditionalShadersUniforms() override;

    /**
     * @brief implement to perform actions in the openGL loop
     * before any type of drawing or gl update is done.
     * Typically, you consume your queue of update events.
     *
     */
    void glLoopPreDraw() override;

    /**
     * @brief implement your openGL drawing logic here.
     */
    void glLoopDrawOverGrid() override;

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
    void deallocateOpenGlResources() override;

    ////////////////////////////
    // MEMBERS
    ////////////////////////////

    NormalizedUnitTransformer &freqTransformer;
    NormalizedUnitTransformer &intensityTransformer;

    int64_t tilesNonce;          /**< A nonce that is incremented when the tiles are updated */
    std::mutex imageAccessMutex; /**< Mutex to protect image access */

    FrequencyLinesDrawer freqLines; /**< A frequency line drawer object */

    juce::Colour backgroundColor;

    TmpNormalizedUnitTransformer tmpFreqTransformer, tmpIntensityTransformer;

    // TODO: rely on an ordered map for drawing instead ?
    std::list<std::pair<uint64_t, std::list<size_t>>>
        trackTilesInDrawingOrder; /**< list of ordered tracks with their tiles lists used to construct
                                     trackTilesDrawOrder */
    uint64_t trackDrawOrderNonce, lastTrackDrawOrderNonce; /**< nonce to know when to regenerate trackTilesDrawOrder */
    std::vector<size_t> trackTilesDrawOrder;               /**< tile indices in ring buffer to draw in order */

    std::vector<TrackSecondTile> secondTilesRingBuffer; /**< Array of tiles that represent one second of track signal */
    size_t secondTileNextIndex; /**< Index of the next tile to create in the secondTilesRingBuffer */

    // TODO: create a custom hash for this pair and use unordered_map
    std::map<std::pair<uint64_t, int64_t>, size_t>
        tileIndexByTrackIdAndPosition; /**< Index of tiles in secondTilesRingBuffer per track id and second tile index
                                        std::pair(track_id, tile_index) */

    int64_t lastDrawTilesNonce; /**< The last nonce tilesNonce drawn */

    std::unique_ptr<juce::OpenGLShaderProgram> texturedPositionedShader; /**< shader to draw ffts */

    GpuConvolutionId convolutionId; /**< Identifier of the GLSL convolution to apply with GPU */

    std::queue<std::shared_ptr<FftToDraw>>
        fftsToDrawQueue;        /**< queue of FFT to be drawn. Depends on the fftsToDrawLock */
    std::mutex fftsToDrawMutex; /**< protect concurrent acces to the queue of ffts to draw */
    std::queue<std::shared_ptr<FftToDraw>> idleFftToDrawStructs; /**< already allocated structs waiting to be filled */
    std::mutex idleFftToDrawStructMutex;

    std::mutex openGlThreadColorsMutex; /**< Locking color updates queues and color map for the openGL thread */
    std::queue<std::pair<uint64_t, juce::Colour>>
        colorUpdatesToApply; /**< track colors to be applied by openGL thread, need the lock */
    std::unordered_map<uint64_t, juce::Colour> knownTrackColors; /**< track colors known to the openGL thread */

    bool needToResetTiles;      /**< true when the openGL thread is expected to clear all tiles */
    std::mutex tilesResetMutex; /**< mutex to protect acces to clearAllTiles */

    std::mutex clearedRangesMutex;
    std::queue<ClearTrackInfoRange> clearedRanges; /**< A list of ranges on which specific tracks were cleared. Here to
                                                      act as a master deleter, not show volume or track names in
                                                      trackList when cleared. */

    std::optional<uint64_t> currentlySelectedTrack;
    std::mutex selectedTrackMutex;

    int64_t renderOpenGlIter; /**< a simple counter which is iterated at each render to track even/odd rendering */

    std::vector<float> fftIntensitiesBuffer; /**< a buffer to set FFT intensities and pass to the textured rectangles */
};