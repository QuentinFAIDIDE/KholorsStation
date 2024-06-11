#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <vector>

/**
 * @brief Queue of indexes (size_t) that is thread safe
 * and can be preallocated, similar to a ring buffer.
 * It will not allocate on push, and will throw an error
 * if the queue is full.
 */
class NoAllocIndexQueue
{
  public:
    NoAllocIndexQueue(size_t queueSize)
    {
        ringBuffer.resize(queueSize);
        queueStartIndex = 0;
        queueEndIndex = 0;
        size = 0;
    }

    /**
     * @brief This exception is thrown when the queue has no more space.
     */
    class NoMoreSpaceException : public std::exception
    {
      private:
        std::string message;

      public:
        NoMoreSpaceException(const std::string &msg) : message(msg)
        {
        }

        virtual const char *what() const throw()
        {
            return message.c_str();
        }
    };

    void queue(size_t indexToInsert)
    {
        std::lock_guard lock(queueMutex);
        if (size == ringBuffer.size())
        {
            throw NoMoreSpaceException("no more memory in the NoAllocIndexQueue");
        }

        ringBuffer[queueStartIndex] = indexToInsert;
        size += 1;
        if (queueStartIndex == 0)
        {
            queueStartIndex = ringBuffer.size() - 1;
        }
        else
        {
            queueStartIndex -= 1;
        }
    }

    std::optional<size_t> dequeue()
    {
        std::lock_guard lock(queueMutex);
        if (size == 0)
        {
            return std::nullopt;
        }

        std::optional<size_t> valueToReturn;
        valueToReturn = ringBuffer[queueEndIndex];
        size -= 1;
        if (queueEndIndex == 0)
        {
            queueEndIndex = ringBuffer.size() - 1;
        }
        else
        {
            queueEndIndex -= 1;
        }
        return valueToReturn;
    }

    size_t getSize()
    {
        std::lock_guard lock(queueMutex);
        return size;
    }

  private:
    std::vector<size_t> ringBuffer; /**< ring buffer where values are stored */
    size_t queueStartIndex;         /**< index of next element to be inserted in the ring buffer */
    size_t queueEndIndex;           /**< index of the oldest element inserted in the ring buffer */
    std::mutex queueMutex;          /**< Prevent concurrent access to queue */
    size_t size;                    /**< Current queue size */
};