#pragma once

#include "juce_core/juce_core.h"
#include <cmath>
#include <limits>
#include <stdexcept>

/**
 * @brief Abstract class for a projection from range [0,1] to [0,1] that
 * is nearly bijective (we tolerate neglectable artefacts for extreme values).
 */
class NormalizedBijectiveProjection
{
  public:
    virtual float projectIn(float input) const = 0;
    virtual float projectOut(float input) const = 0;
};

/**
 * @brief A simple No-Op projection that changes nothing.
 */
class LinearProjection : public NormalizedBijectiveProjection
{
  public:
    float projectIn(float input) const override
    {
        return juce::jlimit(0.0f, 1.0f, input);
    }
    float projectOut(float input) const override
    {
        return juce::jlimit(0.0f, 1.0f, input);
    }
};

/**
 * @brief Invert whathever projection is provided as argument;
 */
class InvertProjection : public NormalizedBijectiveProjection
{
  public:
    InvertProjection(std::shared_ptr<NormalizedBijectiveProjection> proj) : projToInvert(proj)
    {
    }

    float projectIn(float input) const override
    {
        return projToInvert->projectOut(input);
    }
    float projectOut(float input) const override
    {
        return projToInvert->projectIn(input);
    }

  private:
    std::shared_ptr<NormalizedBijectiveProjection> projToInvert;
};

/**
 * @brief A projection chaining multiple other projections.
 * Note that it does not support adding projection while it's in use, do it before.
 */
class StackedProjections : public NormalizedBijectiveProjection
{
  public:
    void addProjection(std::shared_ptr<NormalizedBijectiveProjection> p)
    {
        if (p == nullptr)
        {
            throw std::invalid_argument("sent nullptr to StackedProjection addProjection");
        }
        projections.push_back(p);
    }

    float projectIn(float input) const override
    {
        if (projections.size() == 0)
        {
            return defaultProjection.projectIn(input);
        }
        else
        {
            float currentValue = input;
            for (size_t i = 0; i < projections.size(); i++)
            {
                currentValue = projections[i]->projectIn(currentValue);
            }
            return currentValue;
        }
    }
    float projectOut(float input) const override
    {
        if (projections.size() == 0)
        {
            return defaultProjection.projectOut(input);
        }
        else
        {
            float currentValue = input;
            for (int i = projections.size() - 1; i >= 0; i--)
            {
                currentValue = projections[(size_t)i]->projectOut(currentValue);
            }
            return currentValue;
        }
    }

  private:
    std::vector<std::shared_ptr<NormalizedBijectiveProjection>> projections;
    LinearProjection defaultProjection;
};

/**
 * @brief Quasi log10 projecting, which is a log10 that accepts 0 as input by introducing a shift to inputs.
 * This is then corrected with coefficient a and b to fall in the [0,1] range for input and output.
 * Useful for example to project indexes of FFT in order to have log10 scale.
 * As all NormalizedBijectiveProjection, it maps [0, 1] to [0, 1]
 */
class Log10Projection : public NormalizedBijectiveProjection
{
  public:
    /**
     * @brief Construct a new Log10 Projection object
     *
     * @param shift the shift of source range to avoid having log10(0)
     */
    Log10Projection(float shift = 0.1) : inputShift(shift)
    {
        if (inputShift <= std::numeric_limits<float>::epsilon())
        {
            throw std::invalid_argument("invalid inputShift passed to Log10Projection");
        }
        aCoef = 1.0f / std::log10((shift + 1.0f) / shift);
        aCoefInv = 1.0f / aCoef;
        bCoef = 1.0f / shift;
        bCoefInv = 1.0f / bCoef;
        preComputedFactor1 = aCoef * std::log10(bCoef);
    }

    float projectIn(float input) const override
    {
        // ensure we map in range
        float inputPreProcessed = juce::jlimit(0.0f, 1.0f, input) + inputShift;
        return preComputedFactor1 + (aCoef * std::log10(inputPreProcessed));
    }

    float projectOut(float input) const override
    {
        float inputPreProcessed = juce::jlimit(0.0f, 1.0f, input);
        return (std::powf(10.0f, inputPreProcessed / aCoef) * bCoefInv) - inputShift;
    }

  private:
    float inputShift; /**< Shifting added to input to avoid calling log10(0). 0.1 sounds like a reasonable value. */
    float aCoef;      /**< Coefficient used to scale log10 + shift to [0, 1] */
    float aCoefInv;   /**< multiplicative invert of aCoef  */
    float bCoef;      /**< Coefficient used to scale log10 + shift to [0, 1] */
    float bCoefInv;   /**< multiplicative invert of bCoef */
    float preComputedFactor1; /**< Used to speed up computation by precomputing factors */
};

class SigmoidProjection : public NormalizedBijectiveProjection
{
  public:
    /**
     * @brief Construct a new Sigmoid Projection object.
     * Before being fed to sigmoid, the range is map from [0, 1]
     * to [-maxSourceRangeToUse, maxSourceRangeToUse].
     *
     * @param maxSourceRangeToUse The maximum absolute value of range sent to sigmoid.
     */
    SigmoidProjection(float maxSourceRangeToUse = 6.0)
    {
        maxSourceRange = maxSourceRangeToUse;
        float expc = std::exp(maxSourceRangeToUse);
        a = -1.0f / (expc - 1.0f);
        b = (expc + 1.0f) / (expc - 1.0f);
    }

    float projectIn(float input) const override
    {
        // ensure we map in range
        float limitedInput = juce::jlimit(0.0f, 1.0f, input);
        float mappedInput = juce::jmap(limitedInput, -1.0f, 1.0f);
        return a + (b / (1.0f + std::exp(-mappedInput * maxSourceRange)));
    }

    float projectOut(float input) const override
    {
        float limitedInput = juce::jlimit(0.0f, 1.0f, input);
        float inputAfterSigmoidInv = -(1.0f / maxSourceRange) * std::log((b / (limitedInput - a)) - 1.0f);
        return juce::jmap(inputAfterSigmoidInv, -1.0f, 1.0f, 0.0f, 1.0f);
    }

  private:
    float maxSourceRange;
    float a, b; /**< coefficients used to correct range from [-1,1] into [0,1] */
};