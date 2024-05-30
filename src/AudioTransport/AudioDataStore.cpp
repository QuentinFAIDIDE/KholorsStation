#include "AudioDataStore.h"
#include "AudioSegment.h"
#include "AudioTransport.pb.h"
#include "DawInfo.h"
#include "TooManyRequestsException.h"
#include "TrackInfo.h"
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <vector>

namespace AudioTransport
{

AudioDataStore::AudioDataStore(size_t noAudioSegments)
{
    preallocatedBuffers.resize(noAudioSegments);
    for (size_t i = 0; i < preallocatedBuffers.size(); i++)
    {
        preallocatedBuffers[i].storageIdentifier = i;
        preallocatedBuffers[i].datum = std::make_shared<AudioSegment>();
        freePreallocatedBuffers.insert(i);
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

std::optional<AudioDataStore::AudioDatumWithStorageId> AudioDataStore::reserveAudioSegment()
{
    std::lock_guard<std::mutex> lock(preallocatedBuffersMutex);
    if (freePreallocatedBuffers.size() == 0)
    {
        return std::nullopt;
    }
    auto storageIdentifier = *freePreallocatedBuffers.begin();
    freePreallocatedBuffers.erase(storageIdentifier);
    return preallocatedBuffers[storageIdentifier];
}

void AudioDataStore::freeStoredDatum(uint64_t storageIndentifier)
{
    std::lock_guard<std::mutex> lock(preallocatedBuffersMutex);
    freePreallocatedBuffers.insert(storageIndentifier);
}

void AudioDataStore::parseNewData(AudioSegmentPayload *payload)
{
    if (payload == nullptr)
    {
        throw std::runtime_error("called parseNewData with nullptr payload");
    }

    DawInfo dawInfo;
    TrackInfo trackInfo;

    auto payloadAudioBuffers = extractPayloadAudioSegments(payload);
    for (size_t i = 0; i < payloadAudioBuffers.size(); i++)
    {
        pushAudioDatumToQueue(payloadAudioBuffers[i]);
    }

    dawInfo.parseFromApiPayload(payload);
    trackInfo.parseFromApiPayload(payload);

    if (lastDawInfo != dawInfo)
    {
        AudioDatumWithStorageId dawInfoUpdate;
        // TODO: preallocate these too instead of allocating on the heap
        dawInfoUpdate.datum = std::make_shared<DawInfo>(dawInfo);
        dawInfoUpdate.storageIdentifier = -1;
        pushAudioDatumToQueue(dawInfoUpdate);
    }

    auto trackInfoFound = trackInfoByIdentifier.find(trackInfo.identifier);
    if (trackInfoFound == trackInfoByIdentifier.end())
    {
        AudioDatumWithStorageId trackInfoUpdate;
        // TODO: preallocate these too instead of allocating on the heap
        trackInfoByIdentifier.insert(std::pair<uint64_t, TrackInfo>(trackInfo.identifier, trackInfo));
        trackInfoUpdate.datum = std::make_shared<TrackInfo>(trackInfo);
        trackInfoUpdate.storageIdentifier = -1;
        pushAudioDatumToQueue(trackInfoUpdate);
    }
}

void AudioDataStore::pushAudioDatumToQueue(AudioDatumWithStorageId datum)
{
    {
        std::lock_guard<std::mutex> lock(pendingAudioDataMutex);
        pendingAudioData.emplace(datum);
    }
    pendingAudioDataCondVar.notify_one();
}

std::vector<AudioDataStore::AudioDatumWithStorageId> AudioDataStore::extractPayloadAudioSegments(
    AudioSegmentPayload *payload)
{
    if (payload == nullptr)
    {
        throw std::runtime_error("called extractPayloadAudioSegments with nullptr payload");
    }

    std::vector<AudioDatumWithStorageId> extractedAudioBuffers;

    // this block eventually generate audio segments
    if (payload->segment_sample_duration() > 0)
    {
        // if the audio data size matches segment lenght, we generate a segment
        if (payload->segment_audio_samples().size() == payload->segment_sample_duration())
        {

            for (size_t i = 0; i < payload->segment_no_channels(); i++)
            {
                auto optionalBuffer = reserveAudioSegment();
                if (optionalBuffer.has_value())
                {
                    std::shared_ptr<AudioSegment> segment =
                        std::dynamic_pointer_cast<AudioSegment>(optionalBuffer->datum);
                    segment->parseFromApiPayload(payload, i);
                    extractedAudioBuffers.push_back(*optionalBuffer);
                }
                else
                {
                    throw TooManyRequestsException("No more preallocated gRPC api storage audio segment buffers.");
                }
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

    return extractedAudioBuffers;
}

} // namespace AudioTransport