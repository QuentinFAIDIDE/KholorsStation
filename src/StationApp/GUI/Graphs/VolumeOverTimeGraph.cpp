#include "VolumeOverTimeGraph.h"

VolumeOverTimeGraph::VolumeOverTimeGraph(TrackInfoStore &tis) : BaseOverTimeGraph(tis)
{
}

VolumeOverTimeGraph::~VolumeOverTimeGraph()
{
}

void VolumeOverTimeGraph::clearTrackFromRange(uint64_t trackIdentifier, int64_t startSample, int64_t length)
{
    // TODO: Implement track clearing functionality
}

void VolumeOverTimeGraph::setTrackColor(uint64_t trackIdentifier, juce::Colour col)
{
    // TODO: Implement track color setting functionality
}

void VolumeOverTimeGraph::setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm)
{
    // TODO: Implement track selection functionality
}