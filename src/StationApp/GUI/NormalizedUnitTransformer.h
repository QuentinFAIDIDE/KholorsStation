#pragma once

#include "../Maths/NormalizedBijectiveProjection.h"
#include <memory>
#include <mutex>
#include <shared_mutex>

#define UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE 4096

/**
 * @brief A Unit transformer that maps [0, 1] -> [0, 1]
 * and allow concurrency between transformations and
 * projection changing.
 */
class NormalizedUnitTransformer
{
  public:
    NormalizedUnitTransformer()
    {
        currentProjection = std::make_shared<LinearProjection>();
        updateLookupTables();
    }

    void setProjection(std::shared_ptr<NormalizedBijectiveProjection> newProjection)
    {
        std::unique_lock lock(mutex);
        currentProjection = newProjection;
        updateLookupTables();
    }

    float transform(float in)
    {
        std::shared_lock lock(mutex);
        int64_t lookupIndex = in * float(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        if (lookupIndex >= UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)
        {
            lookupIndex = UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 1;
        }
        if (lookupIndex < 0)
        {
            lookupIndex = 0;
        }
        return lookupTableIn[lookupIndex];
    }

    float transformInv(float in)
    {
        std::shared_lock lock(mutex);
        int64_t lookupIndex = in * float(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        if (lookupIndex >= UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)
        {
            lookupIndex = UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 1;
        }
        if (lookupIndex < 0)
        {
            lookupIndex = 0;
        }
        return lookupTableOut[lookupIndex];
    }

  private:
    void updateLookupTables()
    {
        float iter;
        for (size_t i = 0; i < UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE; i++)
        {
            // we sample the middle of the bin of values, hence +0.5f
            iter = (float(i) + 0.5f) / float(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
            lookupTableIn[i] = currentProjection->projectIn(iter);
            lookupTableOut[i] = currentProjection->projectOut(iter);
        }
    }

    std::shared_ptr<NormalizedBijectiveProjection> currentProjection;
    std::shared_mutex mutex;
    float lookupTableIn[UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE];
    float lookupTableOut[UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE];
};