#ifndef DEF_AUDIO_TRANSPORT_CLIENT_HPP
#define DEF_AUDIO_TRANSPORT_CLIENT_HPP

#include "AudioTransport.grpc.pb.h"
#include "AudioTransport.pb.h"
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
class Client
{
  public:
    Client(uint32_t port);
    void changeDestinationPort(uint32_t port);
    bool sendAudioSegment(AudioSegmentPayload *payload);

  private:
    std::unique_ptr<KholorsAudioTransport::Stub> stub;
    uint32_t lastPortUsed;
    std::shared_mutex portChangeMutex;
};
}; // namespace AudioTransport

#endif // DEF_AUDIO_TRANSPORT_CLIENT_HPP