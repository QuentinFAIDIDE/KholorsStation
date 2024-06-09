#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

/**
 * @brief A simple synchronisation routine
 *        inspired from golang WaitGroups.
 *
 *  Stolen from here: https://github.com/dragonquest/cpp-channels/blob/master/waitgroup.h
 *  Minus the race fix of issue #1
 */
class WaitGroup
{
  public:
    void Add(int i = 1);
    void Done();
    void Wait();

  private:
    std::mutex mu_;
    std::atomic<int> counter_;
    std::condition_variable cv_;
};