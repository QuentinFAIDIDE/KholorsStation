#pragma once

#include "AudioDataStore.h"
#include "RpcServerImplementation.h"
#include "TaskManagement/TaskingManager.h"
#include "grpcpp/server_builder.h"
#include <condition_variable>
#include <memory>
#include <thread>

namespace AudioTransport
{

/**
 * @brief The set of errors a
 * server can return when starting.
 */
enum ServerStartError
{
    NO_ERROR = 0,
    PORT_NOT_AVAILABLE = 1,
    ALREADY_RUNNING = 2,
    ERROR = 3
};

/**
 * @brief A gRPC server that receives audio segments
 * from clients and process request synchronously.
 *
 */
class SyncServer final
{
  public:
    /**
     * @brief Construct a new Sync Server object
     *
     */
    SyncServer();

    ~SyncServer();

    /**
     * @brief Set the Task Manager object
     *
     * @param tm TaskManager that will get sent task with server related updates
     */
    void setTaskManager(TaskingManager *tm);

    /**
     * @brief Start or restart the server on the provided port.
     *
     * @param port Port to start the server on.
     * @return ServerStartError Eventual error the server might encounter while starting.
     */
    ServerStartError setServerToListenOnPort(uint32_t port);

    /**
     * @brief Stop the gRPC server.
     */
    void stopServer();

    /**
     * @brief Block for maximum 1second untill a datum is ready to be passed back to the station.
     * Or untill the store is currently being stopped.
     *
     * @return Either the update datum along with the storage Identifier, or either nullptr if
     * the audio data store is stopping or the 1 second maximum timeout is reached.
     */
    std::optional<AudioDataStore::AudioDatumWithStorageId> waitForDatum();

    /**
     * @brief Will let the store know that this specific struct will not be read
     * any more and can be reassigned to another incoming AudioTransportData.
     *
     * @param storageIndentifier identifier returned along the update in waitForDatum
     */
    void freeStoredDatum(uint64_t storageIndentifier);

    /**
     * @brief Tells if the server is running
     *
     * @return true the server is running
     * @return false the server is not running
     */
    bool isRunning();

  private:
    /**
     * @brief Run server on given port on localhost. The server needs to be stopped with stopServer before.
     *
     * @param port port to run the server on.
     * @return true It worked.
     * @return false It failed.
     */
    bool runServerOnPort(size_t port);

    /**
     * @brief For the server thread to wait for server shutdown and change state.
     */
    void waitForServerShutdown();

    std::shared_ptr<grpc::Server> coreServer;     /**< gRPC server */
    std::shared_ptr<std::thread> serverThread;    /**< Thread that waits for the server to shutdown. Can be joined after
                                                     shutdown is called to get the state to update. */
    std::condition_variable serverThreadCondvar;  /**< Conditional variable to wake up server thread */
    std::mutex serverThreadMutex;                 /**< To protect desired state */
    std::pair<bool, uint32_t> desiredServerState; /**< Running state and port to aim for with the server */
    std::pair<bool, uint32_t> actualServerState;  /**< Actual running state and port of the server */

    std::unique_ptr<grpc::Server> server; /**< gRPC server instance */

    RpcServerImplementation service; /**< gRPC server implementation */
    AudioDataStore store;            /**< Where samples are stored when received untill they're fetched */

    TaskingManager *taskingManager;
};

}; // namespace AudioTransport