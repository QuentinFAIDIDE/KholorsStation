#pragma once

#include <string>

/**
 * @brief      Virtual class to be inherited that defines how an objects can
 *             dump its state into a string file and load back its state from it.
 *             As of right now we use json strings for that.
 */
class Marshalable
{
  public:
    virtual std::string marshal() = 0;
    virtual void unmarshal(std::string &) = 0;
};