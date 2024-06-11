#include "WaitGroup.h"

// shamelessly stolen from here:
// https://github.com/dragonquest/cpp-channels/issues/1

void WaitGroup::Add(int i)
{
    std::unique_lock<std::mutex> l(mu_);
    counter_ += i;
}

void WaitGroup::Done()
{
    {
        std::unique_lock<std::mutex> l(mu_);
        counter_--;
    }
    cv_.notify_all();
}

void WaitGroup::Wait()
{
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [&] { return counter_ <= 0; });
}