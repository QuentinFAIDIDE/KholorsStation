#include "SyncServer.h"
#include "AudioTransport/ServerStatusTask.h"
#include "TaskManagement/TaskingManager.h"
#include "absl/strings/str_format.h"
#include <cstdint>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <thread>

using namespace AudioTransport;

// TODO: make this thing a setting
#define DEFAULT_STORE_PREALLOCS 4096
#define DEFAULT_SERVER_PORT 8991

SyncServer::SyncServer() : store(DEFAULT_STORE_PREALLOCS), service(store)
{
    desiredServerState = std::pair<bool, uint32_t>(false, DEFAULT_SERVER_PORT);
    actualServerState = std::pair<bool, uint32_t>(false, DEFAULT_SERVER_PORT);

    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
}

SyncServer::~SyncServer()
{
    store.stopServing();
    if (isRunning())
    {
        stopServer();
    }
}

ServerStartError SyncServer::setServerToListenOnPort(uint32_t port)
{
    bool serverIsRunning;
    {
        std::lock_guard lock(serverThreadMutex);
        // if we are already in desired state, abort
        desiredServerState.first = true;
        desiredServerState.second = port;
        if (desiredServerState == actualServerState)
        {
            return ALREADY_RUNNING;
        }
        serverIsRunning = actualServerState.first;
    }
    // if server is started, stop it
    if (serverIsRunning)
    {
        stopServer();
    }
    // restart it on a specific port
    bool worked = runServerOnPort(port);
    if (!worked)
    {
        return ERROR;
    }
    else
    {
        return NO_ERROR;
    }
}

bool SyncServer::isRunning()
{
    std::lock_guard lock(serverThreadMutex);
    return actualServerState.first;
}

void SyncServer::stopServer()
{
    {
        std::lock_guard lock(serverThreadMutex);
        desiredServerState.first = false;
        if (!actualServerState.first)
        {
            spdlog::warn("stopServer called but server state is stopped");
            return;
        }
        // stop the server if it's running
        server->Shutdown();
    }
    serverThread->join();
}

bool SyncServer::runServerOnPort(size_t port)
{
    std::lock_guard lock(serverThreadMutex);
    // we first ensure that the server is stopped.
    if (actualServerState.first)
    {
        throw std::runtime_error(
            "runServerOnPort was called with server thread nullptr but with actualState as running");
    }
    // we update the desired state
    desiredServerState.first = true;
    desiredServerState.second = port;
    // then we setup the server
    std::string server_address = absl::StrFormat("127.0.0.1:%d", port);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    server = builder.BuildAndStart();
    if (server == nullptr)
    {
        spdlog::error("Unable to start gRPC server");
        return false;
    }
    spdlog::info("Server listening on " + server_address);
    actualServerState.first = true;
    actualServerState.second = port;
    if (taskingManager != nullptr)
    {
        taskingManager->broadcastTask(std::make_shared<ServerStatusTask>(actualServerState));
    }

    serverThread = std::make_shared<std::thread>(&SyncServer::waitForServerShutdown, this);

    return true;
}

void SyncServer::waitForServerShutdown()
{
    server->Wait();
    spdlog::info("Server has stopped");
    std::lock_guard lock(serverThreadMutex);
    actualServerState.first = false;
    if (taskingManager != nullptr)
    {
        taskingManager->broadcastTask(std::make_shared<ServerStatusTask>(actualServerState));
    }
}

void SyncServer::setTaskManager(TaskingManager *tm)
{
    std::lock_guard lock(serverThreadMutex);
    taskingManager = tm;
    taskingManager->broadcastTask(std::make_shared<ServerStatusTask>(actualServerState));
}

std::optional<AudioDataStore::AudioDatumWithStorageId> SyncServer::waitForDatum()
{
    return store.waitForDatum();
}

void SyncServer::freeStoredDatum(uint64_t storageIndentifier)
{
    store.freeStoredDatum(storageIndentifier);
}