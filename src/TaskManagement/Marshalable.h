#ifndef DEF_MARSHALABLE_HPP
#define DEF_MARSHALABLE_HPP

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

#endif // DEF_MARSHALABLE_HPP