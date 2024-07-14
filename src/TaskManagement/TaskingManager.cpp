#include "TaskingManager.h"
#include "Task.h"
#include "TaskListener.h"
#include "spdlog/spdlog.h"
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

TaskingManager::TaskingManager() : lastUsedTaskListenerId(-1)
{
    backgroundThreadIsRunning = false;
    taskBroadcastStopped = true;
    historyNextIndex = 0;
    registerTaskListener(this);
}

TaskingManager::~TaskingManager()
{
    std::lock_guard<std::mutex> lock(taskingThreadStartMutex);

    if (taskingThread != nullptr)
    {
        {
            std::lock_guard<std::mutex> lock2(taskQueueMutex);
            taskBroadcastStopped = true;
        }
        taskingThreadCV.notify_all();
        taskingThread->join();
        taskingThread.reset(nullptr);
        spdlog::info("TaskingManager tasking thread has been stopped");
    }
    spdlog::info("TaskingManager destroyed");
}

void TaskingManager::startTaskBroadcast()
{
    std::lock_guard<std::mutex> lock(taskingThreadStartMutex);

    if (taskingThread == nullptr)
    {
        {
            std::lock_guard<std::mutex> lock2(taskQueueMutex);
            taskBroadcastStopped = false;
        }
        taskingThread = std::make_unique<std::thread>(&TaskingManager::taskingThreadLoop, this);
        spdlog::info("TaskingManager tasking thread has been started");
    }
    else
    {
        spdlog::warn(
            "TaskingManager's startTaskBroadcast was called even though the current tasking thread is still running.");
    }
}

void TaskingManager::stopTaskBroadcast()
{
    std::lock_guard<std::mutex> lock(taskingThreadStartMutex);

    if (taskingThread != nullptr)
    {
        {
            std::lock_guard<std::mutex> lock2(taskQueueMutex);
            taskBroadcastStopped = true;
        }
        taskingThreadCV.notify_all();
        taskingThread->join();
        taskingThread.reset(nullptr);
        spdlog::info("TaskingManager tasking thread has been stopped");
    }
    else
    {
        spdlog::warn(
            "TaskingManager's stopTaskBroadcast was called even though no tasking thread is currently running.");
    }
}

void TaskingManager::taskingThreadLoop()
{
    backgroundThreadIsRunning = true;
    size_t lastQueueSize;
    std::shared_ptr<Task> currentTask;
    // looping on successive wake ups from condition variable (or timeouts thereof)
    while (true)
    {
        // looping on the tasks that remains in the task queue (with potential exit if asked to stop)
        while (true)
        {
            // fetch size of queue
            {
                std::lock_guard<std::mutex> lock(taskQueueMutex);
                lastQueueSize = taskQueue.size();
                // abort if the thread is currently being stopped
                if (taskBroadcastStopped)
                {
                    backgroundThreadIsRunning = false;
                    return;
                }
            }
            // if size == 0 or task broadcast was stopped, break
            if (lastQueueSize <= 0)
            {
                break;
            }
            // if size > 0, pull and process task
            {
                std::lock_guard<std::mutex> lock(taskQueueMutex);
                currentTask = taskQueue.front();
                taskQueue.pop();
            }
            // this should not happen because we are the only thread reading/popin the queue.
            if (currentTask == nullptr)
            {
                spdlog::warn("A queue item was removed since the tasking thread last fetched size. This should not "
                             "happen.");
                break;
            }

            // core tasking code that iterate over listeners
            {
                std::lock_guard<std::mutex> lock(taskListenersMutex);
                for (size_t i = 0; i < taskListeners.size(); i++)
                {
                    bool shouldStop = taskListeners[i]->taskHandler(currentTask);
                    if (shouldStop)
                    {
                        break;
                    }
                }
            }
            if (currentTask->goesInTaskHistory() && currentTask->isCompleted() && !currentTask->hasFailed())
            {
                recordTaskInHistory(currentTask);
            }
            else
            {
                spdlog::debug("Task not going to task history: " + currentTask->marshal());
            }
        }

        // wait on condition variable with timeout
        std::unique_lock<std::mutex> queueLock(taskQueueMutex);
        taskingThreadCV.wait_for(queueLock, std::chrono::seconds(1),
                                 [this] { return taskBroadcastStopped || taskQueue.size() > 0; });
    }
}

void TaskingManager::broadcastTask(std::shared_ptr<Task> submittedTask)
{
    {
        std::lock_guard<std::mutex> lock(taskQueueMutex);
        taskQueue.push(submittedTask);
    }
    taskingThreadCV.notify_all();
}

int64_t TaskingManager::registerTaskListener(TaskListener *newListener)
{
    std::lock_guard<std::mutex> lock(taskListenersMutex);
    int64_t newId = lastUsedTaskListenerId + 1;
    lastUsedTaskListenerId = newId;
    taskListeners.push_back(newListener);
    taskListenersIds.push_back(newId);
    return newId;
}

void TaskingManager::purgeTaskListener(int64_t idToRemove)
{
    std::lock_guard<std::mutex> lock(taskListenersMutex);

    std::vector<TaskListener *> newTaskListeners;
    std::vector<int64_t> newTaskListersIds;

    newTaskListeners.reserve(taskListeners.size());
    newTaskListersIds.reserve(taskListeners.size());

    for (size_t i = 0; i < taskListeners.size(); i++)
    {
        if (taskListenersIds[i] != idToRemove)
        {
            newTaskListeners.push_back(taskListeners[i]);
            newTaskListersIds.push_back(taskListenersIds[i]);
        }
    }

    taskListeners.swap(newTaskListeners);
    taskListenersIds.swap(newTaskListersIds);
}

void TaskingManager::broadcastNestedTaskNow(std::shared_ptr<Task> priorityTask)
{
    throwIfCallerIsNotTaskingThread("broadcastNestedTaskNow");

    // NOTE: we don't record nested tasks in history

    for (size_t i = 0; i < taskListeners.size(); i++)
    {
        bool shouldStop = taskListeners[i]->taskHandler(priorityTask);
        if (shouldStop)
        {
            break;
        }
    }

    spdlog::trace("Nested task executed: " + priorityTask->marshal());
}

void TaskingManager::throwIfCallerIsNotTaskingThread(std::string caller)
{
    std::lock_guard<std::mutex> lock(taskingThreadStartMutex);
    if (std::this_thread::get_id() != taskingThread->get_id())
    {
        throw std::runtime_error("Trying to call " + caller +
                                 " from outside the tasking thread. It can only "
                                 "be called from within the TaskListeners's taskHandler code.");
    }
}

bool TaskingManager::taskHandler(std::shared_ptr<Task> task)
{
    // Note: As the tasking manager is the first in the list of listeners, any completed
    // task will reach all other TaskListeners if we return false.
    // So this is the only task implementation where we don't need to use broadcastNestedTaskNow
    // to let others now of success.

    auto cancelTask = std::dynamic_pointer_cast<CancelTask>(task);
    if (cancelTask != nullptr && !cancelTask->isCompleted() && !cancelTask->hasFailed())
    {
        bool success = undoLastActivity();
        if (!success)
        {
            cancelTask->setFailed(true);
            cancelTask->setCompleted(false);
            return false;
        }
        else
        {
            cancelTask->setFailed(false);
            cancelTask->setCompleted(true);
            return false;
        }
    }

    auto restoreTask = std::dynamic_pointer_cast<RestoreTask>(task);
    if (restoreTask != nullptr && !restoreTask->isCompleted() && !restoreTask->hasFailed())
    {
        bool success = redoLastActivity();
        if (!success)
        {
            restoreTask->setFailed(true);
            restoreTask->setCompleted(false);
            return false;
        }
        else
        {
            restoreTask->setFailed(false);
            restoreTask->setCompleted(true);
            return false;
        }
    }

    auto clearHistoryTask = std::dynamic_pointer_cast<ClearHistoryTask>(task);
    if (clearHistoryTask != nullptr && !clearHistoryTask->isCompleted() && !clearHistoryTask->hasFailed())
    {
        clearTaskHistory();
        clearHistoryTask->setFailed(false);
        clearHistoryTask->setCompleted(true);
        return false;
    }

    return false;
}

void TaskingManager::recordTaskInHistory(std::shared_ptr<Task> taskToRecord)
{
    throwIfCallerIsNotTaskingThread("recordTaskInHistory");

    // we always empty the canceled task stack
    // after a new task is performed to
    // ensure "ctrl + shift + z" type calls
    // won't restore action that came before
    // new activity
    while (!canceledTasks.empty())
    {
        canceledTasks.pop();
    }

    history[historyNextIndex] = taskToRecord;
    historyNextIndex = (historyNextIndex + 1) % ACTIVITY_HISTORY_RING_BUFFER_SIZE;

    spdlog::debug("Task recorded in history: " + taskToRecord->marshal());
}

bool TaskingManager::undoLastActivity()
{
    throwIfCallerIsNotTaskingThread("undoLastActivity");

    bool cancelingNextTask = true;

    while (cancelingNextTask)
    {
        int lastActivityIndex = (historyNextIndex - 1) % ACTIVITY_HISTORY_RING_BUFFER_SIZE;
        if (lastActivityIndex < 0)
        {
            lastActivityIndex += ACTIVITY_HISTORY_RING_BUFFER_SIZE;
        }

        if (history[lastActivityIndex] == nullptr)
        {
            spdlog::info("Trying to cancel last task but there is no activity in history");
            return false;
        }

        // we are saving the group index so that we can decide on canceling next
        // task if it's from the same task group
        int taskGroupIndex = history[lastActivityIndex]->getTaskGroupIndex();

        auto tasksToCancel = history[lastActivityIndex]->getOppositeTasks();
        if (tasksToCancel.size() == 0)
        {
            spdlog::info("This task cannot be canceled (no opposite task set)");
            return false;
        }

        for (size_t i = 0; i < tasksToCancel.size(); i++)
        {
            tasksToCancel[i]->declareSelfAsPartOfReversion();
            for (size_t j = 0; j < taskListeners.size(); j++)
            {
                bool shouldStop = taskListeners[j]->taskHandler(tasksToCancel[i]);
                if (shouldStop)
                {
                    break;
                }
            }
            spdlog::debug("Canceled last task: " + tasksToCancel[i]->marshal());
        }

        canceledTasks.push(history[lastActivityIndex]);

        history[lastActivityIndex] = nullptr;
        historyNextIndex = lastActivityIndex;

        // if the next task exists and has same task group id, we cancel it as well by calling us again
        lastActivityIndex = (historyNextIndex - 1) % ACTIVITY_HISTORY_RING_BUFFER_SIZE;
        if (lastActivityIndex < 0)
        {
            lastActivityIndex += ACTIVITY_HISTORY_RING_BUFFER_SIZE;
        }
        if (history[lastActivityIndex] != nullptr && history[lastActivityIndex]->getTaskGroupIndex() == taskGroupIndex)
        {
            cancelingNextTask = true;
        }
        else
        {
            cancelingNextTask = false;
        }
    }

    return true;
}

bool TaskingManager::redoLastActivity()
{
    throwIfCallerIsNotTaskingThread("redoLastActivity");

    bool restoringNextTask = true;

    while (restoringNextTask)
    {

        if (canceledTasks.empty())
        {
            spdlog::info("Nothing to restore.");
            return false;
        }

        std::shared_ptr<Task> taskToRestore = canceledTasks.top();
        canceledTasks.pop();

        int taskGroupIndex = taskToRestore->getTaskGroupIndex();

        taskToRestore->prepareForRepost();
        taskToRestore->preventFromGoingToTaskHistory();
        taskToRestore->declareSelfAsPartOfReversion();

        for (size_t j = 0; j < taskListeners.size(); j++)
        {
            bool shouldStop = taskListeners[j]->taskHandler(taskToRestore);
            if (shouldStop)
            {
                break;
            }
        }

        history[historyNextIndex] = taskToRestore;
        historyNextIndex = (historyNextIndex + 1) % ACTIVITY_HISTORY_RING_BUFFER_SIZE;

        spdlog::debug("Restored canceled task: " + taskToRestore->marshal());

        // if the next canceled task exists and has same task group id, we restore it as well
        if (!canceledTasks.empty() && canceledTasks.top()->getTaskGroupIndex() == taskGroupIndex)
        {
            restoringNextTask = true;
        }
        else
        {
            restoringNextTask = false;
        }
    }

    return true;
}

void TaskingManager::clearTaskHistory()
{
    throwIfCallerIsNotTaskingThread("clearTaskHistory");

    std::stack<std::shared_ptr<Task>> emptyTaskStack;
    canceledTasks.swap(emptyTaskStack);

    // this will free a lot of stuff since we dereference smart pointers
    for (int i = 0; i < ACTIVITY_HISTORY_RING_BUFFER_SIZE; i++)
    {
        history[i] = nullptr;
    }

    // this is quite useless but I like it like this
    historyNextIndex = 0;

    spdlog::debug("Cleared task history");
}

void TaskingManager::shutdownBackgroundThreadAsync()
{
    {
        std::lock_guard<std::mutex> lock2(taskQueueMutex);
        taskBroadcastStopped = true;
    }
    taskingThreadCV.notify_all();
}

bool TaskingManager::isBackgroundThreadRunning()
{
    return backgroundThreadIsRunning;
}
