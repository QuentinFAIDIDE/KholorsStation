#include "BufferForwarder.h"
#include "AudioTransport.pb.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

#define NUM_PREALLOCATED_BLOCKINFO 16
#define NUM_PREALLOCATED_COALESCED_PAYLOADS 8
#define PREALLOCATED_BLOCKINFO_SAMPLE_SIZE 4096
#define DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE 4096

BufferForwarder::BufferForwarder()
    : freeBlockInfos(NUM_PREALLOCATED_BLOCKINFO), blockInfosToCoalesce(NUM_PREALLOCATED_BLOCKINFO)
{
    shouldStop = false;
    dawIsCompatible = true;

    // we'll fetch free blocks one by one
    freeBlockInfosFetchContainer = std::make_shared<std::vector<size_t>>();
    freeBlockInfosFetchContainer->reserve(1);

    blockInfoToCoalesceFetchContainer = std::make_shared<std::vector<size_t>>();
    blockInfoToCoalesceFetchContainer->reserve(32);

    // create the block info structs to pass to audio thread in order to fetch signal info
    preallocatedBlockInfo.resize(NUM_PREALLOCATED_BLOCKINFO);
    for (size_t i = 0; i < NUM_PREALLOCATED_BLOCKINFO; i++)
    {
        preallocatedBlockInfo[i] = std::make_shared<AudioBlockInfo>();
        preallocatedBlockInfo[i]->storageId = i;
        preallocatedBlockInfo[i]->firstChannelData.reserve(PREALLOCATED_BLOCKINFO_SAMPLE_SIZE);
        preallocatedBlockInfo[i]->secondChannelData.reserve(PREALLOCATED_BLOCKINFO_SAMPLE_SIZE);
        freeBlockInfos.queue(i);
    }

    // create the preallocated payloads to send to the station and put em in the free payloads queue
    for (size_t i = 0; i < NUM_PREALLOCATED_COALESCED_PAYLOADS; i++)
    {
        freePayloads.push(std::make_shared<AudioTransport::AudioSegmentPayload>());
    }

    // start the thread that wait for payloads
    coalescerThread = std::make_shared<std::thread>(&BufferForwarder::coalescePayloadsThreadLoop, this);
    senderThread = std::make_shared<std::thread>(&BufferForwarder::sendPayloadsThreadLoop, this);
}

BufferForwarder::~BufferForwarder()
{
    shouldStop = true;
    blockInfosToCoalesceCV.notify_one();
    coalescerThread->join();
}

std::shared_ptr<AudioBlockInfo> BufferForwarder::getFreeBlockInfoStruct()
{
    freeBlockInfos.dequeue(freeBlockInfosFetchContainer, 1);
    if (freeBlockInfosFetchContainer->size() > 0)
    {
        return preallocatedBlockInfo[(*freeBlockInfosFetchContainer)[0]];
    }
    else
    {
        return nullptr;
    }
}

void BufferForwarder::forwardAudioBlockInfo(std::shared_ptr<AudioBlockInfo> blockInfo)
{
    size_t numPushed = blockInfosToCoalesce.queue(blockInfo->storageId);
    if (numPushed == 0)
    {
        spdlog::warn("Queues of audio buffer to coalesce is full, dropped an audio buffer");
    }
    blockInfosToCoalesceCV.notify_one();
}

void BufferForwarder::setTrackInfo(uint64_t id)
{
    trackIdentifier = id;
    trackColorRed = trackIdentifier | 0x00000001;
    trackColorGreen = (trackIdentifier >> 8) | 0x00000001;
    trackColorBlue = (trackIdentifier >> 16) | 0x00000001;
    trackColorAlpha = (trackIdentifier >> 24) | 0x00000001;
    {
        std::lock_guard lock(trackNameMutex);
        trackName = std::to_string(trackIdentifier);
    }
}

void BufferForwarder::setDawIsCompatible(bool v)
{
    dawIsCompatible = v;
}

void BufferForwarder::queueCurrentlyFilledPayloadForSend()
{
    // if the payload is full, we just send it and keep iterating with a fresh new payload
    {
        std::lock_guard lockPayload(payloadsMutex);
        payloadsToSend.push(currentlyFilledPayload);
        currentlyFilledPayload = nullptr;
    }
    payloadsCV.notify_one();
}

void BufferForwarder::sendPayloadsThreadLoop()
{
    // TODO
}

void BufferForwarder::coalescePayloadsThreadLoop()
{
    while (true)
    {
        // Wait on a cv for new datums with some timeout
        std::unique_lock lock(blockInfosToCoalesceMutex);
        blockInfosToCoalesceCV.wait_for(lock, std::chrono::milliseconds(FORWARDER_THREAD_MAX_WAIT_MS));
        if (shouldStop)
        {
            return;
        }

        // try to fetch all items from the queue
        blockInfosToCoalesce.dequeue(blockInfoToCoalesceFetchContainer, 10);
        size_t queuedBlockInfoIndex = 0;

        while (payloadIsFullOrBlockInfoRemains(queuedBlockInfoIndex))
        {
            allocateCurrentlyFilledPayloadIfNecessary();

            if (payloadIsFull(currentlyFilledPayload))
            {
                queueCurrentlyFilledPayloadForSend();
                continue;
            }

            // get the content of the first queue item
            auto currentBlockInfo = preallocatedBlockInfo[(*blockInfoToCoalesceFetchContainer)[queuedBlockInfoIndex]];

            if (payloadIsEmpty(currentlyFilledPayload))
            {
                // set metadata on the payload with the one from the first block
                copyMetadataToPayload(currentlyFilledPayload, currentBlockInfo);
                // append the data to the buffer
                size_t remainingSample = appendAudioBlockToPayload(currentlyFilledPayload, currentBlockInfo);
                if (remainingSample > 0)
                {
                    // if it was not fully used, change it to ignore the already used items
                    size_t usedSamples = (size_t)currentBlockInfo->numSamples - remainingSample;
                    removeFirstUsedSamplesFromAudioBlock(currentBlockInfo, usedSamples);
                }
                else
                {
                    // if it was fully used, remove it from the queued items and keep iterating
                    queuedBlockInfoIndex++;
                    freeBlockInfos.queue(currentBlockInfo->storageId);
                }
            }
            else // this is reached if the payload is partially filled
            {
                // if the audio block info is the continuation of the payload data we keep filling
                if (audioBlockInfoFollowsPayloadContent(currentlyFilledPayload, currentBlockInfo))
                {
                    // append the data to the buffer
                    size_t remainingSample = appendAudioBlockToPayload(currentlyFilledPayload, currentBlockInfo);
                    if (remainingSample > 0)
                    {
                        // if it was not fully used, change it to ignore the already used items
                        size_t usedSamples = (size_t)currentBlockInfo->numSamples - remainingSample;
                        removeFirstUsedSamplesFromAudioBlock(currentBlockInfo, usedSamples);
                    }
                    else
                    {
                        // if it was fully used, remove it from the queued items and keep iterating
                        queuedBlockInfoIndex++;
                        freeBlockInfos.queue(currentBlockInfo->storageId);
                    }
                }
                else // if the audio block does not continue previous signal, we send payload padded with zeros
                {
                    fillPayloadRemainingSpaceWithZeros(currentlyFilledPayload);
                }
            }
        }
    }
}

void BufferForwarder::allocateCurrentlyFilledPayloadIfNecessary()
{
    if (currentlyFilledPayload == nullptr)
    {
        {
            std::lock_guard lockPayload(payloadsMutex);
            if (freePayloads.size() == 0)
            {
                currentlyFilledPayload = std::make_shared<AudioTransport::AudioSegmentPayload>();
            }
            else
            {
                currentlyFilledPayload = freePayloads.front();
                freePayloads.pop();
            }
        }

        clearPayload(currentlyFilledPayload);
    }
}

bool BufferForwarder::payloadIsFullOrBlockInfoRemains(size_t queuedBlockInfoIndex)
{
    bool blockInfoRemains = queuedBlockInfoIndex < blockInfoToCoalesceFetchContainer->size();
    bool payloadExistAndIsFull = (currentlyFilledPayload != nullptr && payloadIsFull(currentlyFilledPayload));
    return blockInfoRemains || payloadExistAndIsFull;
}

bool BufferForwarder::payloadIsEmpty(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    if (payload == nullptr)
    {
        throw std::invalid_argument("received nullptr at payloadIsEmpty");
    }
    return payload->segment_sample_duration() == 0;
}

bool BufferForwarder::payloadIsFull(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    return payload->segment_sample_duration() == DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE;
}

void BufferForwarder::clearPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    payload->mutable_segment_audio_samples()->Resize(0, 0.0f);
    payload->mutable_segment_audio_samples()->Resize(DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE * 2, 0.0f);
    payload->set_segment_sample_duration(0);
}