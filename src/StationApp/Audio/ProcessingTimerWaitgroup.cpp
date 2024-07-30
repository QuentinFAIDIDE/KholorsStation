#include "ProcessingTimerWaitgroup.h"
#include "StationApp/Audio/ProcessingTimer.h"
#include <mutex>
#include <stdexcept>

#include "juce_core/juce_core.h"

ProcessingTimerWaitgroup::ProcessingTimerWaitgroup(ProcessingTimer *pt, int64_t id) : parent(pt), identifier(id)
{
    counter = 2;
    payloadSentTimeUnixMs = 0;
}

void ProcessingTimerWaitgroup::reset(int64_t sentTimeMs)
{
    std::lock_guard lock(mutex);
    payloadSentTimeUnixMs = sentTimeMs;
    counter = 0;
    expectedCounts = 0;
}

void ProcessingTimerWaitgroup::recordCompletion()
{
    std::lock_guard lock(mutex);
    counter++;
    if (counter == expectedCounts)
    {
        int64_t processingTimeMs = juce::Time::currentTimeMillis() - payloadSentTimeUnixMs;
        if (parent != nullptr)
        {
            parent->recordCompletion(identifier, processingTimeMs);
        }
    }
    else if (counter > expectedCounts)
    {
        throw std::runtime_error("more completion counts were received than expected in processing timer waitgroup");
    }
}

void ProcessingTimerWaitgroup::deactivate()
{
    std::lock_guard lock(mutex);
    parent = nullptr;
}

void ProcessingTimerWaitgroup::add()
{
    std::lock_guard lock(mutex);
    expectedCounts++;
}