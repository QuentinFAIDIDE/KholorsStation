#ifndef DEF_TASK_LISTENER_HPP
#define DEF_TASK_LISTENER_HPP

#include "Task.h"
#include <memory>

/**
Inherited by classes who wants to be able to receive tasks from the
Activity Manager. It also requires calling ActivityManager's registerTaskListener
for the new task listener to be called.
*/

class TaskListener
{
  public:
    /**
     * Returns true if the task don't need further broadcast.
     */
    virtual bool taskHandler(std::shared_ptr<Task> task) = 0;
};

#endif // DEF_TASK_LISTENER_HPP