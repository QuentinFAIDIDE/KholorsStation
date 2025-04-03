#pragma once

#include "HeadlessAudioBroadcast.pb.h"
namespace HeadlessAudioBroadcast
{

class AudioSegmentPayloadSender
{
  public:
    virtual bool sendAudioSegment(const AudioSegmentPayload *payload) = 0;
    virtual void tryReconnect() = 0;
};

}; // namespace HeadlessAudioBroadcast