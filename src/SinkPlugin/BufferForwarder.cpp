#include "BufferForwarder.h"
#include "AudioTransport.pb.h"
#include "Utils/NoAllocIndexQueue.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>

#define NUM_PREALLOCATED_BLOCKINFO 16
#define NUM_PREALLOCATED_COALESCED_PAYLOADS 8
#define PREALLOCATED_BLOCKINFO_SAMPLE_SIZE 4096
#define DEFAULT_AUDIO_SEGMENT_SIZE 8192

BufferForwarder::BufferForwarder()
    : freeBlockInfos(NUM_PREALLOCATED_BLOCKINFO), blockInfosToCoalesce(NUM_PREALLOCATED_BLOCKINFO)
{
    shouldStop = false;

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
        // While queue items remains
        while (queuedBlockInfoIndex < blockInfoToCoalesceFetchContainer->size())
        {
            // if the currently filled payload is unset, pick a fresh one or allocate it
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

            // get the content of the first queue item
            auto currentBlockInfo = preallocatedBlockInfo[(*blockInfoToCoalesceFetchContainer)[queuedBlockInfoIndex]];

            // if the payload struct to fill is empty
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
                    // we will return to the loop beginning so that the payload is sent
                }
                else
                {
                    // if it was fully used, remove it from the queued items and keep iterating
                    queuedBlockInfoIndex++;
                    freeBlockInfos.queue(currentBlockInfo->storageId);
                }
            }
            else if (payloadIsFull(currentlyFilledPayload))
            {
                // if the payload is full, we just send it and keep iterating with a fresh new payload
                {
                    std::lock_guard lockPayload(payloadsMutex);
                    payloadsToSend.push(currentlyFilledPayload);
                    currentlyFilledPayload = nullptr;
                }
                payloadsCV.notify_one();
            }
            else // this is reached if the payload is partially filled
            {
                // TODO: if the audio block info is the continuation of the payload data we keep filling
                // TODO: if the audio block does not continue previous signal, we send payload padded with zeros
            }
        }
    }
}

bool BufferForwarder::payloadIsEmpty(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    if (payload == nullptr)
    {
        throw std::invalid_argument("received nullptr at payloadIsEmpty");
    }
    return payload->segment_sample_duration() == 0;
}

void BufferForwarder::clearPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    payload->mutable_segment_audio_samples()->Resize(DEFAULT_AUDIO_SEGMENT_SIZE, 0.0f);
    payload->set_segment_sample_duration(0);
}