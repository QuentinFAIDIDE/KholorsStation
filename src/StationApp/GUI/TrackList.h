#pragma once

#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "juce_gui_basics/juce_gui_basics.h"

#define MAX_TRACKS_PER_TILE 16
#define TILES_RING_BUFFER_SIZE 128 // it's better if matches FftDrawingBackend bufsize
// thresold between 0 and 1 corresponding to range from MIN_DB to 0
#define ACTIVE_TRACK_THRESOLD_PER_BIN 0.0001
#define BALANCE_SWITCH_THRESOLD 0.5

#define LABELS_AREA_X_PADDING 6
#define LABELS_AREA_Y_PADDING 2
#define LABEL_HEIGHT 40
#define LABELS_INNER_PADDING 6
#define LABELS_BOX_CORNER_SIZE 3
#define LABELS_CIRCLE_TEXT_PADDING 4
#define CIRCLE_PADDING 2

// max proportion (between 0 and 1) of screen height taken
// by labels we allow to be drawn with one label per stereo
// channel before switching to only allow one.
#define MAX_CONFORTABLE_LABELS_PROPORTION 0.7

#define MAX_ITER_LABEL_POSITIONING 20

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
    TrackList(TrackInfoStore &tis, NormalizedUnitTransformer &nut);

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

        /**
         * @brief Remove informations about a track on this tile.
         *
         * @param trackIdentifier identifier of the track to remove info of.
         */
        void clearFromTrackInfo(uint64_t trackIdentifier);
    };

    /**
     * @brief Summarises information about a track
     * (average freq, stereo balance)
     * about an audio segment.
     */
    struct TrackPresenceSummary
    {
        TrackPresenceSummary()
            : trackIdentifier(0), stereoBalance(0), allChannel(0), stereoDiff(0), noFreqsAveragedLeft(0),
              noFreqsAveragedRight(0), leftChannelAvgFreq(0), rightChannelAvgFreq(0)
        {
        }
        uint64_t trackIdentifier;   /**< identifier of the track */
        int stereoBalance;          /**< 0 for left, 1 for right, 2 for both */
        float allChannel;           /** used for computing stereoBalance */
        float stereoDiff;           /** used for computing stereo balance */
        float noFreqsAveragedLeft;  /**< Used for computing avgFreq */
        float noFreqsAveragedRight; /**< Used for computing avgFreq */
        float leftChannelAvgFreq;   /**< used for avgFreq before balance is known */
        float rightChannelAvgFreq;  /**< used for avgFreq before balance is known */
    };

    static bool compareTrackPresence(const TrackPresenceSummary &a, const TrackPresenceSummary &b);

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

    /**
     * @brief Clear information about a track on a range.
     * Usefull when FFTDrawingBackend forgets about a tile
     * and we need to reflect the disapearance of the tile
     * on the labels here.
     * It will not clear anything if the section is smaller
     * than a TrackList tile (TILE_WIDTH).
     *
     * @param trackIdientifier identifier of the track to clear data for
     * @param startSample start sample of the segment to clear (at VISUAL_SAMPLE_RATE)
     * @param length length of the segment to clear (at VISUAL_SAMPLE_RATE)
     */
    void clearTrackFromRange(uint64_t trackIdientifier, int64_t startSample, uint64_t length);

    void setViewPosition(int64_t newVP);
    void setViewScale(int64_t newVS);
    void setFreqViewWidth(int64_t newFVW);

  private:
    /**
     * @brief Tries to position labelRectangle into drawableBounds,
     * which represents frequencies over stereo with 0Hz in the center
     * and Nyquist freq on top and bottom (left and right channel) sides.
     * It will switch between top and bottom area if allowedToSwitchChanSide is true,
     * and will use a base position offset from center (0 to 1) of freqPositionRatio
     * which is linear to pixels (an not frequency with most frequencyTransformer settings).
     *
     * @param labelRectangle moved around and used as a reference for label dimensions, must have height of LABEL_HEIGHT
     * @param drawableBounds area where the label is allowed to be moved
     * @param allowedToSwitchChanSide is this label allowed to go from left channel area to right channel one ?
     * @param freqPositionRatio ratio of position from 0 to 1 from center to top/bottom border (linear to pixels)
     * @param labelPositionRatioInDrawableArea the base ratio of position from 0 to 2 from top to bottom of drawableArea
     * @param drawableHeightHalf helf the height of drawableBounds
     * @param drawnAreas area that were already drawns on screen and that we should not overlap with
     * @return std::optional<juce::Rectangle<int>> a rectangle where to draw the label if there was room for it
     */
    std::optional<juce::Rectangle<int>> tryPositioningTrackLabel(
        juce::Rectangle<int> labelRectangle, juce::Rectangle<int> drawableBounds, bool allowedToSwitchChanSide,
        float freqPositionRatio, float labelPositionRatioInDrawableArea, int drawableHeightHalf,
        std::vector<juce::Rectangle<int>> &drawnAreas, uint64_t trackIdentifier);

    void drawLabel(juce::Graphics &g, juce::Rectangle<int> bounds, std::string trackName, juce::Colour trackColor);

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
     * This will lock the tilesMutex.
     *
     * @param startTile tile position of the first tile
     * @param endTile tile position of the last tile
     * @return std::vector<TrackPresenceSummary>
     */
    std::vector<TrackPresenceSummary> getVisibleTracks(int64_t startTile, int64_t endTile);

    static bool rectIntersectsRectList(juce::Rectangle<int> &rect, std::vector<juce::Rectangle<int>> &list);

    std::vector<TracksPlaybackInfoTile> tilesRingBuffer; /**< ring buffer of tiles track playback info */
    std::map<int64_t, size_t> tileIndexesPerPosition;    /**< tile indexes indexes by tile position in seconds */
    size_t nextTileIndex;                                /**< index of the next tile to create/replace */
    std::mutex tilesMutex;
    int64_t viewPosition, viewScale, freqViewWidth;
    std::mutex viewMutex;

    std::vector<float>
        freqWeight; /**< weight representing width of each freq bin with current freq transfo. Sums to 1 */
    std::vector<float> binFrequencies; /**< frequency of each bin */

    size_t lastUsedFftSize;            /**< if these change, we need to recompute freqWeight and binFreqs */
    int64_t lastUsedSampleRate;        /**< if these change, we need to recompute freqWeight and binFreqs */
    uint64_t lastUsedTransformerNonce; /**< if these change, we need to recompute freqWeight and binFreqs */

    TrackInfoStore &trackInfoStore;
    NormalizedUnitTransformer &frequencyTransformer;
};