#pragma once

#include "HeadlessAudioBroadcast.pb.h"
#include <cstddef>
#include <queue>

namespace HeadlessAudioBroadcast
{

class ServerFftsRingBuffer
{
  public:
    /**
     * @brief Construct a new Server Ring Buffer object
     *
     * @param size the maximum number of stored messages
     * and also the maximum size of return FFT stored.
     * @param defaultFftArraysSize size reserved for the fft_data
     * arrays of each FftToDrawTask result struct.
     */
    ServerFftsRingBuffer(size_t size, size_t defaultFftArraysSize);
    ~ServerFftsRingBuffer();

    /**
     * @brief Clear the ring buffer and deallocate all memory allocated.
     */
    void clear();

    /**
     * @brief Preallocates the ring buffer with (size constructor param) FftToDrawTask
     * that get their fftData array allocated floatArraySize floats for ffts results.
     */
    void prepare();

    /**
     * @brief Push a new processed FFT into the storage so that it can be delivered
     * to clients.
     */
    void writeItem(double bpm, int32_t timeSignatureNumerator, int32_t timeSignatureDenominator,
                   int64_t trackidentifier, std::string trackName, int32_t trackColor, uint32_t noChannels,
                   uint32_t channelIndex, uint32_t sampleRate, uint32_t segmentStartSample,
                   uint64_t segmentSampleLength, int64_t sentTimeUnixMs, uint32_t noFfts,
                   std::shared_ptr<std::vector<float>> fftData);

    /**
     * @brief Get a pointer to an audioTasks list to return to readers, that summarize
     * a list of fft to draw from the offset required, and return the new offset after reading.
     *
     * @param serverIdentifier identifier of this server, used to avoid requesting foreign offset after
     * a different plugin is elected as master. If different, will basically read from oldest available FFT.
     * @param offset offset to start reading from, last offset returned by the server in the previous req.
     * @return AudioTasks* List of FFT from the offset, as well as next offset to request to get the sequence,
     * and server identifier. Return nullptr if nothing new to return.
     */
    std::shared_ptr<AudioTasks> readItems(uint64_t serverIdentifier, uint64_t offset);

    /**
     * @brief Use the passed structure to populate the respond to readItems.
     *
     * @param structToReuse
     */
    void reuseAudioTasksStruct(std::shared_ptr<AudioTasks> structToReuse);

  private:
    /**
     * @brief Create a Audio Tasks Response Struct object and add it to the queue
     * of freeds response structs. Should be called with the mutex taken.
     */
    void createAudioTasksResponseStruct();

    /**
     * @brief get fftToDrawTask at the specified offset.
     * Must be called under the lock.
     */
    FftToDrawTask *getFftAtOffset(uint64_t offset) const;

    std::vector<FftToDrawTask> fftsToDeliver; /**< the actual ring buffer */
    size_t fftsToDeliverUsedSize;             /**< size of the fftsToDeliver actually used */
    size_t fftToDeliverLastIndex;             /**< index of last value in fftsToDeliver ring buffer */
    uint64_t fftToDeliverLastOffset;          /**< last offset at the fftToDeliverLastIndex value */
    uint64_t serverIdentifier;                /**< random indentifier to prevent using offset from other server */
    std::queue<std::shared_ptr<AudioTasks>> freeAudioTasksStructs; /**< structs to be used as responses */
    size_t maxSize;                                                /**< max number of structs to store and deliver */
    std::mutex mutex;
    size_t fftArraysSize; /**< size to preallocate the fft arrays */
};

}; // namespace HeadlessAudioBroadcast