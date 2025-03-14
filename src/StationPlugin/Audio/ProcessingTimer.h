#pragma once

#include "StationPlugin/Audio/ProcessingTimerWaitgroup.h"
#include "TaskManagement/Task.h"
#include "TaskManagement/TaskingManager.h"
#include <cstdint>
#include <memory>
#include <queue>
#include <vector>

/**
 * @brief A class that records and averages processing times for audio segments.
 */
class ProcessingTimer
{
  public:
    ProcessingTimer(TaskingManager &tm);
    ~ProcessingTimer();

    /**
     * @brief Called by the ProcessingTimerWaitgroup to report an audio segment
     * processing time.
     *
     * @param processingTimerWgIdentifier unique identifier of the waitgroup that emits completion (for reusing it). If
     * smaller than zero, we ignore the waitgroup reuse (currently used for skipping processing when we don't use one).
     * @param processingTimeMs the time it took to process an audio segment.
     */
    void recordCompletion(int64_t processingTimerWgIdentifier, int64_t processingTimeMs);

    /**
     * @brief Get a new Processing Timer Waitgroup object
     *
     * @param processingStartUnixMs timestamp where payload was emmited
     *
     * @return std::shared_ptr<ProcessingTimerWaitgroup>
     */
    std::shared_ptr<ProcessingTimerWaitgroup> getNewProcessingTimerWaitgroup(int64_t processingStartUnixMs);

  private:
    TaskingManager &taskingManager;
    std::mutex mutex;
    std::vector<std::shared_ptr<ProcessingTimerWaitgroup>>
        waitgroups;                    /**< waitgroups that are either currently used or are waiting to be */
    std::queue<size_t> idleWaitgroups; /**< waitgroups that are ready to be used */

    std::vector<int64_t> nextProcessingTimes; /**< processing times that will be averaged once we have enough */
};

/**
 * @brief A task that is emitted everytime we want to
 * broadcast a new audio segument average processing time.
 */
class ProcessingTimeUpdateTask : public SilentTask
{
  public:
    ProcessingTimeUpdateTask(float avgTimeMs);
    std::string marshal() override;

    float averageProcesingTimeMs;
};