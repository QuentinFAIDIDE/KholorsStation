#pragma once

#include "../Maths/NormalizedBijectiveProjection.h"
#include <math.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>

#define UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE 4096

using SharedFloatVectPtr = std::shared_ptr<std::vector<float>>;

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
        auto const newTables = generateLookupTables();
        std::unique_lock const lock(mutex);
        lookupTableIn = newTables.first;
        lookupTableOut = newTables.second;
    }

    void setProjection(std::shared_ptr<NormalizedBijectiveProjection> newProjection)
    {
        currentProjection = std::move(newProjection);
        auto const newTables = generateLookupTables();
        std::unique_lock const lock(mutex);
        lookupTableIn = newTables.first;
        lookupTableOut = newTables.second;
        nonce++;
    }

    uint64_t getNonce()
    {
        std::unique_lock const lock(mutex);
        return nonce;
    }

    float transform(float in)
    {
        int64_t lookupIndex = in * static_cast<float>(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        if (lookupIndex >= UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)
        {
            lookupIndex = UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 1;
        }
        lookupIndex = std::max<int64_t>(lookupIndex, 0);
        auto const lookupIndexSt = static_cast<size_t>(lookupIndex);
        std::shared_lock const lock(mutex);
        return (*lookupTableIn)[lookupIndexSt];
    }

    float transformInv(float in)
    {
        int64_t lookupIndex = in * static_cast<float>(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        if (lookupIndex >= UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)
        {
            lookupIndex = UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 1;
        }
        lookupIndex = std::max<int64_t>(lookupIndex, 0);
        auto const lookupIndexSt = static_cast<size_t>(lookupIndex);
        std::shared_lock const lock(mutex);
        return (*lookupTableOut)[lookupIndexSt];
    }

    friend class TmpNormalizedUnitTransformer;

  private:
    std::pair<SharedFloatVectPtr, SharedFloatVectPtr> generateLookupTables()
    {
        // ass the lookup table are not updated very often and
        // this is done without locking the previous tables,
        // we can take our time here.

        auto const vectorIn = std::make_shared<std::vector<float>>();
        vectorIn->resize(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        std::fill(vectorIn->begin(), vectorIn->end(), 0.0f);

        auto const vectorOut = std::make_shared<std::vector<float>>();
        vectorOut->resize(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        std::fill(vectorOut->begin(), vectorOut->end(), 0.0f);

        float iter = NAN;
        for (size_t i = 0; i < UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE; i++)
        {
            // we sample the middle of the bin of values, hence +0.5f
            iter = (static_cast<float>(i) + 0.5f) / static_cast<float>(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
            (*vectorIn)[i] = currentProjection->projectIn(iter);
            (*vectorOut)[i] = currentProjection->projectOut(iter);
        }

        return {vectorIn, vectorOut};
    }

    std::shared_ptr<NormalizedBijectiveProjection> currentProjection;
    std::shared_mutex mutex;
    SharedFloatVectPtr lookupTableIn;
    SharedFloatVectPtr lookupTableOut;
    uint64_t nonce{}; /**< value that changes when the transfo changes. Never initialized, no need to. */
};

/**
 * @brief Returned by a unit transformer in order for a thread to have its own copy of
 * the current unit transformer that doesn't need to get the shared lock on transforming.
 */
class TmpNormalizedUnitTransformer
{
  public:
    explicit TmpNormalizedUnitTransformer(NormalizedUnitTransformer &reference)
    {
        copyTransformer(reference);
    }

    void copyTransformer(NormalizedUnitTransformer &reference)
    {
        std::scoped_lock const lock(reference.mutex);
        lookupTableIn.resize(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        for (size_t i = 0; i < lookupTableIn.size(); i++)
        {
            lookupTableIn[i] = (*reference.lookupTableIn)[i];
        }
        lookupTableOut.resize(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        for (size_t i = 0; i < lookupTableOut.size(); i++)
        {
            lookupTableOut[i] = (*reference.lookupTableOut)[i];
        }
        nonce = reference.nonce;
    }

    uint64_t getNonce() const
    {
        return nonce;
    }

    float transform(float in)
    {
        int64_t lookupIndex = in * static_cast<float>(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        if (lookupIndex >= UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)
        {
            lookupIndex = UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 1;
        }
        lookupIndex = std::max<int64_t>(lookupIndex, 0);
        auto const lookupIndexSt = static_cast<size_t>(lookupIndex);
        return lookupTableIn[lookupIndexSt];
    }

    float transformInv(float in)
    {
        int64_t lookupIndex = in * static_cast<float>(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE);
        if (lookupIndex >= UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)
        {
            lookupIndex = UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 1;
        }
        lookupIndex = std::max<int64_t>(lookupIndex, 0);
        auto const lookupIndexSt = static_cast<size_t>(lookupIndex);
        return lookupTableOut[lookupIndexSt];
    }

  private:
    std::vector<float> lookupTableIn;
    std::vector<float> lookupTableOut;
    uint64_t nonce{};
};