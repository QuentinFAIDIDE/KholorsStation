#pragma once

namespace AudioTransport
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

} // namespace AudioTransport