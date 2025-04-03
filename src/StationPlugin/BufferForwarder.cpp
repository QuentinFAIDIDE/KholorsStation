#include "BufferForwarder.h"
#include "GUIToolkit/Consts.h"
#include "GUIToolkit/Widgets/ColorPickerUpdateTask.h"
#include "GUIToolkit/Widgets/TextEntry.h"
#include "HeadlessAudioBroadcast.pb.h"
#include "HeadlessAudioBroadcast/ColorBytes.h"
#include "HeadlessAudioBroadcast/Constants.h"
#include "juce_graphics/juce_graphics.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

#define BUFFERS_CONTINUATION_SAMPLE_TOLERANCE 60

BufferForwarder::BufferForwarder(AudioTransport::AudioSegmentPayloadSender &ps)
    : payloadSender(ps), freeBlockInfos(NUM_PREALLOCATED_BLOCKINFO), blockInfosToCoalesce(NUM_PREALLOCATED_BLOCKINFO)
{
    shouldStop = false;
    dawIsCompatible = true;

    lastSucessfullPayloadUpload = juce::Time::currentTimeMillis();

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

    spdlog::debug("Terminating BufferForwarder instance");

    blockInfosToCoalesceCV.notify_one();
    coalescerThread->join();

    spdlog::debug("Joined and deleted BufferForwarder coalescer thread");

    payloadsCV.notify_one();
    senderThread->join();

    spdlog::debug("Joined and deleted BufferForwarder sender thread");
}

std::shared_ptr<AudioBlockInfo> BufferForwarder::getFreeBlockInfoStruct()
{
    freeBlockInfos.dequeue(freeBlockInfosFetchContainer, 1);
    if (freeBlockInfosFetchContainer->size() > 0)
    {
        preallocatedBlockInfo[(*freeBlockInfosFetchContainer)[0]]->numUsedSamples = 0;
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

void BufferForwarder::initializeTrackInfo(uint64_t id)
{
    trackIdentifier = id;
    std::vector<juce::Colour> choices = KHOLORS_COLOR_PALETTE;
    auto color = choices[id % choices.size()];
    trackColorRed = color.getRed();
    trackColorGreen = color.getGreen();
    trackColorBlue = color.getBlue();
    trackColorAlpha = 255;
    {
        std::lock_guard lock(trackNameMutex);
        trackName = std::to_string(trackIdentifier);
    }
}

void BufferForwarder::setDawIsCompatible(bool v)
{
    dawIsCompatible = v;
}

void BufferForwarder::coalescePayloadsThreadLoop()
{
    while (true)
    {
        // Wait on a cv for new datums with some timeout
        std::unique_lock lock(blockInfosToCoalesceMutex);
        blockInfosToCoalesceCV.wait_for(lock, std::chrono::milliseconds(FORWARDER_THREAD_MAX_WAIT_MS));
        spdlog::debug("Buffer coalesce routine woke up...");
        if (shouldStop)
        {
            spdlog::debug("Stopping the coalescer thread...");
            return;
        }

        // try to fetch all items from the queue
        blockInfosToCoalesce.dequeue(blockInfoToCoalesceFetchContainer, 10);
        size_t queuedBlockInfoIndex = 0;

        if (currentlyFilledPayload != nullptr && payloadIsOld(currentlyFilledPayload))
        {
            spdlog::debug("A payloads is too old to be kept around");
            fillPayloadRemainingSpaceWithZeros(currentlyFilledPayload);
        }

        while (payloadIsFullOrBlockInfoRemains(queuedBlockInfoIndex))
        {
            spdlog::debug("Preparing to operate on payloads to send or buffer to coalesce...");

            allocateCurrentlyFilledPayloadIfNecessary();

            if (payloadIsFull(currentlyFilledPayload))
            {
                spdlog::debug("Current payload is full");
                queueCurrentlyFilledPayloadForSend();
                continue;
            }

            // get the content of the first queue item
            auto currentBlockInfo = preallocatedBlockInfo[(*blockInfoToCoalesceFetchContainer)[queuedBlockInfoIndex]];

            if (payloadIsEmpty(currentlyFilledPayload))
            {
                spdlog::debug("Current payload is empty");
                // set metadata on the payload with the one from the first block
                copyMetadataToPayload(currentlyFilledPayload, currentBlockInfo);
                // append the data to the buffer
                size_t remainingSampleInAudioBlock =
                    appendAudioBlockToPayload(currentlyFilledPayload, currentBlockInfo);
                if (remainingSampleInAudioBlock == 0)
                {
                    spdlog::debug("Used all of the audio signal in audio block info");
                    // if it was fully used, remove it from the queued items and keep iterating
                    queuedBlockInfoIndex++;
                    freeBlockInfos.queue(currentBlockInfo->storageId);
                }
            }
            else // this is reached if the payload is partially filled
            {
                spdlog::debug("Current payload is partially filled");
                // if the audio block info is the continuation of the payload data we keep filling
                if (audioBlockInfoFollowsPayloadContent(currentlyFilledPayload, currentBlockInfo))
                {
                    spdlog::debug("Latest audio block info roughly continues current payload signal");
                    // append the data to the buffer
                    size_t remainingSampleInAudioBlock =
                        appendAudioBlockToPayload(currentlyFilledPayload, currentBlockInfo);
                    if (remainingSampleInAudioBlock == 0)
                    {
                        spdlog::debug("Used all of the audio signal in audio block info");
                        // if it was fully used, remove it from the queued items and keep iterating
                        queuedBlockInfoIndex++;
                        freeBlockInfos.queue(currentBlockInfo->storageId);
                    }
                }
                else // if the audio block does not continue previous signal, we send payload padded with zeros
                {
                    spdlog::debug("Latest audio block does not continue payload data, filling payload with zero before "
                                  "sending it");
                    fillPayloadRemainingSpaceWithZeros(currentlyFilledPayload);
                }
            }
        }
    }
}

void BufferForwarder::sendPayloadsThreadLoop()
{
    while (true)
    {
        // Wait on a cv for new datums with some timeout
        std::unique_lock lock(payloadsMutex);
        payloadsCV.wait_for(lock, std::chrono::milliseconds(FORWARDER_THREAD_MAX_WAIT_MS));
        spdlog::debug("Payload sending routine woke up...");
        if (shouldStop)
        {
            spdlog::debug("Stopping payload thread");
            return;
        }

        std::shared_ptr<AudioTransport::AudioSegmentPayload> filledPayloadToSend;
        if (payloadsToSend.size() == 0)
        {
            spdlog::debug("Not payloads are pending transfer to the station");
            continue;
        }
        filledPayloadToSend = payloadsToSend.front();
        payloadsToSend.pop();

        spdlog::debug("Got a payload to send to the station");

        filledPayloadToSend->set_payload_sent_time_unix_ms(juce::Time::currentTimeMillis());

        // send payload to the api
        bool success = payloadSender.sendAudioSegment(filledPayloadToSend.get());

        if (success)
        {
            spdlog::debug("Successfully payload to the station");
            lastSucessfullPayloadUpload = juce::Time::currentTimeMillis();
        }
        else
        {
            spdlog::debug("Failed to send payload to the station, it might not be reachable on this port.");
            if (juce::Time::currentTimeMillis() - lastSucessfullPayloadUpload > MAX_FAILURE_RECONNECT_TIME_MS)
            {
                payloadSender.tryReconnect();
            }
        }

        // release the payload so it can be reused
        freePayloads.push(filledPayloadToSend);

        spdlog::debug("Pushed payload back to free payloads queue");
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

bool BufferForwarder::payloadIsFull(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    return payload->segment_sample_duration() == AUDIO_SEGMENTS_BLOCK_SIZE;
}

bool BufferForwarder::payloadIsOld(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    int64_t currentTime = juce::Time::currentTimeMillis();
    int64_t msDiff = currentTime - payloadsFillingStartTimesMs[payload];
    return msDiff > MAX_PAYLOAD_IDLE_MS;
}

void BufferForwarder::clearPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    payload->mutable_segment_audio_samples()->Resize(0, 0.0f);
    payload->mutable_segment_audio_samples()->Resize(AUDIO_SEGMENTS_BLOCK_SIZE * 2, 0.0f);
    payload->set_segment_sample_duration(0);
    payloadsFillingStartTimesMs[payload] = juce::Time::currentTimeMillis();
}

void BufferForwarder::copyMetadataToPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload> dest,
                                            std::shared_ptr<AudioBlockInfo> src)
{
    dest->set_track_identifier(trackIdentifier);
    dest->set_track_color(
        AudioTransport::ColorContainer(trackColorRed, trackColorGreen, trackColorBlue, trackColorAlpha).toColorBytes());
    {
        std::lock_guard lock(trackNameMutex);
        dest->set_track_name(trackName);
    }

    dest->set_daw_sample_rate(src->sampleRate);
    if (src->bpm.hasValue())
    {
        dest->set_daw_bpm(*src->bpm);
    }
    else
    {
        dest->set_daw_bpm(0);
    }

    if (src->timeSignature.hasValue())
    {
        dest->set_daw_time_signature_numerator(src->timeSignature->numerator);
        dest->set_daw_time_signature_denominator(src->timeSignature->denominator);
    }
    else
    {
        dest->set_daw_time_signature_numerator(0);
        dest->set_daw_time_signature_denominator(0);
    }

    dest->set_daw_is_looping(src->isLooping);
    dest->set_daw_is_playing(src->isPlaying);
    dest->set_daw_not_supported(!dawIsCompatible);

    if (src->loopBounds.hasValue())
    {
        dest->set_daw_loop_start(src->loopBounds->ppqStart);
        dest->set_daw_loop_end(src->loopBounds->ppqEnd);
    }
    else
    {
        dest->set_daw_loop_start(0);
        dest->set_daw_loop_end(0);
    }

    dest->set_segment_start_sample(src->startSample + src->numUsedSamples);
    dest->set_segment_sample_duration(0);
    dest->set_segment_no_channels(src->numChannels);
}

size_t BufferForwarder::appendAudioBlockToPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload> dest,
                                                  std::shared_ptr<AudioBlockInfo> src)
{
    if (dest == nullptr || src == nullptr)
    {
        throw std::runtime_error("passed nullptr pointer to appendAudioBlockToPayload");
    }

    int remainingBlockInfoSamples = src->numTotalSamples - src->numUsedSamples;
    int remainingPayloadSamples = AUDIO_SEGMENTS_BLOCK_SIZE - dest->segment_sample_duration();
    int numMaxIter = std::min(remainingPayloadSamples, remainingBlockInfoSamples);

    if (remainingBlockInfoSamples < 0)
    {
        throw std::runtime_error(
            "block info passed to appendAudioBlockToPayload has numUsedSamples higher than numTotalSamples");
    }

    if (payloadIsFull(dest))
    {
        return (size_t)remainingBlockInfoSamples;
    }

    for (int chan = 0; chan < 2; chan++)
    {
        int channelShift = chan * AUDIO_SEGMENTS_BLOCK_SIZE;
        float *channelData;
        if (chan == 0 || src->numChannels < 2)
        {
            channelData = src->firstChannelData.data();
        }
        else
        {
            channelData = src->secondChannelData.data();
        }
        for (int i = 0; i < numMaxIter; i++)
        {
            dest->mutable_segment_audio_samples()->Set((uint64_t)channelShift + dest->segment_sample_duration() +
                                                           (uint64_t)i,
                                                       channelData[(size_t)src->numUsedSamples + (size_t)i]);
        }
    }

    src->numUsedSamples += numMaxIter;
    dest->set_segment_sample_duration(dest->segment_sample_duration() + (uint64_t)numMaxIter);

    if (src->numTotalSamples < src->numUsedSamples)
    {
        throw std::runtime_error("block info has read more samples than it has.");
    }

    return ((size_t)src->numTotalSamples - (size_t)src->numUsedSamples);
}

bool BufferForwarder::audioBlockInfoFollowsPayloadContent(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload,
                                                          std::shared_ptr<AudioBlockInfo> src)
{
    // NOTE: Most DAW do not store items positions in samples and approximately reconstruct it when asked. This
    // has the consequence that after processing a buffer of size N at position P, the next position might not
    // be P+N and therefore we assert that a buffer continues another with a certain tolerance.
    return std::abs((src->startSample + src->numUsedSamples) -
                    (payload->segment_start_sample() + (int64_t)payload->segment_sample_duration())) <
           BUFFERS_CONTINUATION_SAMPLE_TOLERANCE;
}

void BufferForwarder::fillPayloadRemainingSpaceWithZeros(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload)
{
    if (payload->segment_sample_duration() >= AUDIO_SEGMENTS_BLOCK_SIZE)
    {
        return;
    }
    for (int chan = 0; chan < 2; chan++)
    {
        int channelShift = chan * AUDIO_SEGMENTS_BLOCK_SIZE;

        for (size_t i = payload->segment_sample_duration(); i < AUDIO_SEGMENTS_BLOCK_SIZE; i++)
        {
            payload->mutable_segment_audio_samples()->Set((uint64_t)channelShift + i, 0.0f);
        }
    }
    payload->set_segment_sample_duration(AUDIO_SEGMENTS_BLOCK_SIZE);
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

juce::Colour BufferForwarder::getCurrentColor()
{
    return juce::Colour(trackColorRed, trackColorGreen, trackColorBlue);
}

std::string BufferForwarder::getCurrentTrackName()
{
    std::lock_guard lock(trackNameMutex);
    return trackName;
}

void BufferForwarder::setCurrentColor(juce::Colour c)
{
    trackColorRed = c.getRed();
    trackColorGreen = c.getGreen();
    trackColorBlue = c.getBlue();
}

void BufferForwarder::setCurrentTrackName(std::string s)
{
    std::lock_guard lock(trackNameMutex);
    trackName = s;
}
