#include "ServerFftsRingBuffer.h"
#include "HeadlessAudioBroadcast.pb.h"
#include <cstring>
#include <memory>
#include <mutex>

namespace HeadlessAudioBroadcast
{

ServerFftsRingBuffer::ServerFftsRingBuffer(size_t size, size_t defaultFftArraysSize)
    : maxSize(size), fftArraysSize(defaultFftArraysSize)
{
    srand(time(0));
    serverIdentifier = rand();

    fftsToDeliverUsedSize = 0;
    fftToDeliverLastIndex = -1;
    fftToDeliverLastOffset = 0;
}

ServerFftsRingBuffer::~ServerFftsRingBuffer()
{
}

void ServerFftsRingBuffer::clear()
{
    std::lock_guard lock(mutex);

    std::queue<std::shared_ptr<AudioTasks>> newFreeResponseQueue;
    freeAudioTasksStructs.swap(newFreeResponseQueue);

    std::vector<FftToDrawTask> newFftToDeliver;
    fftsToDeliver.swap(newFftToDeliver);
    fftsToDeliverUsedSize = 0;
}

void ServerFftsRingBuffer::prepare()
{
    std::lock_guard lock(mutex);

    fftsToDeliver.resize(maxSize);
    for (size_t i = 0; i < fftsToDeliver.size(); i++)
    {
        fftsToDeliver[i].mutable_fft_data()->Reserve(fftArraysSize);
    }
    fftsToDeliverUsedSize = 0;
}

void ServerFftsRingBuffer::createAudioTasksResponseStruct()
{
    // note that this is called with the lock on
    auto newResponseAudioTasks = std::make_shared<AudioTasks>();
    for (size_t i = 0; i < maxSize; i++)
    {
        FftToDrawTask *newBufferedFft = new FftToDrawTask;
        newBufferedFft->mutable_fft_data()->Reserve(fftArraysSize);
        newResponseAudioTasks->mutable_fft_to_draw_tasks()->AddCleared(newBufferedFft);
    }
    freeAudioTasksStructs.push(newResponseAudioTasks);
}

void ServerFftsRingBuffer::writeItem(double bpm, int32_t timeSignatureNumerator, int32_t timeSignatureDenominator,
                                     int64_t trackidentifier, std::string trackName, int32_t trackColor,
                                     uint32_t noChannels, uint32_t channelIndex, uint32_t sampleRate,
                                     uint32_t segmentStartSample, uint64_t segmentSampleLength, int64_t sentTimeUnixMs,
                                     uint32_t noFfts, std::shared_ptr<std::vector<float>> fftData)
{
    std::lock_guard lock(mutex);

    // if the array is not full, resize it and increase offset and index of last element
    if (fftsToDeliverUsedSize != maxSize)
    {
        fftsToDeliverUsedSize++;
    }
    // shift the offset and index(modulo size) of last element
    fftToDeliverLastOffset++; // TODO: handle max offset
    if (fftToDeliverLastIndex == maxSize - 1)
    {
        fftToDeliverLastIndex = 0;
    }
    else
    {
        fftToDeliverLastIndex = fftToDeliverLastIndex + 1;
    }

    // write the item to the last index
    fftsToDeliver[fftToDeliverLastIndex].set_daw_bpm(bpm);
    fftsToDeliver[fftToDeliverLastIndex].set_daw_time_signature_numerator(timeSignatureNumerator);
    fftsToDeliver[fftToDeliverLastIndex].set_daw_time_signature_denominator(timeSignatureDenominator);
    fftsToDeliver[fftToDeliverLastIndex].set_track_identifier(trackidentifier);
    fftsToDeliver[fftToDeliverLastIndex].set_track_name(trackName);
    fftsToDeliver[fftToDeliverLastIndex].set_track_color(trackColor);
    fftsToDeliver[fftToDeliverLastIndex].set_total_no_channels(noChannels);
    fftsToDeliver[fftToDeliverLastIndex].set_channel_index(channelIndex);
    fftsToDeliver[fftToDeliverLastIndex].set_sample_rate(sampleRate);
    fftsToDeliver[fftToDeliverLastIndex].set_segment_start_sample(segmentStartSample);
    fftsToDeliver[fftToDeliverLastIndex].set_segment_sample_length(segmentSampleLength);
    fftsToDeliver[fftToDeliverLastIndex].set_sent_time_unix_ms(sentTimeUnixMs);
    fftsToDeliver[fftToDeliverLastIndex].set_no_ffts(noFfts);
    fftsToDeliver[fftToDeliverLastIndex].mutable_fft_data()->Resize(fftData->size(), 0.0f);
    float *dest = fftsToDeliver[fftToDeliverLastIndex].mutable_fft_data()->mutable_data();
    float *src = fftData->data();
    memcpy(dest, src, sizeof(float) * fftData->size());
}

std::shared_ptr<AudioTasks> ServerFftsRingBuffer::readItems(uint64_t requestServerIdentifier, uint64_t offset)
{
    std::lock_guard lock(mutex);

    uint64_t offsetToReadFrom = offset;
    if (serverIdentifier != requestServerIdentifier || offset > fftToDeliverLastOffset)
    {
        offsetToReadFrom = 1;
    }

    if (freeAudioTasksStructs.size() == 0)
    {
        createAudioTasksResponseStruct();
    }

    auto taskListToPopulate = freeAudioTasksStructs.front();
    freeAudioTasksStructs.pop();

    // TODO: handle max offset ?

    // The mission here is to find a way to modify the fft to draw tasks array
    // members without reallocating shit unless if necessary.
    taskListToPopulate->set_new_offset(fftToDeliverLastOffset + 1);
    taskListToPopulate->set_server_identifier(serverIdentifier);
    taskListToPopulate->mutable_fft_to_draw_tasks()->Clear();

    // handle the case where fftsToDeliverUsedSize == 0
    if (fftsToDeliverUsedSize == 0)
    {
        return taskListToPopulate;
    }

    offsetToReadFrom = std::max(fftToDeliverLastOffset - (fftsToDeliverUsedSize - 1), offsetToReadFrom);
    size_t fftRingBufIndex = getRingIndexFromOffset(offsetToReadFrom);
    while (offsetToReadFrom <= fftToDeliverLastOffset)
    {
        if (fftRingBufIndex >= maxSize)
        {
            fftRingBufIndex = 0;
        }
        auto newFftTask = taskListToPopulate->mutable_fft_to_draw_tasks()->Add();
<<<<<<< HEAD
        newFftTask->CopyFrom(fftsToDeliver[fftRingBufIndex]);
        // Can offsetToReadFrom overflow ?
        // Since we check that fftToDeliverLastOffset is at maximum max_uint64-1,
        // offsetToReadFrom will reach max_uint64 and loop will stop and not increment it more
=======
        auto taskToCopyFrom = getFftAtOffset(offsetToReadFrom);
        newFftTask->CopyFrom(*taskToCopyFrom);
>>>>>>> parent of ec600b6 (feat: ServerFftsRingBuffer)
        offsetToReadFrom++;
        fftRingBufIndex++;
    }

    return taskListToPopulate;
}

size_t ServerFftsRingBuffer::getRingIndexFromOffset(uint64_t offset) const
{
<<<<<<< HEAD
    int diff = fftToDeliverLastOffset - offset;
    int start = fftToDeliverLastIndex - diff;
    while (start < 0)
    {
        start += maxSize;
    }
    return start;
=======
    // TODO
>>>>>>> parent of ec600b6 (feat: ServerFftsRingBuffer)
}

void ServerFftsRingBuffer::reuseAudioTasksStruct(std::shared_ptr<AudioTasks> structToReuse)
{
    std::lock_guard lock(mutex);
    freeAudioTasksStructs.push(structToReuse);
}

} // namespace HeadlessAudioBroadcast