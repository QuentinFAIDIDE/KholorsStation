#pragma once

#include "StationApp/GUI/FftDrawingBackend.h"
#include <cstdint>
#include <memory>

#define CPU_IMAGE_FFT_BACKEND_UPDATE_INTERVAL_MS 100

class CpuImageDrawingBackend : public FftDrawingBackend, public juce::Timer
{
  public:
    CpuImageDrawingBackend(TrackInfoStore &tis, NormalizedUnitTransformer &ft, NormalizedUnitTransformer &it);
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
    int64_t getTileIndexIfExists(uint64_t trackIdentifier, int64_t secondTileIndex);

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
    size_t createSecondTile(uint64_t trackIdentifier, int64_t secondTileIndex);

    /**
     * @brief Set a pixel inside an already existing tile
     *
     * @param tileRingBufferIndex index of the tile in the ring buffer of tiles
     * @param x horizontal position in pixels
     * @param y vertical position in pixels
     * @param intensity intensity of the FFT at this position, between 0 and 1
     */
    void setTilePixelIntensity(size_t tileRingBufferIndex, int x, int y, float intensity);

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
                       float *data, int channel) override;

    int64_t viewPosition; /**< Position of the view in samples */
    int64_t viewScale;    /**< Scale of the view in samples per pixels */

    std::vector<TrackSecondTile> secondTilesRingBuffer; /**< Array of tiles that represent one second of track signal */
    size_t secondTileNextIndex; /**< Index of the next tile to create in the secondTilesRingBuffer */
    std::map<std::pair<uint64_t, int64_t>, size_t>
        tileIndexByTrackIdAndPosition; /**< Index of tiles in secondTilesRingBuffer per track id and second tile index
                                        std::pair(track_id, tile_index) */

    int64_t lastDrawTilesNonce; /**< The last nonce tilesNonce drawn */
};