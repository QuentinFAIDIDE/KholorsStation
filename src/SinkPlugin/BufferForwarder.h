#pragma once

#include "AudioTransport.pb.h"
#include "SinkPlugin/LockFreeFIFO.h"
#include "Utils/NoAllocIndexQueue.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "AudioBlockInfo.h"

#define FORWARDER_THREAD_MAX_WAIT_MS 100

class BufferForwarder
{
  public:
    BufferForwarder();
    ~BufferForwarder();

    /**
     * @brief Get a preallocated BlockInfo struct to copy audio block data into;
     *
     * @return std::shared_ptr<AudioBlockInfo> a shared_ptr with the nonce value set where
     * audio block data should be copied.
     * It should not be used anymore after forwardAudioBlockInfo is called.
     */
    std::shared_ptr<AudioBlockInfo> getFreeBlockInfoStruct();

    /**
     * @brief Pass the AudioBlockInfo onto the thread responsible for shipping audio data to the Station.
     * @param blockInfo collection of daw and buffer info to be shipped to the station, that will be
     * considered not used anymore by the called (audio thread) and will be reused in getFreeBlockInfoStruct once
     * processed.
     */
    void forwardAudioBlockInfo(std::shared_ptr<AudioBlockInfo> blockInfo);

    /**
     * @brief Set the track identifier this forwarder is responsible for.
     *
     * @param trackIdentifier track identifier, preferrably generated from hash of UUID at plugin startup.
     */
    void setTrackInfo(uint64_t trackIdentifier);

    /**
     * @brief Set the boolean telling if the daw is compatible with what we wanna do or not.
     * If the daw is not compatible, we will still ship data but with the compatibility flag to flag
     * to ignore the data.
     *
     * @param isCompatible true if the daw is compatible, false otherwise.
     */
    void setDawIsCompatible(bool isCompatible);

  private:
    /**
     * @brief The background thread loop that coalesce audio block info
     */
    void coalescePayloadsThreadLoop();

    /**
     * @brief The background thread loop that sends audio payloads to the station
     */
    void sendPayloadsThreadLoop();

    /**
     * @brief Tells if the audio segment payload is empty or not.
     *
     * @return true The audio segment payload does not yet include data
     * @return false The audio segment payload already include some audio data
     */
    static bool payloadIsEmpty(std::shared_ptr<AudioTransport::AudioSegmentPayload>);

    /**
     * @brief Tells if the audio segment payload is full or not.
     *
     * @return true The audio segment payload has all its samples filled, it should be sent.
     * @return false The audio segment payload still has room for more audio data, we'll keep filling it.
     */
    static bool payloadIsFull(std::shared_ptr<AudioTransport::AudioSegmentPayload>);

    /**
     * @brief Clear the audio samples.
     *
     */
    static void clearPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload>);

    /**
     * @brief Copy metadata from src to dest, and also add some track info from this class.
     * Metadata includes all daw and track informations except the segment size or audio data.
     *
     * @param dest where to copy the data to
     * @param src where to copy the data from
     */
    void copyMetadataToPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload> dest,
                               std::shared_ptr<AudioBlockInfo> src);

    static size_t appendAudioBlockToPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload> dest,
                                            std::shared_ptr<AudioBlockInfo> src);

    /**
     * @brief Remove the first noUsedSamples samples used from the audio block info so that
     * the remaining ones can be used. It also shift the buffer start position.
     *
     * @param usedBlock The audio block info that was used to fill a payload.
     * @param noUsedSamples The number of audio blocks that were used and to not account for anymore.
     */
    static void removeFirstUsedSamplesFromAudioBlock(std::shared_ptr<AudioBlockInfo> usedBlock, size_t noUsedSamples);

    static bool audioBlockInfoFollowsPayloadContent(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload,
                                                    std::shared_ptr<AudioBlockInfo> src);

    static void fillPayloadRemainingSpaceWithZeros(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload);

    void allocateCurrentlyFilledPayloadIfNecessary();

    bool payloadIsFullOrBlockInfoRemains(size_t queuedBlockInfoIndex);

    void queueCurrentlyFilledPayloadForSend();

    /////////////////////////////////////

    std::shared_ptr<std::thread> coalescerThread;
    std::shared_ptr<std::thread> senderThread;

    std::queue<std::shared_ptr<AudioTransport::AudioSegmentPayload>> freePayloads;
    std::queue<std::shared_ptr<AudioTransport::AudioSegmentPayload>> payloadsToSend;
    std::condition_variable payloadsCV;
    std::mutex payloadsMutex;

    std::vector<std::shared_ptr<AudioBlockInfo>> preallocatedBlockInfo;
    LockFreeIndexFIFO freeBlockInfos;
    std::shared_ptr<std::vector<size_t>> freeBlockInfosFetchContainer;

    LockFreeIndexFIFO blockInfosToCoalesce;
    std::condition_variable blockInfosToCoalesceCV;
    std::shared_ptr<std::vector<size_t>> blockInfoToCoalesceFetchContainer;
    std::mutex blockInfosToCoalesceMutex;

    std::shared_ptr<AudioTransport::AudioSegmentPayload> currentlyFilledPayload;

    std::atomic<bool> shouldStop;

    std::atomic<uint64_t> trackIdentifier;
    std::atomic<uint8_t> trackColorRed;
    std::atomic<uint8_t> trackColorGreen;
    std::atomic<uint8_t> trackColorBlue;
    std::atomic<uint8_t> trackColorAlpha;
    std::string trackName;
    std::mutex trackNameMutex;

    std::atomic<bool> dawIsCompatible;
};