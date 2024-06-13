#pragma once

#include "AudioTransport.pb.h"
namespace AudioTransport
{

class AudioSegmentPayloadSender
{
  public:
    virtual bool sendAudioSegment(AudioSegmentPayload *payload) = 0;
};

}; // namespace AudioTransport