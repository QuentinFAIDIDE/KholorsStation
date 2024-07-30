#pragma once

#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>

#include "TaskManagement/Task.h"

/**
 * @brief This task is emmited when Short Time FFTs were generated from audio data
 * and we want the UI to display these FFTs for this track at this channel and position.
 */
class NewFftDataTask : public SilentTask
{
  public:
    NewFftDataTask(uint64_t _trackIdentifier, uint32_t _noChannels, uint32_t _channelIndex, uint32_t _sampleRate,
                   uint32_t _segmentStartSample, uint64_t _segmentSampleLength, uint32_t _noFFTs,
                   std::shared_ptr<std::vector<float>> _data, int64_t _sentTimeUnixMs)
    {
        trackIdentifier = _trackIdentifier;
        totalNoChannels = _noChannels;
        channelIndex = _channelIndex;
        sampleRate = _sampleRate;
        segmentStartSample = _segmentStartSample;
        segmentSampleLength = _segmentSampleLength;
        noFFTs = _noFFTs;
        fftData = _data;
        sentTimeUnixMs = _sentTimeUnixMs;
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "new_fft_data_task"},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"track_identifier", trackIdentifier},
                                {"total_no_channels", totalNoChannels},
                                {"channel_index", channelIndex},
                                {"sample_rate", sampleRate},
                                {"segment_start_sample", segmentStartSample},
                                {"segment_sample_length", segmentSampleLength},
                                {"no_ffts", noFFTs},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    uint64_t trackIdentifier;                    /**< Identifier of the track this segment belongs to */
    uint32_t totalNoChannels;                    /**< Total number of channels of this track */
    uint32_t channelIndex;                       /**< Index of this specific channel data */
    uint32_t sampleRate;                         /**< Sample rate of this segment */
    uint32_t segmentStartSample;                 /**< Start sample of this segment */
    uint64_t segmentSampleLength;                /**< Length of the segment in samples */
    int64_t sentTimeUnixMs;                      /**< time at which the plugin sent the payload */
    uint32_t noFFTs;                             /**< Number of FFTs generated for this segment */
    std::shared_ptr<std::vector<float>> fftData; /**< raw FFT result in dBs (noFFTs FFTs of len FFT_OUTPUT_NO_FREQS) */
};