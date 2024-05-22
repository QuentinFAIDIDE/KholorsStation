#ifndef DEF_TASKING_MANAGER_HPP
#define DEF_TASKING_MANAGER_HPP

#include "TaskListener.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <stack>
#include <thread>

#define ACTIVITY_HISTORY_RING_BUFFER_SIZE 4096

/**
Class responsible for app activity, for example tasks and history.
*/
class TaskingManager : public TaskListener
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
     * Starts the task broadcasting thread. If not called
     * the task are never broadcast and just wait in the queue.
     */
    void startTaskBroadcast();

    /**
       Take the task broadcasting lock and set its status
       to stopped so no further task is processed.
     */
    void stopTaskBroadcast();

    /**
     Push a task in the queue for it to be picked and broadcasted by the
     tasking thread if it's started.
     */
    void broadcastTask(std::shared_ptr<Task>);

    /**
     This should only be called from an already running taskHandler.
     Will call another taskHandler and jump the queue of tasks.
     */
    void broadcastNestedTaskNow(std::shared_ptr<Task>);

    /**
     * @brief Task handler responsible for task related tasks.
     * This specific handler will recognize and perform CancelTask, RestoreTask and ClearHistoryTask.
     *
     * @param task The task passed down to this handler.
     * @return true This task can be passed to other TaskListeners handlers.
     * @return false This task broadcasting does not need to reach remaining TaskListeners handlers.
     */
    bool taskHandler(std::shared_ptr<Task> task);

  private:
    /**
     * The function that runs the thread
     * that depile and run tasks.
     */
    void taskingThreadLoop();

    /**
     Append this task to history ring buffer.
     */
    void recordTaskInHistory(std::shared_ptr<Task>);

    /**
     * @brief This can be used to throw a std::runtime_error when the caller is not
     * the tasking thread.
     *
     * @param caller name of the function that is calling, to be displayed in error string.
     */
    void throwIfCallerIsNotTaskingThread(std::string caller);

    /**
     Undo the last activity.
     Returns a bool to tell if it succeeded or not.
     */
    bool undoLastActivity();

    /**
     Redo the stored last activities that were undone.
     Returns a bool to tell if it succeeded or not.
     */
    bool redoLastActivity();

    /**
     *       Clear all the tasks in history. Mostly used
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

    std::shared_ptr<Task> history[ACTIVITY_HISTORY_RING_BUFFER_SIZE]; /**< ring buffer with the last executed tasks */
    int historyNextIndex;                            /**<  the index of the next recorded history entry */
    std::stack<std::shared_ptr<Task>> canceledTasks; //**<  stack of canceled tasks */
    std::vector<TaskListener *> taskListeners;       /**<  a list of object we broadcast tasks to */
    std::mutex taskListenersMutex;               /**< Mutex to prevent race condition on the vector of taskListeners */
    std::queue<std::shared_ptr<Task>> taskQueue; /**<  the queue of tasks to be broadcasted */
    bool taskBroadcastStopped;                   /**<  boolean to tell if the broadcasting processing is enabled */
    std::unique_ptr<std::thread> taskingThread;  /**< Reference to the eventually running task broadcasting thread */
    std::mutex taskQueueMutex;                   /**< mutex to synchronize queue access across different threads */
    std::mutex taskingThreadStartMutex; /**< Used to secure start and stop being called concurrently, and reading of
                                           thread ids and nullity in other functions */
    std::condition_variable taskingThreadCV; /**< Used to wake up the background thread */
};

#endif // DEF_TASKING_MANAGER_HPP
