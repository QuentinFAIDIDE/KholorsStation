#include "TaskingManager.h"
#include <iostream>
#include <memory>

TaskingManager::TaskingManager()
{
    taskBroadcastStopped = false;
    historyNextIndex = 0;
}

TaskingManager::~TaskingManager()
{
}

void TaskingManager::stopTaskBroadcast()
{
    // TODO: const juce::SpinLock::ScopedLockType lock(broadcastLock);
    taskBroadcastStopped = true;
}

void TaskingManager::broadcastTask(std::shared_ptr<Task> submittedTask)
{
    // TODO: isolate in its own thread

    // TODO: const juce::SpinLock::ScopedTryLockType lock(broadcastLock);
    if (taskBroadcastStopped)
    {
        return;
    }

    {
        // TODO: const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
        taskQueue.push(submittedTask);
    }

    /* TODO
    if (!lock.isLocked())
    {
        return;
    }
    */

    std::shared_ptr<Task> taskToBroadcast(nullptr);

    // all broadcasted tasks arrive on the message thread
    // const juce::MessageManagerLock mmLock;

    {
        // TODO: const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
        taskToBroadcast = taskQueue.front();
        taskQueue.pop();
    }

    while (taskToBroadcast != nullptr)
    {
        for (size_t i = 0; i < taskListeners.size(); i++)
        {
            bool shouldStop = taskListeners[i]->taskHandler(taskToBroadcast);
            if (shouldStop)
            {
                break;
            }
        }

        if (taskToBroadcast->goesInTaskHistory() && taskToBroadcast->isCompleted() && !taskToBroadcast->hasFailed())
        {
            recordTaskInHistory(taskToBroadcast);
        }
        else
        {
            std::cout << "Unrecorded task: " << taskToBroadcast->marshal() << std::endl;
        }

        taskToBroadcast = std::shared_ptr<Task>(nullptr);
        {
            // TODO: const juce::SpinLock::ScopedLockType queueLock(taskQueueLock);
            if (!taskQueue.empty())
            {
                taskToBroadcast = taskQueue.front();
                taskQueue.pop();
            }
        }
    }
}

void TaskingManager::registerTaskListener(TaskListener *newListener)
{
    // TODO: const juce::SpinLock::ScopedLockType lock(broadcastLock);
    taskListeners.push_back(newListener);
}

void TaskingManager::broadcastNestedTaskNow(std::shared_ptr<Task> priorityTask)
{
    // yet another warning: don't call this function from anything else
    // than an already running taskHandler !

    // NOTE: we don't record nested tasks in history

    for (size_t i = 0; i < taskListeners.size(); i++)
    {
        bool shouldStop = taskListeners[i]->taskHandler(priorityTask);
        if (shouldStop)
        {
            break;
        }
    }
}

void TaskingManager::recordTaskInHistory(std::shared_ptr<Task> taskToRecord)
{
    // This should only be called by the broadcastTask function
    // when it has lock message thread and broadcasting lock.

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

    std::cout << "Recorded task: " << taskToRecord->marshal() << std::endl;
}

void TaskingManager::undoLastActivity()
{
    // TODO: juce::SpinLock::ScopedLockType bLock(broadcastLock);

    bool cancelingNextTask = true;

    while (cancelingNextTask)
    {
        int lastActivityIndex = (historyNextIndex - 1) % ACTIVITY_HISTORY_RING_BUFFER_SIZE;

        if (history[lastActivityIndex] == nullptr)
        {
            std::cout << "No operation to cancel" << std::endl;
            return;
        }

        // we are saving the group index so that we can decide on canceling next
        // task if it's from the same task group
        int taskGroupIndex = history[lastActivityIndex]->getTaskGroupIndex();

        auto tasksToCancel = history[lastActivityIndex]->getOppositeTasks();
        if (tasksToCancel.size() == 0)
        {
            std::cout << "Operation cannot be canceled" << std::endl;
            return;
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
            std::cout << "Reversion task: " << tasksToCancel[i]->marshal() << std::endl;
        }

        canceledTasks.push(history[lastActivityIndex]);

        history[lastActivityIndex] = nullptr;
        historyNextIndex = lastActivityIndex;

        // if the next task exists and has same task group id, we cancel it as well by calling us again
        lastActivityIndex = (historyNextIndex - 1) % ACTIVITY_HISTORY_RING_BUFFER_SIZE;
        if (history[lastActivityIndex] != nullptr && history[lastActivityIndex]->getTaskGroupIndex() == taskGroupIndex)
        {
            cancelingNextTask = true;
        }
        else
        {
            cancelingNextTask = false;
        }
    }
}

void TaskingManager::redoLastActivity()
{
    // always make sure no other thread is currently broadcasting events
    // (also prevents calls from task broadcasting loop)
    // TODO: juce::SpinLock::ScopedLockType bLock(broadcastLock);

    bool restoringNextTask = true;

    while (restoringNextTask)
    {

        if (canceledTasks.empty())
        {
            return;
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

        std::cout << "Restored task: " << taskToRestore->marshal() << std::endl;

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
}

void TaskingManager::clearTaskHistory()
{
    std::stack<std::shared_ptr<Task>> emptyTaskStack;
    canceledTasks.swap(emptyTaskStack);

    // this will free a lot of stuff since we dereference smart pointers
    for (int i = 0; i < ACTIVITY_HISTORY_RING_BUFFER_SIZE; i++)
    {
        history[i] = nullptr;
    }

    // this is quite useless but I like it like this
    historyNextIndex = 0;

    std::cout << "cleared history" << std::endl;
}