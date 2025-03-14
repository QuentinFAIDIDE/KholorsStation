#pragma once

#include "juce_core/juce_core.h"
#include <stdexcept>

/**
 * @brief A class that implement a thread safe single-reader
 * single-writer lock free FIFO queue for array indices.
 * This queue was meant for the audio thread to write buffer indices,
 * while another background threads reads them by pair to coalesce them.
 * So the design reflect those needs.
 */
class LockFreeIndexFIFO
{
  public:
    LockFreeIndexFIFO(size_t size) : abstractFifo(size)
    {
        ringBuffer.resize(size);
    }

    /**
     * @brief Queue will insert a single element into the queue.
     *
     * @param indexToInsert the number to insert into the queue
     * @return size_t the number of items inserted
     */
    size_t queue(size_t indexToInsert)
    {
        const auto scope = abstractFifo.write(1);
        if (scope.blockSize1 > 0)
        {
            ringBuffer[(size_t)scope.startIndex1] = indexToInsert;
            return 1;
        }
        if (scope.blockSize2 > 0)
        {
            ringBuffer[(size_t)scope.startIndex2] = indexToInsert;
            return 1;
        }
        return 0;
    }

    /**
     * @brief retrieve a sequence of elements from the FIFO queue.
     *
     * @param data a vector to resize to 0 and push the data onto.
     * @param numItems the number of items to fetch (may be less if not enough are available)
     */
    void dequeue(std::shared_ptr<std::vector<size_t>> data, int numItems)
    {
        if (data == nullptr)
        {
            throw std::invalid_argument("passed a nullptr data array to LockFreeIndexFIFO's dequeue");
        }

        const auto scope = abstractFifo.read(numItems);

        // we ensure there is enough space to push_back our fetched indices.
        data->reserve((size_t)(scope.blockSize1 + scope.blockSize2));
        data->resize(0);

        if (scope.blockSize1 > 0)
        {
            for (size_t i = (size_t)scope.startIndex1; i < (size_t)scope.startIndex1 + (size_t)scope.blockSize1; i++)
            {
                data->push_back(ringBuffer[i]);
            }
        }

        if (scope.blockSize2 > 0)
        {
            for (size_t i = (size_t)scope.startIndex2; i < (size_t)scope.startIndex2 + (size_t)scope.blockSize2; i++)
            {
                data->push_back(ringBuffer[i]);
            }
        }
    }

  private:
    juce::AbstractFifo abstractFifo;
    std::vector<size_t> ringBuffer;
};