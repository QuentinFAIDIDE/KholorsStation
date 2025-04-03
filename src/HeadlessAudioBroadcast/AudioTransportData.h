#pragma once

namespace HeadlessAudioBroadcast
{

/**
 * @brief udioTransportData is an abstract class representing
 * various types of data the server can emit.
 *
 */
class AudioTransportData
{
  public:
    virtual ~AudioTransportData() = default;
};

} // namespace HeadlessAudioBroadcast