#ifndef DEF_ACTIVITY_MANAGER_HPP
#define DEF_ACTIVITY_MANAGER_HPP

#include "TaskListener.h"
#include <memory>
#include <queue>
#include <stack>

#define ACTIVITY_HISTORY_RING_BUFFER_SIZE 4096

/**
Class responsible for app activity, for example tasks and history.
*/
class TaskingManager
{
  public:
    /**
     Creates a new activity manager
     */
    TaskingManager();

    /**
     Deletes the activity manager
     */
    ~TaskingManager();

    /**
     Add the TaskListener class to a list of objects which gets
     their callback called with the tasks when they are broadcasted.
     */
    void registerTaskListener(TaskListener *);

    /**
       Take the task broadcasting lock and set its status
       to stopped so no further task is processed.
     */
    void stopTaskBroadcast();

    /**
     Broadcast the task to all listeners.
     */
    void broadcastTask(std::shared_ptr<Task>);

    /**
     This should only be called from an already running taskHandler.
     Will call another taskHandler and jump the queue of tasks.
     */
    void broadcastNestedTaskNow(std::shared_ptr<Task>);

    /**
     Append this task to history ring buffer.
     */
    void recordTaskInHistory(std::shared_ptr<Task>);

    /**
     Undo the last activity.
     */
    void undoLastActivity();

    /**
     Redo the stored last activities that were undone.
     */
    void redoLastActivity();

    /**
     * @brief      Clear all the tasks in history. Mostly used
     *             to prevent calling undo after a commit from AppState.
     *             We don't want users to undo commited changes to prevent
     *             situations where the app crash and we cannot replay history at restart
     *             because an undo has restored a pointer to a previously deleted
     *             ressource that was not saved on disk and was persisted in the task.
     *             Beware of race condition with history and canceledTasks if you decide
     *             to call this function from a new place (it is currently called when the lock
     *             for broadcastLock is locked, so we don't take it!).
     */
    void clearTaskHistory();

  private:
    std::shared_ptr<Task> history[ACTIVITY_HISTORY_RING_BUFFER_SIZE]; /**< ring buffer with the last executed tasks */
    int historyNextIndex;                            /**<  the index of the next recorded history entry */
    std::stack<std::shared_ptr<Task>> canceledTasks; //**<  stack of canceled tasks */

    std::vector<TaskListener *> taskListeners;   /**<  a list of object we broadcast tasks to */
    std::queue<std::shared_ptr<Task>> taskQueue; /**<  the queue of tasks to be broadcasted */
    // TODO: juce::SpinLock taskQueueLock;                /**<  the lock to prevent race condition on the task queue */
    // TODO: juce::SpinLock broadcastLock;                /**<  the lock to prevent concurrent broadcast of tasks */
    bool taskBroadcastStopped; /**<  boolean to tell if the broadcasting processing is enabled */
};

#endif // DEF_ACTIVITY_MANAGER_HPP
