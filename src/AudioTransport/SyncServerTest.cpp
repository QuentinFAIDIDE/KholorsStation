#include "SyncServerTest.h"
#include "SyncServer.h"
#include <stdexcept>

using namespace AudioTransport;

void SyncServerTestSuite::runAll()
{
    smokeTest01();
}

void SyncServerTestSuite::smokeTest01()
{
    SyncServer server;

    auto state = server.setServerToListenOnPort(8879);
    if (state != NO_ERROR)
    {
        throw std::runtime_error("Unable to start gRPC server");
    }
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
    sleep(2);
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
    // should do nothing
    state = server.setServerToListenOnPort(8879);
    if (state != ALREADY_RUNNING)
    {
        throw std::runtime_error("Unable to start gRPC server");
    }
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
    state = server.setServerToListenOnPort(8899);
    if (state != NO_ERROR)
    {
        throw std::runtime_error("Unable to start gRPC server");
    }
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
    server.stopServer();
    if (server.isRunning())
    {
        throw std::runtime_error("Server state is running");
    }
    state = server.setServerToListenOnPort(8899);
    if (state != NO_ERROR)
    {
        throw std::runtime_error("Unable to start gRPC server");
    }
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
}