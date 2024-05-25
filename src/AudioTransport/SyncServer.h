#ifndef DEF_SYNC_SERVER_HPP
#define DEF_SYNC_SERVER_HPP

#include "RpcServerImplementation.h"

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
    OTHER_ERROR = 2
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

    /**
     * @brief Start or restart the server on the provided port.
     *
     * @param port Port to start the server on.
     * @return ServerStartError Eventual error the server might encounter while starting.
     */
    ServerStartError serveRequestsOnPort(uint32_t port);

  private:
    // gRPC server implementation for the transport of audio segments
    RpcServerImplementation service;
};

}; // namespace AudioTransport

#endif // DEF_SYNC_SERVER_HPP