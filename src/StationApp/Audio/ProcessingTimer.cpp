#include "ProcessingTimer.h"
#include "StationApp/Audio/ProcessingTimerWaitgroup.h"
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>

constexpr int DEFAULT_PREALLOCATED_PROC_TIMER_WAITGROUPS = 128;
constexpr int AVERAGING_TIMER_SIZE = 32;

ProcessingTimer::ProcessingTimer(TaskingManager &tm) : taskingManager(tm)
{
    for (size_t i = 0; i < DEFAULT_PREALLOCATED_PROC_TIMER_WAITGROUPS; i++)
    {
        auto const newWg = std::make_shared<ProcessingTimerWaitgroup>(this, i);
        waitgroups.push_back(newWg);
        idleWaitgroups.push(i);
    }
}

ProcessingTimer::~ProcessingTimer()
{
    for (auto const &waitgroup : waitgroups)
    {
        waitgroup->deactivate();
    }
}

void ProcessingTimer::recordCompletion(int64_t processingTimerWgIdentifier, int64_t processingTimeMs)
{
    std::scoped_lock const lock(mutex);
    if (processingTimerWgIdentifier >= 0)
    {
        idleWaitgroups.push(static_cast<size_t>(processingTimerWgIdentifier));
    }
    nextProcessingTimes.push_back(processingTimeMs);
    if (nextProcessingTimes.size() >= AVERAGING_TIMER_SIZE)
    {
        float avgValue = 0.0f;
        int64_t avgValueInt = 0;
        float const proportion = (1.0f / static_cast<float>(AVERAGING_TIMER_SIZE));
        for (size_t i = 0; i < AVERAGING_TIMER_SIZE; i++)
        {
            avgValueInt += nextProcessingTimes[i];
        }
        avgValue = static_cast<float>(avgValueInt) * proportion;
        nextProcessingTimes.resize(0);
        auto const task = std::make_shared<ProcessingTimeUpdateTask>(avgValue);
        taskingManager.broadcastTask(task);
    }
}

std::shared_ptr<ProcessingTimerWaitgroup> ProcessingTimer::getNewProcessingTimerWaitgroup(int64_t processingStartUnixMs)
{
    std::scoped_lock const lock(mutex);

    // if all processing timers are in use, it means that we are
    // on overload and we'll use that as a hint to ignore the FFT to draw
    if (idleWaitgroups.empty())
    {
        return nullptr;
    }

    size_t const index = idleWaitgroups.front();
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
    nlohmann::json const taskj = {
        {"object", "task"},
        {"task", "processing_time_update_task"},
        {"is_completed", isCompleted()},
        {"failed", hasFailed()},
        {"average_processing_time_ms", averageProcesingTimeMs},
        {"recordable_in_history", recordableInHistory},
        {"is_part_of_reversion", isPartOfReversion},
    };
    return taskj.dump();
}