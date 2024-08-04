#include "ProcessingTimer.h"
#include "StationApp/Audio/ProcessingTimerWaitgroup.h"
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

#define DEFAULT_PREALLOCATED_PROC_TIMER_WAITGROUPS 32
#define AVERAGING_TIMER_SIZE 32

ProcessingTimer::ProcessingTimer(TaskingManager &tm) : taskingManager(tm)
{
    for (size_t i = 0; i < DEFAULT_PREALLOCATED_PROC_TIMER_WAITGROUPS; i++)
    {
        auto newWg = std::make_shared<ProcessingTimerWaitgroup>(this, i);
        waitgroups.push_back(newWg);
        idleWaitgroups.push(i);
    }
}

ProcessingTimer::~ProcessingTimer()
{
    for (size_t i = 0; i < waitgroups.size(); i++)
    {
        waitgroups[i]->deactivate();
    }
}

void ProcessingTimer::recordCompletion(int64_t processingTimerWgIdentifier, int64_t processingTimeMs)
{
    std::lock_guard lock(mutex);
    if (processingTimerWgIdentifier >= 0)
    {
        idleWaitgroups.push((size_t)processingTimerWgIdentifier);
    }
    nextProcessingTimes.push_back(processingTimeMs);
    if (nextProcessingTimes.size() >= AVERAGING_TIMER_SIZE)
    {
        float avgValue = 0.0f;
        int64_t avgValueInt = 0;
        float proportion = (1.0f / float(AVERAGING_TIMER_SIZE));
        for (size_t i = 0; i < AVERAGING_TIMER_SIZE; i++)
        {
            avgValueInt += nextProcessingTimes[i];
        }
        avgValue = float(avgValueInt) * proportion;
        nextProcessingTimes.resize(0);
        auto task = std::make_shared<ProcessingTimeUpdateTask>(avgValue);
        taskingManager.broadcastTask(task);
    }
}

std::shared_ptr<ProcessingTimerWaitgroup> ProcessingTimer::getNewProcessingTimerWaitgroup(int64_t processingStartUnixMs)
{
    std::lock_guard lock(mutex);

    if (idleWaitgroups.size() == 0)
    {
        size_t newIndex = waitgroups.size();
        auto newWg = std::make_shared<ProcessingTimerWaitgroup>(this, newIndex);
        waitgroups.push_back(newWg);
        idleWaitgroups.push(newIndex);
    }

    size_t index = idleWaitgroups.front();
    idleWaitgroups.pop();
    waitgroups[index]->reset(processingStartUnixMs);
    return waitgroups[index];
}

/////////////////////////////////////////////:

ProcessingTimeUpdateTask::ProcessingTimeUpdateTask(float msTime)
{
    averageProcesingTimeMs = msTime;
}

std::string ProcessingTimeUpdateTask::marshal()
{
    nlohmann::json taskj = {{"object", "task"},
                            {"task", "processing_time_update_task"},
                            {"is_completed", isCompleted()},
                            {"failed", hasFailed()},
                            {"average_processing_time_ms", averageProcesingTimeMs},
                            {"recordable_in_history", recordableInHistory},
                            {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}