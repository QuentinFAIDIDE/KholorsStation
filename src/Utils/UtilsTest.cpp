#include "NoAllocIndexQueue.h"
#include <stdexcept>

int main(int, char **)
{
    NoAllocIndexQueue queue(10);

    // the queue should insert and retrieve the elements in the right order
    for (size_t i = 0; i < 100; i++)
    {
        if (queue.getSize() != 0)
        {
            throw std::runtime_error("wrong queue size");
        }
        queue.queue(1);
        queue.queue(2);

        if (queue.getSize() != 2)
        {
            throw std::runtime_error("wrong queue size");
        }

        auto deq1 = queue.dequeue();
        auto deq2 = queue.dequeue();
        auto deq3 = queue.dequeue();

        if (queue.getSize() != 0)
        {
            throw std::runtime_error("wrong queue size");
        }

        if (deq3.has_value())
        {
            throw std::runtime_error("dequeued more than queued");
        }

        if (!deq2.has_value() || !deq1.has_value())
        {
            throw std::runtime_error("not able to dequeue a value");
        }

        if (*deq2 != 2)
        {
            throw std::runtime_error("deq2 is not 2");
        }

        if (*deq1 != 1)
        {
            throw std::runtime_error("deq1 is not 1");
        }
    }

    for (size_t i = 0; i < 10; i++)
    {
        queue.queue(i);
        if (queue.getSize() != i + 1)
        {
            throw std::runtime_error("invalid queue size");
        }
    }

    bool threwException = false;
    try
    {
        queue.queue(0);
    }
    catch (const NoAllocIndexQueue::NoMoreSpaceException &e)
    {
        threwException = true;
    }

    if (!threwException)
    {
        throw std::runtime_error("queue didn't throw exception when full");
    }
}