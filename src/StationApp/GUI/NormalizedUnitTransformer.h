#pragma once

#include "../Maths/NormalizedBijectiveProjection.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <shared_mutex>

#define UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE 4096

typedef std::shared_ptr<std::vector<float>> SharedFloatVectPtr;

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
        auto newTables = generateLookupTables();
        std::unique_lock lock(mutex);
        lookupTableIn = newTables.first;
        lookupTableOut = newTables.second;
    }

    void setProjection(std::shared_ptr<NormalizedBijectiveProjection> newProjection)
    {
        currentProjection = newProjection;
        auto newTables = generateLookupTables();
        std::unique_lock lock(mutex);
        lookupTableIn = newTables.first;
        lookupTableOut = newTables.second;
        nonce++;
    }

    uint64_t getNonce()
    {
        std::unique_lock lock(mutex);
        return nonce;
    }

    float transform(float in)
    {
        int64_t lookupIndex = in * float(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        if (lookupIndex >= UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)
        {
            lookupIndex = UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 1;
        }
        if (lookupIndex < 0)
        {
            lookupIndex = 0;
        }
        size_t lookupIndexSt = (size_t)lookupIndex;
        std::shared_lock lock(mutex);
        return (*lookupTableIn)[lookupIndexSt];
    }

    float transformInv(float in)
    {
        int64_t lookupIndex = in * float(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        if (lookupIndex >= UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)
        {
            lookupIndex = UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 1;
        }
        if (lookupIndex < 0)
        {
            lookupIndex = 0;
        }
        size_t lookupIndexSt = (size_t)lookupIndex;
        std::shared_lock lock(mutex);
        return (*lookupTableOut)[lookupIndexSt];
    }

  private:
    std::pair<SharedFloatVectPtr, SharedFloatVectPtr> generateLookupTables()
    {
        // ass the lookup table are not updated very often and
        // this is done without locking the previous tables,
        // we can take our time here.

        auto vectorIn = std::make_shared<std::vector<float>>();
        vectorIn->resize(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        std::fill(vectorIn->begin(), vectorIn->end(), 0.0f);

        auto vectorOut = std::make_shared<std::vector<float>>();
        vectorOut->resize(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        std::fill(vectorOut->begin(), vectorOut->end(), 0.0f);

        float iter;
        for (size_t i = 0; i < UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE; i++)
        {
            // we sample the middle of the bin of values, hence +0.5f
            iter = (float(i) + 0.5f) / float(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
            (*vectorIn)[i] = currentProjection->projectIn(iter);
            (*vectorOut)[i] = currentProjection->projectOut(iter);
        }

        return std::pair<SharedFloatVectPtr, SharedFloatVectPtr>(vectorIn, vectorOut);
    }

    std::shared_ptr<NormalizedBijectiveProjection> currentProjection;
    std::shared_mutex mutex;
    SharedFloatVectPtr lookupTableIn;
    SharedFloatVectPtr lookupTableOut;
    uint64_t nonce; /**< value that changes when the transfo changes. Never initialized, no need to. */
};