#pragma once

#include "HeadlessAudioBroadcast.grpc.pb.h"
#include "HeadlessAudioBroadcast.pb.h"
#include "HeadlessAudioBroadcast/FftProcessor.h"
#include <condition_variable>
#include <memory>
namespace HeadlessAudioBroadcast
{

class ServerService final : public KholorsHeadlessAudioBroadcast::Service
{
  public:
    ServerService();
    void freePreallocatedStructs();
    void drainPendingFfts();

  private:
    /**
     * @brief gRPC callback for when clients call the audio upload endpoint.
     *
     * @param ctx gRPC server context
     * @param data audio segments and their metadata uploaded by VST clients
     * @param response repsponse to send to clients uploading data
     * @return grpc::Status Status of the request (ie Ok, Failed, etc...)
     */
    grpc::Status UploadAudioSegment(grpc::ServerContext *ctx, const AudioSegmentPayload *data,
                                    AudioSegmentUploadResponse *response) override;

    /**
     * @brief gRPC callback for when clients that opened UI are fetching processed FFTs..
     *
     * @param ctx gRPC server context
     * @param data offset in the sequence of processed audio tasks, as well as server id
     * @param response response, a list of FFT "tasks" bearing daw and tracks metadata starting at the required offset
     * @return grpc::Status Status of the request (ie Ok, Failed, etc...)
     */
    grpc::Status GetAudioTasks(grpc::ServerContext *ctx, const AudioTasksOffset *request,
                               AudioTasks *response) override;

    /**
     * @brief Allocate audio segment payloads and fft result structs
     *
     */
    void preallocateStructs();

    std::shared_ptr<FftProcessor> fftProcessor;
    std::mutex fftProcessorMutex;

    std::queue<std::shared_ptr<AudioSegmentPayload>> audioSegmentsToProcess;
    std::mutex audioSegmentsToProcessMutex;
    std::condition_variable audioSegmentsToProcessCondvar;

    std::queue<std::shared_ptr<AudioSegmentPayload>> freeAudioSegmentStructs;
    std::mutex freeAudioSegmentStructsMutex;
};

} // namespace HeadlessAudioBroadcast