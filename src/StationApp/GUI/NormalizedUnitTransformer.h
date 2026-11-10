#pragma once

#include "../Maths/NormalizedBijectiveProjection.h"
#include <cmath>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

inline constexpr int UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE = 8192;

using SharedFloatVectPtr = std::shared_ptr<std::vector<float>>;

/**
 * @brief Read a lookup table with linear interpolation between its entries.
 * @param table UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE values.
 * @param in value between 0 and 1, clamped otherwise (NaN reads as 0).
 */
inline float interpolatedLookup(const std::vector<float> &table, float in)
{
    float const clampedIn = (in > 0.0f) ? std::min(in, 1.0f) : 0.0f;
    float const position = (clampedIn * static_cast<float>(UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE)) - 0.5f;
    auto const index0 = static_cast<size_t>(
        std::clamp<int64_t>(static_cast<int64_t>(std::floor(position)), 0, UNIT_TRANSFORMER_LOOKUP_TABLE_SIZE - 2));
    float const fraction = position - static_cast<float>(index0);
    return table[index0] + (fraction * (table[index0 + 1] - table[index0]));
}

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
        std::shared_lock const lock(mutex);
        return interpolatedLookup(*lookupTableIn, in);
    }

    float transformInv(float in)
    {
        std::shared_lock const lock(mutex);
        return interpolatedLookup(*lookupTableOut, in);
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
        return interpolatedLookup(lookupTableIn, in);
    }

    float transformInv(float in)
    {
        return interpolatedLookup(lookupTableOut, in);
    }

  private:
    std::vector<float> lookupTableIn;
    std::vector<float> lookupTableOut;
    uint64_t nonce{};
};
