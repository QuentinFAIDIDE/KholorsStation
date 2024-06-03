#pragma once

#include <exception>
#include <string>

namespace AudioTransport
{

class TooManyRequestsException : public std::exception
{
  public:
    TooManyRequestsException(const std::string &message) : msg("Too many requests: " + message)
    {
    }

    virtual const char *what() const throw()
    {
        return msg.c_str();
    }

  private:
    std::string msg;
};

}; // namespace AudioTransport