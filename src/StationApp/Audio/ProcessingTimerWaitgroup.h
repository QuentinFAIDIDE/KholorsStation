#pragma once

#include <cstdint>
#include <mutex>

class ProcessingTimer;

/**
 * @brief This class is used to handle asynchronous
 * completion of AudioSegment (NewFfftDataTask) across
 * multiple threads.
 * It's passed to worker threads after calling reset
 * and each worker thread call recordCompletion, after
 * the count goes over how many the thread specified we call the parent ProcessingTimer
 * with the audio segment processing time.
 */
class ProcessingTimerWaitgroup
{
  public:
    /**
     * @brief Construct a new Processing Timer Waitgroup object
     *
     * @param pt the parent processing timer that create this object
     * @param identifier a unique identifier for the new processing timer waitgroup
     */
    ProcessingTimerWaitgroup(ProcessingTimer *pt, int64_t identifier);

    /**
     * @brief Called to reset the processing timer waitgroup
     * in order to reuse it.
     * @param payloadSentTimeUnixMs
     */
    void reset(int64_t payloadSentTimeUnixMs);

    /**
     * @brief This is called by worker thread when they finish their
     * work with the audio segment we track.
     */
    void recordCompletion();

    /**
     * @brief Ensures it cannot call the parent anymore (because we destroy it)
     *
     */
    void deactivate();

    /**
     * @brief Add one more completion count requirement.
     */
    void add();

  private:
    ProcessingTimer *parent;       /**< to be called to report processing time */
    int counter;                   /**< number of worker thread which reported completion, max to 2 */
    int64_t payloadSentTimeUnixMs; /**< time at which the audio segment we track was sent */
    std::mutex mutex;              /**< preventing race conditions between threads */
    int64_t identifier;            /**< a unique identifier for this processing timer waitgroup */
    int expectedCounts;            /**< how many counts we expect to consider the task completed */
};