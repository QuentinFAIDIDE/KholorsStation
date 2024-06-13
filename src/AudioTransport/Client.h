#pragma once

#include "AudioTransport.grpc.pb.h"
#include "AudioTransport.pb.h"
#include "AudioTransport/AudioSegmentPayloadSender.h"
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <shared_mutex>

namespace AudioTransport
{
/**
 * @brief A gRPC client that forwards audio segments
 * to the server.
 *
 */
class Client : public AudioSegmentPayloadSender
{
  public:
    Client(uint32_t port);
    void changeDestinationPort(uint32_t port);
    bool sendAudioSegment(AudioSegmentPayload *payload) override;

  private:
    std::unique_ptr<KholorsAudioTransport::Stub> stub;
    uint32_t lastPortUsed;
    std::shared_mutex portChangeMutex;
};
}; // namespace AudioTransport