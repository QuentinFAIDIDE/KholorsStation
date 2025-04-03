#pragma once

#include "HeadlessAudioBroadcast/ServerService.h"
#include "grpcpp/server.h"

namespace HeadlessAudioBroadcast
{

/**
 * @brief ServerRunner is a class that describe
 * an object that constantly tries to start a
 * ServerService in the background on a certain predefined port,
 * using the port reservation as a distributed lock to ensure
 * only one server per plugin replica works.
 * The ServerService it runs receive Audio Data that it returns
 * to the clients along with the metadata after having
 * converted its audio segments to a sequence of SFFTs.
 *
 */
class ServerRunner
{
  public:
    ServerRunner();
    ~ServerRunner();

  private:
    void masterElectionThreadLoop();
    void stopServer();
    bool runServerOnPort(size_t port);
    void waitForServerShutdown();

    std::shared_ptr<std::thread> electionLoopThread;
    std::shared_ptr<std::thread> serverThread; /**< Thread that waits for the server to shutdown. Can be joined after
                                                  shutdown is called to get the state to update. */
    bool serverIsRunning;
    bool needsToStop;
    std::mutex serverThreadMutex;
    ServerService serverBackend;          /**< actual server implementation */
    std::unique_ptr<grpc::Server> server; /**< gRPC server instance */
};

} // namespace HeadlessAudioBroadcast