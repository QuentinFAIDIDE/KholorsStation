#include "ServerRunner.h"
#include "HeadlessAudioBroadcast.pb.h"
#include "HeadlessAudioBroadcast/Constants.h"
#include "absl/strings/str_format.h"
#include "grpcpp/ext/proto_server_reflection_plugin.h"
#include "grpcpp/server_builder.h"
#include "spdlog/spdlog.h"
#include <chrono>
#include <mutex>
#include <thread>

namespace HeadlessAudioBroadcast
{

ServerRunner::ServerRunner()
{
    serverIsRunning = false;
    needsToStop = false;
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
}

ServerRunner::~ServerRunner()
{
    // lock the server mutex and set needsToStop to true
    {
        std::lock_guard lock(serverThreadMutex);
        if (!serverIsRunning)
        {
            return;
        }
        needsToStop = true;
    }
    // wait for the election thread to complete
    electionLoopThread->join();
}

void ServerRunner::masterElectionThreadLoop()
{
    using namespace std::chrono_literals;
    while (true)
    {
        {
            std::lock_guard lock(serverThreadMutex);
            if (needsToStop)
            {
                stopServer();
                return;
            }
            if (!serverIsRunning)
            {
                runServerOnPort(DEFAULT_SERVER_PORT);
            }
        }
        std::this_thread::sleep_for(100ms);
    }
}

bool ServerRunner::runServerOnPort(size_t port)
{
    // NOTE: this is called with the serverThreadMutex locked

    if (serverIsRunning)
    {
        spdlog::warn("Tried to start the server but it is already started");
        return true;
    }

    // then we setup the server
    std::string server_address = absl::StrFormat("127.0.0.1:%d", port);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&serverBackend);

    server = builder.BuildAndStart();
    if (server == nullptr)
    {
        return false;
    }

    spdlog::info("Server started and is listening on " + server_address);
    serverIsRunning = true;

    serverThread = std::make_shared<std::thread>(&ServerRunner::waitForServerShutdown, this);

    return true;
}

void ServerRunner::waitForServerShutdown()
{
    server->Wait();
    spdlog::info("Server has stopped");
}

void ServerRunner::stopServer()
{
    // NOTE: this is called with the serverThreadMutex locked

    // stop the server if it's running
    server->Shutdown();
    serverThread->join();

    // ensure the ServerService clients have likely drained their pending ffts
    serverBackend.drainPendingFfts();
    std::this_thread::sleep_for(std::chrono::milliseconds(4 * CLIENTS_DRAIN_PERIOD_MS));

    // free the service implementation preallocated memory
    serverBackend.freePreallocatedStructs();
}

} // namespace HeadlessAudioBroadcast