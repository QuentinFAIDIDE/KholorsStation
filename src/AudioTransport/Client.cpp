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
    auto chan = grpc::CreateChannel("127.0.0.1:" + std::to_string(portToUse), grpc::InsecureChannelCredentials());
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

    auto chan = grpc::CreateChannel("127.0.0.1:" + std::to_string(port), grpc::InsecureChannelCredentials());
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