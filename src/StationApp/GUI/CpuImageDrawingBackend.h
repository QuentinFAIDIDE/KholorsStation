#pragma once

#include "StationApp/GUI/FftDrawingBackend.h"
#include <cstdint>
#include <memory>

#define IMAGES_RING_BUFFER_SIZE 128
#define VISUAL_SAMPLE_RATE 48000
#define MIN_DB -64.0f
#define CPU_IMAGE_FFT_BACKEND_UPDATE_INTERVAL_MS 100
#define SECOND_TILE_WIDTH 32
#define SECOND_TILE_HEIGHT 256
#define MAX_SCALE_SAMPLE_PER_PIXEL 20000
#define MIN_SCALE_SAMPLE_PER_PIXEL 40
#define PIXEL_SCALE_SPEED 0.01f

class CpuImageDrawingBackend : public FftDrawingBackend, public juce::Timer
{
  public:
    CpuImageDrawingBackend(TrackInfoStore &tis);
    ~CpuImageDrawingBackend();

    void paint(juce::Graphics &g) override;

    /**
     * @brief An image tile of one second of signal for a track.
     */
    struct TrackSecondTile
    {
        TrackSecondTile() : img(juce::Image::PixelFormat::SingleChannel, SECOND_TILE_WIDTH, SECOND_TILE_HEIGHT, false)
        {
            for (size_t i = 0; i < SECOND_TILE_WIDTH; i++)
            {
                for (size_t j = 0; j < SECOND_TILE_HEIGHT; j++)
                {
                    img.setPixelAt(i, j, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
                }
            }
        }
        uint64_t trackIdentifer;   /**< Identifier of the track this tile is for */
        int64_t samplePosition;    /**< Position of the tile in samples */
        int64_t tileIndexPosition; /**< Position of the tile in second-tile index */
        juce::Image img;           /**< Image that draws the tile signal */
    };

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
     * @brief Add the fft data inside the task struct to the currently displayed data.
     *
     * @param fftData struct containing the FFt data position, length, channel info and data
     */
    void displayNewFftData(std::shared_ptr<NewFftDataTask> fftData) override;

    /**
     * @brief Called every X ms. We use it here for coalescing ffts
     * into images and eventually repainting.
     * See https://docs.juce.com/master/classTimer.html
     */
    void timerCallback() override;

  private:
    /**
     * @brief Draws the provided FFT (there's only one) on the TrackSecondTile;
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
                       float *data, int channel);

    /**
     * @brief Create a Second Tile object in the secondTilesRingBuffer ring buffer, eventually overwriting/deleting
     * a previous tile, and clear the tile. Return a pointer to the tile.
     *
     * @throws std::invalid_argument when the tile already exist for this track at that position
     *
     * @param trackIdentifier identifier of the track this tile will be for
     * @param secondTileIndex index of the tile in seconds this tile is positioned at
     * @return TrackSecondTile* A pointer to the tile
     */
    TrackSecondTile *createSecondTile(uint64_t trackIdentifier, int64_t secondTileIndex);

    int64_t viewPosition;             /**< Position of the view in samples */
    int64_t viewScale;                /**< Scale of the view in samples per pixels */
    std::mutex secondImageTilesMutex; /**< Mutex to prevent concurrent access to the one-second tiles of data */

    std::vector<TrackSecondTile> secondTilesRingBuffer; /**< Array of tiles that represent one second of track signal */
    size_t secondTileNextIndex; /**< Index of the next tile to create in the secondTilesRingBuffer */
    std::map<std::pair<uint64_t, int64_t>, size_t>
        tileIndexByTrackIdAndPosition; /**< Index of tiles in secondTilesRingBuffer per track id and second tile index
                                        std::pair(track_id, tile_index) */
    int64_t tilesNonce;                /**< A nonce that is incremented when the tiles are updated */
    int64_t lastDrawTilesNonce;        /**< The last nonce tilesNonce drawn */
    std::mutex imageAccessMutex;       /**< Mutex to protect image access */
};