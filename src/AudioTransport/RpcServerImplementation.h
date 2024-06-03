#pragma once

#include "AudioDataStore.h"
#include "AudioTransport.grpc.pb.h"
#include "AudioTransport.pb.h"
#include "grpcpp/server_context.h"
#include <grpcpp/support/status.h>

namespace AudioTransport
{

class RpcServerImplementation final : public KholorsAudioTransport::Service
{
  public:
    RpcServerImplementation(AudioDataStore &);

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

    AudioDataStore &dataStore; /**< where the endpoint callback will store data and where it will be read by consumers*/
};

} // namespace AudioTransport