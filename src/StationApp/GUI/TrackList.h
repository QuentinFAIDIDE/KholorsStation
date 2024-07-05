#pragma once

#include "StationApp/Audio/NewFftDataTask.h"
#include "juce_gui_basics/juce_gui_basics.h"

#define MAX_TRACKS_PER_TILE 16
#define TILES_RING_BUFFER_SIZE 128 // it's better if matches FftDrawingBackend bufsize
#define ACTIVE_TRACK_DB_THRESOLD_PER_BIN -20.0f

/**
 * @brief Component that shows a list of tracks
 * that are to be displayed on the same level
 * as the FreqView. Tracks are displayed depending
 * on if they are on screen, and height depends
 * on their frequency and stereo balance.
 *
 * It has a set of (1-second) tiles with a list track identifiers along
 * their average frequencies and summed intensities.
 */
class TrackList : public juce::Component
{
  public:
    TrackList();

    void paint(juce::Graphics &g) override;

    struct TracksPlaybackInfoTile
    {
        uint64_t trackIdentifiers[MAX_TRACKS_PER_TILE];
        float summedLeftChanIntensities[MAX_TRACKS_PER_TILE];
        float averageLeftChanFrequencies[MAX_TRACKS_PER_TILE];
        float summedRightChanIntensities[MAX_TRACKS_PER_TILE];
        float averageRightChanFrequencies[MAX_TRACKS_PER_TILE];
        size_t leftChanFftCount[MAX_TRACKS_PER_TILE];
        size_t rightChanFftCount[MAX_TRACKS_PER_TILE];
        size_t numSeenTracks; /**< How many track are used in there */
        int64_t tilePosition; /**< Position of the tile in the track (in seconds starting at 0) */
    };

    /**
     * @brief Summarises information about a track
     * (average freq, stereo balance)
     * about an audio segment.
     */
    struct TrackPresenceSummary
    {
        uint64_t trackIdentifier;  /**< identifier of the track */
        int stereoBalance;         /**< 0 for left, 1 for right, 2 for both */
        float allChannel;          /** used for computing stereoBalance */
        float stereoDiff;          /** used for computing stereo balance */
        float avgFreq;             /**< average frequency on the stereo chan */
        float noFreqsAveraged;     /**< Used for computing avgFreq */
        float leftChannelAvgFreq;  /**< used for avgFreq once balance is known */
        float rightChannelAvgFreq; /**< used for avgFreq once balance is known */
    };

    /**
     * @brief Records a SFFT for a track on a channel.
     * It will compute its intensity, average freq pos
     * and average balance on the second tile.
     *
     * @param newFftDataTask data of the new incoming SFFTs
     */
    void recordSfft(std::shared_ptr<NewFftDataTask> newFftDataTask);

    /**
     * @brief Forget about all label data saved.
     */
    void clear();

  private:
    /**
     * @brief Get tile index, and create it if not exists.
     * To be called when you already locked the tileMutex.
     *
     * @param tilePosition tile index in seconds starting at zero.
     * @return size_t index of the tile in the ring buffer.
     */
    size_t getOrCreateTile(int64_t tilePosition);

    /**
     * @brief Get index of the track inside the tile, eventually
     * creating it, will return -1 if no more space for tracks in this
     * tile.
     * Note that you NEEED to have first retrieved the tile index from its
     * position in getOrCreateTile.
     * To be called when you already locked the tileMutex.
     *
     * @param tileIndex index of the tile in the ring buffer
     * @param trackIdentifier track identifier to add.
     * @return index of the tile, -1 if no more room
     */
    int64_t getTrackIndexInTileOrInsert(size_t tileIndex, uint64_t trackIdentifier);

    /**
     * @brief Get a vector of visible track on this section with summarized info.
     *
     * @param startTile tile position of the first tile
     * @param endTile tile position of the last tile
     * @return std::vector<TrackPresenceSummary>
     */
    std::vector<TrackPresenceSummary> getVisibleTracks(int64_t startTile, int64_t endTile);

    std::vector<TracksPlaybackInfoTile> tilesRingBuffer; /**< ring buffer of tiles track playback info */
    std::map<int64_t, size_t> indexOfTilesWithData;      /**< tile indexes indexes by tile position in seconds */
    size_t nextTileIndex;                                /**< index of the next tile to create/replace */
    std::mutex tilesMutex;
};