#ifndef DEF_TOO_MANY_REQUESTS_EXCEPTION_HPP
#define DEF_TOO_MANY_REQUESTS_EXCEPTION_HPP

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

#endif // DEF_TOO_MANY_REQUESTS_EXCEPTION_HPP