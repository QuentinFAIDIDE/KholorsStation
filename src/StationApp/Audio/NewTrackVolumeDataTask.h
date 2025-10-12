#pragma once

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>

#include "TaskManagement/Task.h"

/**
 * @brief This task is emmited when Short Time FFTs were generated from audio data
 * and we want the UI to display these FFTs for this track at this channel and position.
 */
class NewTrackVolumeDataTask : public SilentTask
{
  public:
    NewTrackVolumeDataTask(uint64_t _trackIdentifier, uint32_t _numChannels, uint32_t _channelIndex,
                           uint32_t _segmentStartSample, uint64_t _segmentSampleLength, float _volume,
                           uint64_t _sampleRate)
    {
        trackIdentifier = _trackIdentifier;
        totalNoChannels = _numChannels;
        channelIndex = _channelIndex;
        segmentStartSample = _segmentStartSample;
        segmentSampleLength = _segmentSampleLength;
        volume = _volume;
        sampleRate = _sampleRate;
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "new_track_volume_data_task"},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"track_identifier", trackIdentifier},
                                {"total_no_channels", totalNoChannels},
                                {"channel_index", channelIndex},
                                {"sample_rate", sampleRate},
                                {"segment_start_sample", segmentStartSample},
                                {"segment_sample_length", segmentSampleLength},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    uint64_t trackIdentifier;     /**< Identifier of the track this segment belongs to */
    uint32_t totalNoChannels;     /**< Total number of channels of this track */
    uint32_t channelIndex;        /**< Index of this specific channel data */
    uint32_t segmentStartSample;  /**< Start sample of this segment */
    uint64_t segmentSampleLength; /**< Length of the segment in samples */
    uint64_t sampleRate;
    float volume;
};