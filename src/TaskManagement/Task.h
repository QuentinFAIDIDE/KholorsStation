#ifndef DEF_TASK_HPP
#define DEF_TASK_HPP

#include "Marshalable.h"
#include <memory>
#include <vector>

#define MAX_TASK_INDEX 1048576

#define RESET_TASK_NO_STEPS 3

/**
  Unused as of now.
 */
class State
{
};

/**
  When a sample is created, it's one
  of those duplication types.
 */
enum DuplicationType
{
    DUPLICATION_TYPE_NO_DUPLICATION,    // plain new sample
    DUPLICATION_TYPE_COPY_AT_POSITION,  // copied from another and moved to a certain position
    DUPLICATION_TYPE_SPLIT_AT_POSITION, // take an existing sample, reduce it to be before some position, and create the
                                        // cropped out part after as a new sample
    DUPLICATION_TYPE_SPLIT_AT_FREQUENCY, // take an existing sample, filter it to be below some frequency, and create a
                                         // new sample with the filtered out part
};

/**
  Abstract class for "Tasks", which are actions to perform or that were
  already performed. These Tasks are sent down the ActivityManager
  broadcasting functions to TaskListeners.

  The goals are to isolate the classes by having a shared task channel (less deps injection),
  allowing for easy task broadcast by any new class (eg. imagine a
  new rpc api working on top of the UI), and to allow recording and reverting tasks
  in history.
 */
class Task : public Marshalable
{
  public:
    /**
      Constructor
     */
    Task();

    /**
      Virtual destructor
     */
    virtual ~Task();

    /**
      Convert the task into a string.
     */
    std::string marshal() override;

    /**
      Parse the task from a string.
     */
    void unmarshal(std::string &) override;

    /**
      Has the task been completed already ?
     */
    bool isCompleted();

    /**
      Set the completion state of the task.
      @param c true if task was completed, false otherwise
     */
    void setCompleted(bool c);

    /**
      Set the failed state of the task.
      @param bool true if the task has failed, false otherwise.
     */
    void setFailed(bool);

    /**
      Has that task failed ?
     */
    bool hasFailed();

    /**
      Should this task be recorded in history ?
     */
    bool goesInTaskHistory();

    /**
      Get the sequence of opposite task from this one, eg the ones
      that when performed cancels out this task.
      If this is not possible, the array is empty.
      The tasks are performed in order, ie lowest ids first.
     */
    virtual std::vector<std::shared_ptr<Task>> getOppositeTasks();

    /**
     Declare this task as part of a reversion (eg obtained with getOppositeTasks).
     */
    void declareSelfAsPartOfReversion();

    /**
     Declare this task as not going into the task history (used for restoring canceled actions mostly)
     */
    void preventFromGoingToTaskHistory();

    /**
     Some tasks default to not going in task history, beware, and use this if you
     create them with the bad constructor. I promise I'm actively considering
     better designs for the shitty tasks :(
     Declare this task as going into the task history.
     */
    void forceGoingToTaskHistory();

    /**
     Called when the task is about to be posted a second time after a rewind. Example of specific use case:
     SampleCreateTask to reuse ids.
     */
    virtual void prepareForRepost();

    /**
     * @brief      Gets the task group index.
     *
     * @return     The task group index.
     */
    int getTaskGroupIndex();

    /**
     * @brief      Sets the task group index.
     *
     * @param[in]  newTaskGroupIndex  The new task group index
     */
    void setTaskGroupIndex(int newTaskGroupIndex);

    /**
     * @brief      Sets the task group to same of the target.
     *
     * @param      targeTask  The target task
     */
    void setTaskGroupToTarget(Task &targetTask);

    /**
     * @brief      Gets a new task group index (incremented from task index counter).
     *
     * @return     The new task group index.
     */
    static int getNewTaskGroupIndex();

  protected:
    bool recordableInHistory; // tell if this task should be saved in history. Inherit SilentTask to have it false.
    bool isPartOfReversion;   // tells if this task was obtained through getOppositeTasks

  private:
    bool completed;           // has this task been completed/performed
    bool failed;              // has this task failed
    std::string errorMessage; // an eventual error messaged for the task failure

    int taskGroupIndex; // a unique identifier to group tasks together

    static int taskGroupIndexIterator; // a static value that is incremented and assigned to new task, and also wrapped
                                       // at MAX_TASK_INDEX
};

/**
 * @brief      This class describes a silent task. This is a task
 *             that defaults to not being recordable in history.
 */
class SilentTask : public Task
{
  public:
    SilentTask();
};

#endif // DEF_TASK_HPP