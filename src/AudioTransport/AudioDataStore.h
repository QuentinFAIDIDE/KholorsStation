#pragma once

#include "AudioTransport.pb.h"
#include "AudioTransportData.h"
#include "DawInfo.h"
#include "TrackInfo.h"
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <queue>

namespace AudioTransport
{

class AudioDataStoreTestSuite;

/**
 * @brief Defines a class that stores and deliver various type of structures
 * and preallocate and reuse the structs in order to minimize heap allocations.
 * It contains a queue on which audio data consumers can concurrently do blocking reads,
 * and data publishers can concurrently send the struct received by the gRPC api to be
 * split into various AudioTransportData instances fed to the queue.
 */
class AudioDataStore
{
  public:
    /**
     * @brief Construct a new Audio Data Store object
     *
     * @param preallocatedAudioSegments number of audio segment structs to preallocate
     */
    AudioDataStore(size_t preallocatedAudioSegments);

    /**
     * @brief An audio datum along with an id to identify where it's stored.
     */
    struct AudioDatumWithStorageId
    {
        std::shared_ptr<AudioTransportData> datum;
        uint64_t storageIdentifier;
    };

    /**
     * @brief Block for maximum 1second untill a datum is ready to be passed back to the station.
     * Or untill the store is currently being stopped.
     *
     * @return Either the update datum along with the storage Identifier, or either nullptr if
     * the audio data store is stopping or the 1 second maximum timeout is reached.
     */
    std::optional<AudioDatumWithStorageId> waitForDatum();

    /**
     * @brief Will let the store know that this specific struct will not be read
     * any more and can be reassigned to another incoming AudioTransportData.
     *
     * @param storageIndentifier identifier returned along the update in waitForDatum
     */
    void freeStoredDatum(uint64_t storageIndentifier);

    /**
     * @brief Called by server when it received AudioSegmentPayload structs
     * and will copy the data to preallocated buffers, and split it into
     * different types of AudioTransportData. These types are as of now
     * Daw information, track information, audio segments (data signal), and audio segments no-op.
     *
     * @param payload the structure received by gRPC api as defined by protobuf containing audio information.
     *
     * @throw std::runtime_error if the error is internal (including if nullptr is provided) == 400 api error.
     * @throw std::invalid_argument if the request is invalid == 500 api error.
     */
    void parseNewData(const AudioSegmentPayload *payload);

    /**
     * @brief A testing utility to check on how many buffers of each struct are free to be used.
     *
     * @return std::vector<size_t> Vector of size 3 with number of free structs for [AudioSegment, DawInfo, TrackInfo].
     */
    std::vector<size_t> countFreePreallocatedStructs();

    /**
     * @brief Will stop serving consumers and publishers.
     */
    void stopServing();

    /**
     * @brief Tell consumers or publishers that no more requests will be served.
     *
     * @return true no more request will be served.
     * @return false requests are served as usual.
     */
    bool hasStoppedServing();

    friend class AudioDataStoreTestSuite;

  private:
    /**
     * @brief If necessary, extract an audio segment from the gRPC endpoint payload.
     *
     * @param payload The payload received by the gRPC api
     * @return Returns nothing if there is no need for a segment, or the segment with id
     * otherwise.
     */
    std::vector<AudioDatumWithStorageId> extractPayloadAudioSegments(const AudioSegmentPayload *payload);

    /**
     * @brief Tries to reserve ownership for one of the preallocated audio segments.
     *
     * @return std::optional<AudioDatumWithStorageId> std::nullopt if there is no more free preallocated buffers,
     * an AudioSegment datastruct otherwise.
     */
    std::optional<AudioDatumWithStorageId> reserveAudioSegment();

    /**
     * @brief Tries to reserve ownership for one of the preallocated DAW info.
     *
     * @return std::optional<AudioDatumWithStorageId> std::nullopt if there is no more free daw info,
     * an AudioDatumWithStorageId datastruct otherwise.
     */
    std::optional<AudioDatumWithStorageId> reserveDawInfo();

    /**
     * @brief Tries to reserve ownership for one of the preallocated Track info.
     *
     * @return std::optional<AudioDatumWithStorageId> std::nullopt if there is no more free track info,
     * an AudioDatumWithStorageId datastruct otherwise.
     */
    std::optional<AudioDatumWithStorageId> reserveTrackInfo();

    /**
     * @brief Push the datum with its storage id to the queue and notify the condition variable.
     *
     * @param datum audio update datum with its id to broadcast to the listeners through waitForDatum
     */
    void pushAudioDatumToQueue(AudioDatumWithStorageId datum);

    std::queue<AudioDatumWithStorageId> pendingAudioData; /**< data updates to be passed to the Station */
    std::mutex pendingAudioDataMutex; /**< A mutex to protect concurrent access to the pendingAudioData queue*/
    std::condition_variable pendingAudioDataCondVar; /**< a condition variable to wait on new queued data updates */

    std::vector<AudioDatumWithStorageId> preallocatedAudioSegments; /**< preallocated audio buffers to reuse */
    std::set<uint64_t> freeAudioSegments;      /**< Buffers that are not currently being used by the station */
    std::mutex preallocatedAudioSegmentsMutex; /**< Mutex to protect freeAudioSegments set. */

    std::vector<AudioDatumWithStorageId> preallocatedDawInfo; /**< preallocated daws info structs to reuse */
    std::set<uint64_t> freeDawInfo;      /**< DawInfo that are not currently being used by the station */
    std::mutex preallocatedDawInfoMutex; /**< Mutex to protect freeDawInfo set. */

    std::vector<AudioDatumWithStorageId> preallocatedTrackInfo; /**< preallocated track info structs to reuse */
    std::set<uint64_t> freeTrackInfo;      /**< DawInfo that are not currently being used by the station */
    std::mutex preallocatedTrackInfoMutex; /**< Mutex to protect freeTrackInfo set. */

    // these next two elements are not thread safe, the server currently only has one thread, beware.
    std::map<uint64_t, TrackInfo> trackInfoByIdentifier; /**< Map of tracks info to prevent pushing duplicate updates*/
    DawInfo lastDawInfo; /**< last daw info received to prevent pushing duplicate updates */

    size_t noPreallocatedStructs;

    bool isStopping;
};

} // namespace AudioTransport
