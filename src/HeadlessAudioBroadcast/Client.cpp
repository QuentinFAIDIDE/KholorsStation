#include "Client.h"
#include "HeadlessAudioBroadcast.grpc.pb.h"
#include "HeadlessAudioBroadcast.pb.h"
#include "HeadlessAudioBroadcast/BpmUpdateTask.h"
#include "HeadlessAudioBroadcast/ColorBytes.h"
#include "HeadlessAudioBroadcast/Constants.h"
#include "HeadlessAudioBroadcast/FftResultVectorReuseTask.h"
#include "HeadlessAudioBroadcast/NewFftDataTask.h"
#include "HeadlessAudioBroadcast/TimeSignatureUpdateTask.h"
#include "HeadlessAudioBroadcast/TrackInfo.h"
#include "HeadlessAudioBroadcast/TrackInfoUpdateTask.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include <cstdint>
#include <grpc/grpc.h>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace HeadlessAudioBroadcast;

// WARNING: you should match whathever is parametered in the FFtRunner.h constants!
#define NUM_FFT_PER_DEFAULT_SEGMENT_SIZE 5 // this is knowing we sned 4096, and strafe bins of 2048 every 512
#define FFT_ZERO_PADDING_FACTOR 2
#define FFT_INPUT_NO_INTENSITIES 2048
#define FFT_OUTPUT_NO_FREQS (((FFT_INPUT_NO_INTENSITIES * FFT_ZERO_PADDING_FACTOR) >> 1) + 1)
#define AUDIO_SEGMENTS_FFTS_PREALLOC_SIZE 5 * FFT_OUTPUT_NO_FREQS

#define SEGMENT_PREALLOCATION_BATCH_SIZE 64

// NOTE: 512 preallocated buffers with 4096 segments and 2048 fft size with zero padding factor of 2
// corresponds to 40 Mb of memory.
// With 60 instances that can make a few gigabytes, therefore we preallocate smaller batches,
// and we should call freePreallocatedFftResultBuffers when the UI is closed in order to avoid
// memory usage building up.

Client::Client(uint32_t portToUse) : lastPortUsed(portToUse)
{
    grpc::ChannelArguments channelArgs;

    std::string serviceConfigJSON =
        R"(

{
  "methodConfig": [
    {
      "name": [
        {
          "service": ""
        }
      ],
      "waitForReady": false,
      "timeout": "2s"
    }
  ]
}

)";

    // Send a keepalive ping every 2 seconds
    channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 2000);
    // If no ping response is received after 1 second, consider the connection dead
    channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 1000);

    // see: https://github.com/grpc/grpc/blob/master/doc/connection-backoff.md
    // initial time to retry connecting after a disconnect
    channelArgs.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, 500);
    // minimum connect timeout
    channelArgs.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 500);
    // maximum time between retrying to connect
    channelArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 2000);

    channelArgs.SetServiceConfigJSON(serviceConfigJSON);

    knownBpm = DEFAULT_CLIENT_BPM;
    knownTimeSignatureNumerator = DEFAULT_TIME_SIGNATURE_NUMERATOR;

    // this is the default values that are shipped to the server,
    // server identifier and task offset cannot be zero so it knows
    // that it needs to ignore these values and return appropriate replacements.
    audioTaskOffset.set_task_offset(0);
    audioTaskOffset.set_server_identifier(0);

    auto chan = grpc::CreateCustomChannel("127.0.0.1:" + std::to_string(portToUse), grpc::InsecureChannelCredentials(),
                                          channelArgs);
    stub = KholorsHeadlessAudioBroadcast::NewStub(chan);
}

void Client::changeDestinationPort(uint32_t port)
{
    std::lock_guard lock(mutex);
    if (lastPortUsed == port)
    {
        return;
    }
    lastPortUsed = port;

    auto chan =
        grpc::CreateCustomChannel("127.0.0.1:" + std::to_string(port), grpc::InsecureChannelCredentials(), channelArgs);
    stub = KholorsHeadlessAudioBroadcast::NewStub(chan);
}

void Client::tryReconnect()
{
    std::lock_guard lock(mutex);
    auto chan = grpc::CreateCustomChannel("127.0.0.1:" + std::to_string(lastPortUsed),
                                          grpc::InsecureChannelCredentials(), channelArgs);
    stub = KholorsHeadlessAudioBroadcast::NewStub(chan);
}

bool Client::sendAudioSegment(const AudioSegmentPayload *payload)
{
    std::lock_guard lock(mutex);

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    AudioSegmentUploadResponse reply;

    grpc::Status status = stub->UploadAudioSegment(&context, *payload, &reply);
    return status.ok();
}

std::vector<std::shared_ptr<Task>> Client::getNextAudioEvents()
{
    std::lock_guard lock(mutex);

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    grpc::Status status = stub->GetAudioTasks(&context, audioTaskOffset, &taskListResponseBuffer);

    std::vector<std::shared_ptr<Task>> tasks;

    if (status.ok())
    {
        // we save the bpm and time signature to compare after the loop if we need a bpm
        // or time signature update task
        float previousBpm = knownBpm;
        int previousTimeSignatureNominator = knownTimeSignatureNumerator;

        tasks.reserve(taskListResponseBuffer.fft_to_draw_tasks_size() * 2);
        // this offset will be used in the next query for the server to know the datums that were
        // not yet received by this instance
        audioTaskOffset.set_task_offset(taskListResponseBuffer.new_offset());
        audioTaskOffset.set_server_identifier(taskListResponseBuffer.server_identifier());

        TrackInfo currentTrackInfo;

        // for each FFT received
        for (size_t i = 0; i < taskListResponseBuffer.fft_to_draw_tasks_size(); i++)
        {
            auto currentFft = taskListResponseBuffer.fft_to_draw_tasks()[i];

            // check against the track info store to know if updates for track name/color should trigger tasks
            // and eventually do it
            currentTrackInfo.name = currentFft.track_name();
            currentTrackInfo.identifier = currentFft.track_identifier();
            ColorContainer trackColor(currentFft.track_color());
            currentTrackInfo.redColorLevel = trackColor.red;
            currentTrackInfo.greenColorLevel = trackColor.green;
            currentTrackInfo.blueColorLevel = trackColor.blue;
            currentTrackInfo.alphaColorLevel = trackColor.alpha;
            bool trackInfoUpdated = storeTrackInfo(currentTrackInfo);
            if (trackInfoUpdated)
            {
                auto trackInfoUpdateTask = std::make_shared<TrackInfoUpdateTask>(
                    currentTrackInfo.identifier, currentTrackInfo.name, currentTrackInfo.redColorLevel,
                    currentTrackInfo.greenColorLevel, currentTrackInfo.blueColorLevel);
                tasks.push_back(trackInfoUpdateTask);
            }

            // check if time signature or bpm have updated since last time
            if (std::abs(knownBpm - float(currentFft.daw_bpm())) > std::numeric_limits<float>::epsilon())
            {
                knownBpm = currentFft.daw_bpm();
            }
            if (knownTimeSignatureNumerator != currentFft.daw_time_signature_numerator())
            {
                knownTimeSignatureNumerator = currentFft.daw_time_signature_numerator();
            }

            // copy the FFT data in a preallocated buffer (or allocate one if none is available)
            auto fftData = extractFftDatums(currentFft);

            // make FFT tasks that use preallocated vectors
            auto newFftTask = std::make_shared<NewFftDataTask>(
                currentFft.track_identifier(), currentFft.total_no_channels(), currentFft.channel_index(),
                currentFft.sample_rate(), currentFft.segment_start_sample(), currentFft.segment_sample_length(),
                currentFft.no_ffts(), currentFft.fft_data(), currentFft.sent_time_unix_ms());
            tasks.push_back(newFftTask);
        }

        // now we eventually emit bpm and time signatures update tasks
        if (std::abs(knownBpm - previousBpm) > std::numeric_limits<float>::epsilon())
        {
            auto bpmUpdateTask = std::make_shared<BpmUpdateTask>(knownBpm);
            tasks.push_back(bpmUpdateTask);
        }
        if (knownTimeSignatureNumerator != previousTimeSignatureNominator)
        {
            auto timeSignatureUpdateTask = std::make_shared<TimeSignatureUpdateTask>(knownTimeSignatureNumerator);
            tasks.push_back(timeSignatureUpdateTask);
        }
    }

    return tasks;
}

std::shared_ptr<std::vector<float>> Client::extractFftDatums(FftToDrawTask &receivedTask)
{
    // if preallocated result buffers are empty, reallocate a batch of vectors
    if (preallocatedFftResultArrays.size() == 0)
    {
        for (size_t i = 0; i < SEGMENT_PREALLOCATION_BATCH_SIZE; i++)
        {
            auto newVector = std::make_shared<std::vector<float>>();
            newVector->reserve(AUDIO_SEGMENTS_FFTS_PREALLOC_SIZE);
            preallocatedFftResultArrays.push(newVector);
        }
    }
    auto newVector = preallocatedFftResultArrays.front();
    preallocatedFftResultArrays.pop();

    newVector->resize(receivedTask.fft_data_size());
    for (size_t i = 0; i < receivedTask.fft_data_size(); i++)
    {
        (*newVector)[i] = receivedTask.fft_data(i);
    }
    return newVector;
}

bool Client::taskHandler(std::shared_ptr<Task> task)
{
    // handle FftResultVectorReuseTask to reuse the vector where we store FFT results
    // returned from the server.
    auto fftResultArrayReuseTask = std::dynamic_pointer_cast<FftResultVectorReuseTask>(task);
    if (fftResultArrayReuseTask != nullptr && !fftResultArrayReuseTask->isCompleted() &&
        !fftResultArrayReuseTask->hasFailed())
    {
        std::lock_guard lock(mutex);
        preallocatedFftResultArrays.push(fftResultArrayReuseTask->resultArray);
        fftResultArrayReuseTask->setCompleted(true);
        return true;
    }

    return false;
}

std::vector<TrackInfo> Client::getAllKnownTrackInfos()
{
    std::lock_guard lock(mutex);
    std::vector<TrackInfo> trackInfos;
    trackInfos.reserve(knownTrackInfos.size());
    for (auto currentTrack = knownTrackInfos.begin(); currentTrack != knownTrackInfos.end(); currentTrack++)
    {
        trackInfos.push_back(currentTrack->second);
    }
    return trackInfos;
}

float Client::getKnownBpm()
{
    std::lock_guard lock(mutex);
    return knownBpm;
}

int Client::getKnownTimeSignature()
{
    std::lock_guard lock(mutex);
    return knownTimeSignatureNumerator;
}

bool Client::storeTrackInfo(TrackInfo &ti)
{
    // check against the map, store the value, return true if updated, false if no change
    auto existingTrack = knownTrackInfos.find(ti.identifier);
    if (existingTrack == knownTrackInfos.end() || existingTrack->second != ti)
    {
        knownTrackInfos.emplace(std::pair<uint64_t, TrackInfo>(ti.identifier, ti));
        return true;
    }
    return false;
}

void Client::freePreallocatedFftResultBuffers()
{
    std::lock_guard lock(mutex);
    std::queue<std::shared_ptr<std::vector<float>>> newQueue;
    preallocatedFftResultArrays.swap(newQueue);
}