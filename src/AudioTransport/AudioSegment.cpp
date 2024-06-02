#include "AudioSegment.h"
#include <stdexcept>

namespace AudioTransport
{

AudioSegment::AudioSegment()
{
}

void AudioSegment::parseFromApiPayload(const AudioSegmentPayload *payload, size_t channelPicked)
{
    if (payload == nullptr)
    {
        throw std::runtime_error("parseFromApiPayload received nullptr payload");
    }

    if (payload->segment_audio_samples().size() / payload->segment_no_channels() != payload->segment_sample_duration())
    {
        throw std::invalid_argument(
            "parseFromApiPayload called when segment_audio_samples has different size than segment_sample_duration");
    }

    if (channel >= payload->segment_no_channels())
    {
        throw std::invalid_argument("parseFromApiPayload called with invalid channel");
    }

    trackIdentifier = payload->track_identifier();
    channel = channelPicked;
    noChannels = payload->segment_no_channels();
    sampleRate = payload->daw_sample_rate();
    segmentStartSample = payload->segment_start_sample();
    noAudioSamples = payload->segment_sample_duration();

    auto payloadAudioSamples = payload->segment_audio_samples();

    for (size_t i = 0; i < payload->segment_sample_duration(); i++)
    {
        audioSamples[i] = payloadAudioSamples[(channel * noAudioSamples) + i];
    }
}

} // namespace AudioTransport