#include "Task.h"

int Task::taskGroupIndexIterator = 0;

#include <nlohmann/json.hpp>
using nlohmann::json;

Task::Task()
{
    currentTaskingManager = nullptr;
    completed = false;
    failed = false;

    // default to record tasks in history.
    // if you don't want to, inherit SilentTask
    recordableInHistory = true;

    isPartOfReversion = false;

    taskGroupIndex = getNewTaskGroupIndex();
}

Task::~Task() = default;

std::string Task::marshal()
{
    json const taskj = {
        {"object", "task"},
        {"task", "generic_task"},
        {"is_completed", completed},
        {"failed", failed},
        {"recordable_in_history", recordableInHistory},
        {"reversed", false},
    };
    return taskj.dump();
}

void Task::unmarshal(std::string & /*unused*/)
{
}

bool Task::isCompleted() const
{
    return completed;
}

void Task::setCompleted(bool c)
{
    completed = c;
}

void Task::setFailed(bool f)
{
    failed = f;
}

bool Task::hasFailed() const
{
    return failed;
}

bool Task::goesInTaskHistory() const
{
    return recordableInHistory;
}

void Task::declareSelfAsPartOfReversion()
{
    isPartOfReversion = true;
}

void Task::forceGoingToTaskHistory()
{
    recordableInHistory = true;
}

void Task::preventFromGoingToTaskHistory()
{
    recordableInHistory = false;
}

void Task::prepareForRepost()
{
    completed = false;
    failed = false;
}

std::vector<std::shared_ptr<Task>> Task::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> emptyReversionTasks;
    return emptyReversionTasks;
}

int Task::getTaskGroupIndex() const
{
    return taskGroupIndex;
}

void Task::setTaskGroupIndex(int index)
{
    taskGroupIndex = index;
}

void Task::setTaskGroupToTarget(Task &targetTask)
{
    setTaskGroupIndex(targetTask.getTaskGroupIndex());
}

int Task::getNewTaskGroupIndex()
{
    int const taskGroupToReturn = taskGroupIndexIterator;

    taskGroupIndexIterator++;
    if (taskGroupIndexIterator >= MAX_TASK_INDEX)
    {
        taskGroupIndexIterator = 0;
    }

    return taskGroupToReturn;
}

TaskingManager *Task::getTaskingManager()
{
    return currentTaskingManager;
}

void Task::setTaskingManager(TaskingManager *tm)
{
    currentTaskingManager = tm;
}

// ============================

SilentTask::SilentTask()
{
    recordableInHistory = false;
}

// =============================

std::string CancelTask::marshal()
{
    json const taskj = {
        {"object", "task"},
        {"task", "cancel_task"},
        {"is_completed", isCompleted()},
        {"failed", hasFailed()},
        {"recordable_in_history", recordableInHistory},
        {"is_part_of_reversion", isPartOfReversion},
    };
    return taskj.dump();
}

std::string RestoreTask::marshal()
{
    json const taskj = {
        {"object", "task"},
        {"task", "restore_task"},
        {"is_completed", isCompleted()},
        {"failed", hasFailed()},
        {"recordable_in_history", recordableInHistory},
        {"is_part_of_reversion", isPartOfReversion},
    };
    return taskj.dump();
}

std::string ClearHistoryTask::marshal()
{
    json const taskj = {
        {"object", "task"},
        {"task", "clear_history_task"},
        {"is_completed", isCompleted()},
        {"failed", hasFailed()},
        {"recordable_in_history", recordableInHistory},
        {"is_part_of_reversion", isPartOfReversion},
    };
    return taskj.dump();
}

///////////////////////////////////////

std::string QuittingTask::marshal()
{
    json const taskj = {
        {"object", "task"},
        {"task", "quitting"},
        {"is_completed", isCompleted()},
        {"failed", hasFailed()},
        {"recordable_in_history", recordableInHistory},
        {"is_part_of_reversion", isPartOfReversion},
    };
    return taskj.dump();
}