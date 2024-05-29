#ifndef DEF_AUDIO_SEGMENT_HPP
#define DEF_AUDIO_SEGMENT_HPP

#include "AudioTransportData.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace AudioTransport
{

typedef std::shared_ptr<std::vector<float>> AudioBufferVector;

/**
 * @brief Holds a segment of audio samples as well
 * as identification information.
 */
struct AudioSegment : public AudioTransportData
{
    AudioSegment();

    AudioBufferVector audioSamples; /**< buffer of AUDIO_SEGMENTS_BLOCK_SIZE audio samples of the
                             track channel (used size is noAudioSamples) */
    uint64_t trackIdentifier;       /**< Identifier of the track the data comes from */
    uint32_t channel;               /**< Index of the channel the data comes from */
    uint32_t sampleRate;            /**< Sample rate of the data */
    uint32_t segmentStartSample;    /**< Start sample of the audio segment position */
    uint64_t noAudioSamples;        /**< How many audio samples there are in this audio segment */
};

/**
 * @brief Holds information about a near empty audio data segment that we don't need to process.
 */
struct AudioSegmentNoOp : public AudioTransportData
{
    uint64_t trackIdentifier; /**< Identifier of the track the data comes from */
    uint32_t channel;         /**< Index of the channel the data comes from */
    uint32_t sampleRate;      /**< Sample rate of the data */
    uint64_t noAudioSamples;  /**< How many audio samples there are in this audio segment */
};

} // namespace AudioTransport

#endif // DEF_AUDIO_SEGMENT_HPP