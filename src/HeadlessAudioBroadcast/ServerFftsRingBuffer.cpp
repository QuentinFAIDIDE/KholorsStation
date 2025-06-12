#include "ServerFftsRingBuffer.h"
#include "HeadlessAudioBroadcast.pb.h"
#include <cstdint>
#include <cstring>
#include <limits>
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

int ServerFftsRingBuffer::writeItem(double bpm, int32_t timeSignatureNumerator, int32_t timeSignatureDenominator,
                                    int64_t trackidentifier, std::string trackName, int32_t trackColor,
                                    uint32_t noChannels, uint32_t channelIndex, uint32_t sampleRate,
                                    uint32_t segmentStartSample, uint64_t segmentSampleLength, int64_t sentTimeUnixMs,
                                    uint32_t noFfts, std::shared_ptr<std::vector<float>> fftData)
{
    std::lock_guard lock(mutex);

    // we abort writing nothing if the offset is about to overflow
    if (fftToDeliverLastOffset >= (std::numeric_limits<uint64_t>::max() - 1))
    {
        return 0;
    }
    // if not overflowing we increase the last offset used (that we will use for this datum)
    fftToDeliverLastOffset++;

    // eventually update size of ring buffer if not full yet
    if (fftsToDeliverUsedSize != maxSize)
    {
        fftsToDeliverUsedSize++;
    }
    // we either wrap to 0 if we are at the end of the ring, or increment to next datum
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
    return 1;
}

std::shared_ptr<AudioTasks> ServerFftsRingBuffer::readItems(uint64_t requestServerIdentifier, uint64_t offset)
{
    std::lock_guard lock(mutex);

    uint64_t offsetToReadFrom = offset;
    if (serverIdentifier != requestServerIdentifier)
    {
        offsetToReadFrom = 1;
    }

    if (freeAudioTasksStructs.size() == 0)
    {
        createAudioTasksResponseStruct();
    }

    auto taskListToPopulate = freeAudioTasksStructs.front();
    freeAudioTasksStructs.pop();

    // BUG: on overflow, we re-read the last ones in a loop untill the server switch
    // to the newly elected server takes over. Since the fft reads are done baxsed on
    // time interval, we will redraw the same last fft untill the new server takes over.
    uint64_t nextOffset;
    if (fftToDeliverLastOffset >= (std::numeric_limits<uint64_t>::max() - 1))
    {
        nextOffset = fftToDeliverLastOffset;
    }
    else
    {
        nextOffset = fftToDeliverLastOffset + 1;
    }

    // The challenge here is to find a way to modify the fft to draw tasks array
    // members without reallocating shit unless if necessary. It should work, to verify
    // at profiling time.
    taskListToPopulate->set_new_offset(nextOffset);
    taskListToPopulate->set_server_identifier(serverIdentifier);
    taskListToPopulate->mutable_fft_to_draw_tasks()->Clear();

    // handle the case where there is no FFT result to deliver yet
    if (fftsToDeliverUsedSize == 0)
    {
        return taskListToPopulate;
    }

    offsetToReadFrom = std::max(fftToDeliverLastOffset - (fftsToDeliverUsedSize - 1), offsetToReadFrom);
    while (offsetToReadFrom <= fftToDeliverLastOffset)
    {
        auto newFftTask = taskListToPopulate->mutable_fft_to_draw_tasks()->Add();
        auto taskToCopyFrom = getFftAtOffset(offsetToReadFrom);
        newFftTask->CopyFrom(*taskToCopyFrom);
        // Can offsetToReadFrom overflow ?
        // Since we check that fftToDeliverLastOffset is at maximum max_uint64-1,
        // offsetToReadFrom will reach max_uint64 and loop will stop and not increment it more
        offsetToReadFrom++;
    }

    return taskListToPopulate;
}

FftToDrawTask *getFftAtOffset(uint64_t offset)
{
    // TODO: replace with a conditional pointer increment rather than recomputing the index every time ?
}

void ServerFftsRingBuffer::reuseAudioTasksStruct(std::shared_ptr<AudioTasks> structToReuse)
{
    std::lock_guard lock(mutex);
    freeAudioTasksStructs.push(structToReuse);
}

} // namespace HeadlessAudioBroadcast