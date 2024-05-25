#ifndef DEF_RPC_SERVER_IMPLEMENTATION_DEF
#define DEF_RPC_SERVER_IMPLEMENTATION_DEF

#include "AudioTransport.grpc.pb.h"
#include "AudioTransport.pb.h"
#include "grpcpp/server_context.h"
#include <grpcpp/support/status.h>

namespace AudioTransport
{

class RpcServerImplementation final : public KholorsAudioTransport::Service
{
  public:
    // TODO: Use AudioSegmentStore

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
};

} // namespace AudioTransport

#endif // DEF_RPC_SERVER_IMPLEMENTATION_DEF