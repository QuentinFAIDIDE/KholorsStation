#pragma once

#include "AudioTransport.pb.h"
#include "AudioTransport/AudioSegmentPayloadSender.h"
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <vector>
namespace AudioTransport
{

class MockedAudioSegmentPayloadSender : public AudioSegmentPayloadSender
{
  public:
    ~MockedAudioSegmentPayloadSender()
    {
        spdlog::debug("Destroyed mocked segment sender");
    }

    bool sendAudioSegment(const AudioSegmentPayload *payload) override
    {
        std::lock_guard lock(mutex);
        receivedAudioSegments.emplace_back(std::make_shared<AudioSegmentPayload>());
        receivedAudioSegments[receivedAudioSegments.size() - 1]->CopyFrom(*payload);
        spdlog::debug("Appended a payload to the mocked buffer");
        return true;
    }

    std::vector<std::shared_ptr<AudioSegmentPayload>> getAllReceivedSegments()
    {
        spdlog::debug("Querying a copy of mocked sent payloads");
        std::lock_guard lock(mutex);
        spdlog::debug("Got lock for a copy of mocked sent payloads");
        return receivedAudioSegments;
    }

    void tryReconnect() override
    {
    }

  private:
    std::vector<std::shared_ptr<AudioSegmentPayload>> receivedAudioSegments;
    std::mutex mutex;
};

} // namespace AudioTransport