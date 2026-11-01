#include "RpcServerImplementation.h"
#include "AudioDataStore.h"
#include "TooManyRequestsException.h"
#include <exception>
#include <grpcpp/support/status.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

using namespace AudioTransport;

RpcServerImplementation::RpcServerImplementation(AudioDataStore &storeToUse) : dataStore(storeToUse)
{
}

grpc::Status RpcServerImplementation::UploadAudioSegment(grpc::ServerContext * /*context*/,
                                                         const AudioSegmentPayload *data,
                                                         AudioSegmentUploadResponse * /*response*/)
{
    try
    {
        dataStore.parseNewData(data);
        return {grpc::Status::OK};
    }
    catch (TooManyRequestsException &e)
    {
        return {grpc::StatusCode::RESOURCE_EXHAUSTED, e.what()};
    }
    catch (std::invalid_argument &e)
    {
        return {grpc::StatusCode::INVALID_ARGUMENT, e.what()};
    }
    catch (std::exception &e)
    {
        return {grpc::StatusCode::INTERNAL, e.what()};
    }
}