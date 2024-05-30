#include "AudioSegment.h"
#include <stdexcept>

namespace AudioTransport
{

void AudioSegment::parseFromApiPayload(AudioSegmentPayload *payload, size_t channel)
{
    if (payload == nullptr)
    {
        throw std::runtime_error("parseFromApiPayload received nullptr payload");
    }

    if (payload->segment_audio_samples().size() != payload->segment_sample_duration())
    {
        throw std::invalid_argument(
            "parseFromApiPayload called when segment_audio_samples has different size than segment_sample_duration");
    }

    if (channel >= payload->segment_no_channels())
    {
        throw std::invalid_argument("parseFromApiPayload called with invalid channel");
    }

    trackIdentifier = payload->track_identifier();
    channel = channel;
    sampleRate = payload->daw_sample_rate();
    segmentStartSample = payload->segment_start_sample();
    noAudioSamples = payload->segment_sample_duration();

    for (size_t i = 0; i < payload->segment_sample_duration(); i++)
    {
        audioSamples[i] = payload->segment_audio_samples()[i];
    }
}

} // namespace AudioTransport