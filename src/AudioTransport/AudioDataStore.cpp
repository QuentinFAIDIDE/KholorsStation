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

AudioDataStore::AudioDataStore(size_t noAllocatedStructs) : noPreallocatedStructs(noAllocatedStructs)
{
    preallocatedAudioSegments.resize(noAllocatedStructs);
    for (size_t i = 0; i < preallocatedAudioSegments.size(); i++)
    {
        preallocatedAudioSegments[i].storageIdentifier = i;
        preallocatedAudioSegments[i].datum = std::make_shared<AudioSegment>();
        freeAudioSegments.insert(i);
    }

    preallocatedDawInfo.resize(noAllocatedStructs);
    for (size_t i = 0; i < preallocatedDawInfo.size(); i++)
    {
        size_t dawIndex = noAllocatedStructs + i;
        preallocatedDawInfo[i].storageIdentifier = dawIndex;
        preallocatedDawInfo[i].datum = std::make_shared<DawInfo>();
        freeDawInfo.insert(i);
    }

    preallocatedTrackInfo.resize(noAllocatedStructs);
    for (size_t i = 0; i < preallocatedTrackInfo.size(); i++)
    {
        size_t trackIndex = (noAllocatedStructs * 2) + i;
        preallocatedTrackInfo[i].storageIdentifier = trackIndex;
        preallocatedTrackInfo[i].datum = std::make_shared<TrackInfo>();
        freeTrackInfo.insert(i);
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
    std::lock_guard<std::mutex> lock(preallocatedAudioSegmentsMutex);
    if (freeAudioSegments.size() == 0)
    {
        return std::nullopt;
    }
    auto storageIdentifier = *freeAudioSegments.begin();
    freeAudioSegments.erase(storageIdentifier);
    return preallocatedAudioSegments[storageIdentifier];
}

std::optional<AudioDataStore::AudioDatumWithStorageId> AudioDataStore::reserveDawInfo()
{
    std::lock_guard<std::mutex> lock(preallocatedDawInfoMutex);
    if (freeDawInfo.size() == 0)
    {
        return std::nullopt;
    }
    auto storageIdentifier = *freeDawInfo.begin();
    freeDawInfo.erase(storageIdentifier);
    return preallocatedDawInfo[storageIdentifier];
}

std::optional<AudioDataStore::AudioDatumWithStorageId> AudioDataStore::reserveTrackInfo()
{
    std::lock_guard<std::mutex> lock(preallocatedTrackInfoMutex);
    if (freeTrackInfo.size() == 0)
    {
        return std::nullopt;
    }
    auto storageIdentifier = *freeTrackInfo.begin();
    freeTrackInfo.erase(storageIdentifier);
    return preallocatedTrackInfo[storageIdentifier];
}

void AudioDataStore::freeStoredDatum(uint64_t storageIndentifier)
{
    if (storageIndentifier >= 3 * noPreallocatedStructs || storageIndentifier < 0)
    {
        throw std::invalid_argument("storageidentifier out of range in freeStoredDatum");
    }

    if (storageIndentifier >= 2 * noPreallocatedStructs)
    {
        // this is a track info
        int i = storageIndentifier - (2 * noPreallocatedStructs);
        std::lock_guard<std::mutex> lock(preallocatedTrackInfoMutex);
        freeTrackInfo.insert(i);
    }
    else if (storageIndentifier >= noPreallocatedStructs)
    {
        // this is a daw info
        int i = storageIndentifier - noPreallocatedStructs;
        std::lock_guard<std::mutex> lock(preallocatedDawInfoMutex);
        freeDawInfo.insert(i);
    }
    else
    {
        // this is an audio segment
        std::lock_guard<std::mutex> lock(preallocatedAudioSegmentsMutex);
        freeAudioSegments.insert(storageIndentifier);
    }
}

std::vector<size_t> AudioDataStore::countFreePreallocatedStructs()
{
    std::vector<size_t> freeStructsCount(3);
    freeStructsCount[0] = freeAudioSegments.size();
    freeStructsCount[1] = freeDawInfo.size();
    freeStructsCount[2] = freeTrackInfo.size();
    return freeStructsCount;
}

void AudioDataStore::parseNewData(const AudioSegmentPayload *payload)
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
        lastDawInfo = dawInfo;
        auto optionalDawInfo = reserveDawInfo();
        if (!optionalDawInfo.has_value())
        {
            throw TooManyRequestsException("No more preallocated gRPC api storage daw info buffers.");
        }
        else
        {
            *std::dynamic_pointer_cast<DawInfo>(optionalDawInfo->datum) = dawInfo;
            pushAudioDatumToQueue(*optionalDawInfo);
        }
    }

    auto trackInfoFound = trackInfoByIdentifier.find(trackInfo.identifier);
    if (trackInfoFound == trackInfoByIdentifier.end() || trackInfoFound->second != trackInfo)
    {
        // we update or insert the new value
        trackInfoByIdentifier[trackInfo.identifier] = trackInfo;

        // we then reserve the buffer and push it to the queue for listeners (server threads) to pick it
        auto optionalTrackInfo = reserveTrackInfo();
        if (!optionalTrackInfo.has_value())
        {
            throw TooManyRequestsException("No more preallocated gRPC api storage track info buffers.");
        }
        else
        {
            *std::dynamic_pointer_cast<TrackInfo>(optionalTrackInfo->datum) = trackInfo;
            pushAudioDatumToQueue(*optionalTrackInfo);
        }
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
    const AudioSegmentPayload *payload)
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
        if (payload->segment_audio_samples().size() / payload->segment_no_channels() ==
            payload->segment_sample_duration())
        {

            for (size_t i = 0; i < payload->segment_no_channels(); i++)
            {
                auto optionalBuffer = reserveAudioSegment();
                if (optionalBuffer.has_value())
                {
                    auto segment = std::dynamic_pointer_cast<AudioSegment>(optionalBuffer->datum);
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