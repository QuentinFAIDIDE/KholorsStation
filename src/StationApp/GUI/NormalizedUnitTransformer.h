#pragma once

#include "../Maths/NormalizedBijectiveProjection.h"
#include <memory>
#include <mutex>
#include <shared_mutex>

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
    }

    void setProjection(std::shared_ptr<NormalizedBijectiveProjection> newProjection)
    {
        std::unique_lock lock(mutex);
        currentProjection = newProjection;
    }

    float transform(float in)
    {
        std::shared_lock lock(mutex);
        return currentProjection->projectIn(in);
    }

    float transformInv(float in)
    {
        std::shared_lock lock(mutex);
        return currentProjection->projectOut(in);
    }

  private:
    std::shared_ptr<NormalizedBijectiveProjection> currentProjection;
    std::shared_mutex mutex;
};