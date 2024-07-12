#pragma once

#include "AudioTransport.pb.h"
#include "AudioTransport/AudioSegmentPayloadSender.h"
#include "SinkPlugin/LockFreeFIFO.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "AudioBlockInfo.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"

#define FORWARDER_THREAD_MAX_WAIT_MS 100

#define NUM_PREALLOCATED_BLOCKINFO 16
#define NUM_PREALLOCATED_COALESCED_PAYLOADS 8
#define PREALLOCATED_BLOCKINFO_SAMPLE_SIZE 4096
#define DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE 4096

/**
 * @brief A class that receives AudioBlockInfos from audio thread, and
 * queue them for a coalescing thread to aggregate them into AudioSegmentPayloads,
 * which are queued for a sender thread to send these to the Kholors Station gRPC api.
 */
class BufferForwarder : public TaskListener
{
  public:
    BufferForwarder(AudioTransport::AudioSegmentPayloadSender &ps, TaskingManager &tm);
    ~BufferForwarder();

    /**
     * @brief Get a preallocated AudioBlockInfo struct to copy audio block data into;
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
    void initializeTrackInfo(uint64_t trackIdentifier);

    /**
     * @brief Set the boolean telling if the daw is compatible with what we wanna do or not.
     * If the daw is not compatible, we will still ship data but with the compatibility flag to flag
     * to ignore the data.
     *
     * @param isCompatible true if the daw is compatible, false otherwise.
     */
    void setDawIsCompatible(bool isCompatible);

    /**
     * @brief Handler for the tasks that goes through task manager.
     * Mostly here to receive tasks about UI changes related to color
     * and track name update.
     *
     * @param task
     * @return true
     * @return false
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    juce::Colour getCurrentColor();

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

    /**
     * @brief Copy audio data from audio block info to audio segment payload. It continues
     * whathever data was already copied in payload, and will update the duration of payload and used samples
     * of block info. It returns how many samples are remaining in the audio block info or 0 if the block info was fully
     * exhausted.
     *
     * @param dest The payload to copy data into
     * @param src The audio block info to copy data from
     * @return size_t The number of samples remaining unused in audio block (0 if payload is not full or its sample
     * perfect filled).
     */
    static size_t appendAudioBlockToPayload(std::shared_ptr<AudioTransport::AudioSegmentPayload> dest,
                                            std::shared_ptr<AudioBlockInfo> src);

    /**
     * @brief Tells if the data in the audio block is the continuation of the one already in the payload
     * by checking the sample position in the track. If there is no data in the payload, it returns true.
     *
     * @param payload Payload where the audio info blocks are supposed to be copied.
     * @param src Block info wheere audio data is supposed to be copied from.
     * @return true The src audio data is the continuation of what's inside the payload.
     * @return false There is a gap (even maybe negative) between the payload audio data and what's in the src buffer.
     */
    static bool audioBlockInfoFollowsPayloadContent(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload,
                                                    std::shared_ptr<AudioBlockInfo> src);

    /**
     * @brief Change the size of the payload to the full size, and ensure the remaining signal up untill then
     * is filled with zeros.
     *
     * @param payload the payload to operate on.
     */
    static void fillPayloadRemainingSpaceWithZeros(std::shared_ptr<AudioTransport::AudioSegmentPayload> payload);

    /**
     * @brief If the currently filled payload is nullptr, will try to fetch a preallocated one
     * or allocate one, assign it to currentlyFilledPayload ptr, and clear it.
     * Does nothing if currentlyFilledPayload is not nullptr.
     */
    void allocateCurrentlyFilledPayloadIfNecessary();

    /**
     * @brief Tells of the currentlyFilledPayload is fulled or if the freeBlockInfosFetchContainer
     * still has elemented at the provided queuedBlockInfoIndex index.
     * This condition is used by the coalescing thread main loop to know if there is still work
     * to perform.
     *
     * @param queuedBlockInfoIndex Index at which we wanna check if freeBlockInfosFetchContainer has data.
     * @return true The payload is full or there is still data to coalesce.
     * @return false The payload is not full and there is no more data to coalesce into payloads.
     */
    bool payloadIsFullOrBlockInfoRemains(size_t queuedBlockInfoIndex);

    /**
     * @brief Put the pointer in currentlyFilledPayload into the queue of payloads to send to the station,
     * and reset its value to nullptr.
     *
     */
    void queueCurrentlyFilledPayloadForSend();

    /////////////////////////////////////

    TaskingManager &taskManager;

    std::shared_ptr<std::thread> coalescerThread;
    std::shared_ptr<std::thread> senderThread;

    AudioTransport::AudioSegmentPayloadSender &payloadSender;

    std::queue<std::shared_ptr<AudioTransport::AudioSegmentPayload>>
        freePayloads; /**< payloads that are allocated and ready to be filled */
    std::queue<std::shared_ptr<AudioTransport::AudioSegmentPayload>>
        payloadsToSend; /**< payloads that are waiting to be sent to Kholors station */
    std::condition_variable
        payloadsCV; /**< A condition variable to wake up the sender thread when paylodsToSend has queued elements */
    std::mutex payloadsMutex; /**< A mutex to prevent race condition on the payloadsToSend queue */

    std::vector<std::shared_ptr<AudioBlockInfo>>
        preallocatedBlockInfo; /**< Total number of block info allocated, can be used anywhere and ids are tracked by
                                  queues */
    LockFreeIndexFIFO
        freeBlockInfos; /**< Thread (1 in 1 out) safe queue to store unused AUdioBlockInfos ready to be filled */
    std::shared_ptr<std::vector<size_t>>
        freeBlockInfosFetchContainer; /**< Vector that is filled when audio thread request a blockInfo struct to copy
                                         data onto. Note that it's a specific case where only 1 item is requested. */

    LockFreeIndexFIFO blockInfosToCoalesce; /**< Thread (1 in 1 out) safe queue of block info that are waiting to be
                                               coalesced into payloads*/
    std::condition_variable
        blockInfosToCoalesceCV; /**< A condition variable for the coalescer thread to wait for work */
    std::shared_ptr<std::vector<size_t>> blockInfoToCoalesceFetchContainer; /**< Vector that gets filled when the
                                         coalescer thread request the queue for items to coalesce. */
    std::mutex blockInfosToCoalesceMutex; /**< A mutex to give to the CV wait method, not really usefull as we use a
                                             lock free queue with atomics */

    std::shared_ptr<AudioTransport::AudioSegmentPayload>
        currentlyFilledPayload; /**< The payload that is currently being copied AudioBlockInfo data into by coalescer
                                   thread */

    std::atomic<bool> shouldStop; /**< True whenever the background threads should stop */

    std::atomic<uint64_t> trackIdentifier; /**< Unique identifier of this vst instance */
    std::atomic<uint8_t> trackColorRed;    /**< Level of red in track color */
    std::atomic<uint8_t> trackColorGreen;  /**< Level of green in track color */
    std::atomic<uint8_t> trackColorBlue;   /**< Level of ble in track color */
    std::atomic<uint8_t>
        trackColorAlpha;       /**< Level of alpha in track color (unused but still required for alignment) */
    std::string trackName;     /**< Name of the audio track */
    std::mutex trackNameMutex; /**< Lock to protect the string trackName */

    std::atomic<bool> dawIsCompatible; /**< tells if the DAW is compatible with Kholors station */
};