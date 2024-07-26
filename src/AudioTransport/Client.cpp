#include "Client.h"
#include "AudioTransport.grpc.pb.h"
#include "AudioTransport.pb.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include <cstdint>
#include <grpc/grpc.h>
#include <mutex>
#include <shared_mutex>
#include <string>

using namespace AudioTransport;

Client::Client(uint32_t portToUse) : lastPortUsed(portToUse)
{
    grpc::ChannelArguments channelArgs;

    std::string serviceConfigJSON =
        R"(

{
  "methodConfig": [
    {
      "name": [
        {
          "service": ""
        }
      ],
      "waitForReady": false,
      "timeout": "2s"
    }
  ]
}

)";

    // Send a keepalive ping every 2 seconds
    channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 2000);
    // If no ping response is received after 1 second, consider the connection dead
    channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 1000);

    // see: https://github.com/grpc/grpc/blob/master/doc/connection-backoff.md
    // initial time to retry connecting after a disconnect
    channelArgs.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, 500);
    // minimum connect timeout
    channelArgs.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 500);
    // maximum time between retrying to connect
    channelArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 2000);

    channelArgs.SetServiceConfigJSON(serviceConfigJSON);

    auto chan = grpc::CreateCustomChannel("127.0.0.1:" + std::to_string(portToUse), grpc::InsecureChannelCredentials(),
                                          channelArgs);
    stub = KholorsAudioTransport::NewStub(chan);
}

void Client::changeDestinationPort(uint32_t port)
{
    std::unique_lock lock(portChangeMutex);
    if (lastPortUsed == port)
    {
        return;
    }
    lastPortUsed = port;

    auto chan =
        grpc::CreateCustomChannel("127.0.0.1:" + std::to_string(port), grpc::InsecureChannelCredentials(), channelArgs);
    stub = KholorsAudioTransport::NewStub(chan);
}

bool Client::sendAudioSegment(const AudioSegmentPayload *payload)
{
    std::shared_lock lock(portChangeMutex);

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    AudioSegmentUploadResponse reply;

    grpc::Status status = stub->UploadAudioSegment(&context, *payload, &reply);
    return status.ok();
}