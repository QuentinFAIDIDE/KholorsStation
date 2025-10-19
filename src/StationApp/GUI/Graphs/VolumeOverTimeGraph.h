#pragma once

#include "StationApp/Audio/NewTrackVolumeDataTask.h"
#include "StationApp/GUI/Graphs/BaseOverTimeGraph.h"
#include "StationApp/OpenGL/TexturedMulticoloredRectangle.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <queue>
#include <unordered_map>
#include <unordered_set>

// NOTE: the class in this file receives audio events with volumes or color
// updates from UI thread, and post that data in queues for the openGL thread to
// draw volume bars over time within tiles of one second of signal (in visual sample rate).

// Maximum size of queue for volume drawing
// requests (UI Thread writes / OpenGL thread reads)
#define MAX_QUEUED_VOLUME_DRAW 256
// Maximum size of queue for tile clearing
// requests (UI Thread writes / OpenGL thread reads)
#define MAX_QUEUED_TILE_REMOVAL 256

// number of one-second tiles allocated in memory
#define MAX_NUM_TILES 256

// Mumber of bars in a one second tile for a channel (left/right).
// Note that we draw the left and the right channel within each tile
// in the top (left chan) and bottom (right chan) halves. Therefore there
// is in practice two times that number of tiles.
#define BARS_PER_TILE 128

#define TILE_PIXEL_HEIGHT 256
#define TILE_PIXEL_WIDTH BARS_PER_TILE

// How many audio samples in visual sample rate one bar contains.
#define TILE_BAR_SAMPLE_WIDTH (VISUAL_SAMPLE_RATE / BARS_PER_TILE)
// Width of a bar in pixel
#define TILE_BAR_PIXEL_WIDTH ((float)(TILE_PIXEL_WIDTH) / BARS_PER_TILE)

// Maximum number of tracks to show in the second tile.
#define MAX_NUM_TRACKS_PER_TILE 32

// Maximum pixel height of one track within a bar of stacked volumes.
#define MAX_SECOND_TILE_TRACK_PIXEL_SIZE ((float)(TILE_PIXEL_HEIGHT / 2) / MAX_NUM_TRACKS_PER_TILE)

// Maximum RMS value when stacking all RMS intensity bars from all 32 channels
#define MAX_TOTAL_TILE_BAR_RMS_VALUE ((float)(MAX_NUM_TRACKS_PER_TILE) * MAX_SHOWABLE_RMS_VOLUME)

class VolumeOverTimeGraph : public BaseOverTimeGraph
{
  public:
    VolumeOverTimeGraph(TrackInfoStore &tis);
    ~VolumeOverTimeGraph();

    struct TrackVolumeData
    {
        uint64_t trackIdentifier;
        int64_t secondTileIndex;
        int64_t tileStartSample;
        int64_t tileEndSample;
        float volumeLevel;
        int channelIdentifier; /* 0=left, 1=right, 2=both */
    };

    /**
     * @brief Represents a tile of one second of audio graph.
     * The content display is a sequence of second long tiles where
     * track volume are drawn together on.
     */
    struct SecondTile
    {
        SecondTile()
            : trackVolumesBuffer(8 * 4 * BARS_PER_TILE * MAX_NUM_TRACKS_PER_TILE),
              trackVolumesPool(&trackVolumesBuffer), trackVolumes(&trackVolumesPool)
        {
            mesh = std::make_shared<TexturedMulticoloredRectangle>(TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);
            if (mesh == nullptr)
            {
                spdlog::error("Unable to init TexturedMulticoloredRectangle");
            }
            tileIndexPosition = -1;
            samplePosition = -1;
            maxHeightRatio = 0.0f;
        }

        // TODO: reimplement a different kind of mesh that has colors and no track id (along with a new shader)

        std::shared_ptr<TexturedMulticoloredRectangle> mesh;
        int64_t samplePosition;                                  /**< Position of the tile in samples */
        int64_t tileIndexPosition;                               /**< Position of the tile in second-tile index */
        float maxHeightRatio;                                    /**< Maximum height at which stacked volumes peak */
        std::pmr::monotonic_buffer_resource trackVolumesBuffer;  /**< preallocated mem for pool */
        std::pmr::unsynchronized_pool_resource trackVolumesPool; /**< pool for track volumes */
        std::pmr::map<uint64_t, std::array<float, BARS_PER_TILE * 2>>
            trackVolumes; /**< map of track volumes per track id, for left and right channels */
    };

    /**
     * @brief Clear tracks data on a specific range.
     */
    void clearTracksFromRange(int64_t startSample, int64_t length);

    /**
     * @brief Set the color of a track.
     */
    void setTrackColor(uint64_t trackIdentifier, juce::Colour col);

    /**
     * @brief Set the currently selected track.
     */
    void setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm);

    void displayNewVolumeData(std::shared_ptr<NewTrackVolumeDataTask> volumeData);

  private:
    void queueVolumeForDrawing(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t tileStartSample,
                               int64_t tileEndSample, float volume, int channelIndex);
    void queueTileRemoval(int64_t tileIndex);

    void deleteTilesQueuedForDeletion();
    void drawQueuedVolumesToTextures();
    void applyQueuedColorUpdates();
    void eventuallyRefreshGPUTextures();
    void drawGlMeshes();

    size_t getOrCreateSecondTile(int64_t secondTileIndex);
    void addVolumeOnTile(size_t tileIndex, TrackVolumeData &volData);
    void drawUpdatedTileBars();

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

    // MEMBERS BELOW

    std::unique_ptr<juce::OpenGLShaderProgram> texturedPositionedShader; /**< shader to draw volume bars */

    std::mutex volumeUpdateQueueMutex;
    std::queue<TrackVolumeData> volumeUpdateQueue; /** UI thread queues volume drawing for openGL thread to process */
    std::queue<TrackVolumeData> volumeUpdateReadQueue; /** swapped under lock with main queue for processing */

    std::mutex tileRemovalQueueMutex;
    std::queue<int64_t> tileRemovalQueue;             /** UI threads queues tile removal for openGL thread to process */
    std::unordered_set<int64_t> tilesToRemoveSet;     /** set to prevent queueing the same tile twice */
    std::queue<int64_t> tileRemovalReadQueue;         /** swapped under lock with main queue for processing */
    std::unordered_set<int64_t> tilesToRemoveReadSet; /** swapped under lock with main set for processing */

    std::array<std::shared_ptr<SecondTile>, MAX_NUM_TILES>
        secondTilesRingBuffer;                               /**< ring buffer of second-tiles to draw on */
    std::unordered_map<int64_t, size_t> secondTilesIndexMap; /**< map of second-tile index to ring buffer index */
    std::queue<size_t> freeSecondTilesIndexes;               /**< index of second tiles that are currently unused */
    std::unordered_set<size_t> secondTilesToDraw; /**< these tiles had new volumes but the texture was not drawn */

    int64_t glIterCount = 0; /**< incremented at each openGL loop iteration, used to periodically perform actions */

    std::unordered_map<uint64_t, juce::Colour> trackColors;
};