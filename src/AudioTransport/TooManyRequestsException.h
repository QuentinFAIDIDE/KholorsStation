#pragma once

#include <exception>
#include <string>

namespace AudioTransport
{

class TooManyRequestsException : public std::exception
{
  public:
    explicit TooManyRequestsException(const std::string &message) : msg("Too many requests: " + message)
    {
    }

    const char *what() const noexcept override
    {
        return msg.c_str();
    }

  private:
    std::string msg;
};

}; // namespace AudioTransport