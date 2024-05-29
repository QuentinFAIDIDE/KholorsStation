#include "AudioDataStore.h"
#include "AudioTransport.pb.h"
#include "TooManyRequestsException.h"
#include <condition_variable>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace AudioTransport
{

AudioDataStore::AudioDataStore(size_t noAudioSegments)
{
    preallocatedAudioSegments.resize(noAudioSegments);
    for (size_t i = 0; i < preallocatedAudioSegments.size(); i++)
    {
        preallocatedAudioSegments[i].storageIdentifier = i;
    }
}

std::optional<AudioDataStore::AudioDatumWithStorageId> AudioDataStore::waitForDatum()
{
    std::unique_lock<std::mutex> lock(pendingAudioDataMutex);
    auto predicate = [this] { return pendingAudioData.size() > 0; };
    bool result = pendingAudioDataCondVar.wait_for(lock, std::chrono::seconds(1), predicate);
    if (result)
    {
        auto datum = pendingAudioData.front();
        pendingAudioData.pop();
        return datum;
    }
    else
    {
        return std::nullopt;
    }
}

void AudioDataStore::parseNewData(AudioSegmentPayload *payload)
{
    if (payload == nullptr)
    {
        throw std::runtime_error("called parseNewData with nullptr payload");
    }

    auto payloadSegment = extractPayloadAudioSegment(payload);
}

std::optional<AudioDataStore::AudioDatumWithStorageId> AudioDataStore::extractPayloadAudioSegment(
    AudioSegmentPayload *payload)
{
    if (payload == nullptr)
    {
        throw std::runtime_error("called extractPayloadAudioSegment with nullptr payload");
    }

    // this block eventually generate audio segments
    if (payload->segment_sample_duration() > 0)
    {
        // if the audio data size matches segment lenght, we generate a segment
        if (payload->segment_audio_samples().size() == payload->segment_sample_duration())
        {
            auto optionalBuffer = reserveAudioSegment();
            if (optionalBuffer.has_value())
            {
                // TODO: assign and return the optional buffer
            }
            else
            {
                throw TooManyRequestsException("No more preallocated gRPC api storage audio segment buffers.");
            }
        }
        else if (payload->segment_audio_samples().size() != 0)
        {
            throw std::invalid_argument("number of audio samples differ from segment size");
        }
        // if the length is zero then by protocol, it means segment
        //  signal is near zero and we won't generate a segment
        // but if the length is different than samples
    }

    return std::nullopt;
}

} // namespace AudioTransport