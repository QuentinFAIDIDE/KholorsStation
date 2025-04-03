#pragma once

#include "HeadlessAudioBroadcast.grpc.pb.h"
#include "HeadlessAudioBroadcast.pb.h"
#include "HeadlessAudioBroadcast/AudioEventFetcher.h"
#include "HeadlessAudioBroadcast/AudioSegmentPayloadSender.h"
#include "HeadlessAudioBroadcast/TrackInfo.h"
#include "TaskManagement/TaskListener.h"
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <queue>

namespace HeadlessAudioBroadcast
{
/**
 * @brief A gRPC client that forwards audio segments
 * to the server.
 *
 */
class Client : public AudioSegmentPayloadSender, public AudioEventFetcher, public TaskListener
{
  public:
    Client(uint32_t port);
    void changeDestinationPort(uint32_t port);
    bool sendAudioSegment(const AudioSegmentPayload *payload) override;
    std::vector<std::shared_ptr<Task>> getNextAudioEvents() override;
    std::vector<TrackInfo> getAllKnownTrackInfos() override;
    float getKnownBpm() override;
    int getKnownTimeSignature() override;
    void tryReconnect() override;
    bool taskHandler(std::shared_ptr<Task> task) override;
    void freePreallocatedFftResultBuffers();

  private:
    /**
     * @brief Store this track info locally and let caller
     * know if the value was changed or not.
     * The mutex must be locked to safely call this function.
     *
     * @param ti track info to store
     * @return true track info provided was new or an update
     * @return false track info provided was already stored with that value
     */
    bool storeTrackInfo(TrackInfo &ti);

    /**
     * @brief Reads the receivedTask that is received from the server,
     * and copy the fft data (the actual fft result datums for intensity bins)
     * into a new vector of floats, that is either taken from a pool of preallocated
     * buffers or directly allocated if the pool is empty.
     * Note that once the NewFftDataTask this vector will be put into is fully processed,
     * the vector will be passed into a FftResultVectorReuseTask that this class
     * taskHandler will catch and put back into the pool.
     * SHOULD BE CALLED WHEN THE mutex IS LOCKED.
     *
     * @param receivedTask The audio task received from the Server summarizing the fft
     * data, its position, length, as well as track and daw info.
     * @return std::shared_ptr<std::vector<float>> a vector of float with the fft data in it.
     */
    std::shared_ptr<std::vector<float>> extractFftDatums(FftToDrawTask &receivedTask);

    std::unique_ptr<KholorsHeadlessAudioBroadcast::Stub> stub;
    uint32_t lastPortUsed;
    std::mutex mutex;
    grpc::ChannelArguments channelArgs;
    AudioTasksOffset audioTaskOffset;  /**< The offset in audio tasks that is passed and updated at every request to
                                          query next audio tasks */
    AudioTasks taskListResponseBuffer; /**< Responst struct that is returned by the getNettAudioEvents endpoint,
                                               pased into tasks */
    std::queue<std::shared_ptr<std::vector<float>>> preallocatedFftResultArrays;
    std::map<uint64_t, TrackInfo> knownTrackInfos;

    float knownBpm;
    int knownTimeSignatureNumerator;
};
}; // namespace HeadlessAudioBroadcast