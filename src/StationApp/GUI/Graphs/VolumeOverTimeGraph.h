#pragma once

#include "StationApp/GUI/Graphs/BaseOverTimeGraph.h"

class VolumeOverTimeGraph : public BaseOverTimeGraph
{
  public:
    VolumeOverTimeGraph(TrackInfoStore &tis);
    ~VolumeOverTimeGraph();

    /**
     * @brief Clear track data from a specific range.
     */
    void clearTrackFromRange(uint64_t trackIdentifier, int64_t startSample, int64_t length);

    /**
     * @brief Set the color of a track.
     */
    void setTrackColor(uint64_t trackIdentifier, juce::Colour col);

    /**
     * @brief Set the currently selected track.
     */
    void setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm);
};