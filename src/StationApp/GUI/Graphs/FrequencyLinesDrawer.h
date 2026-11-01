#pragma once

#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "juce_gui_basics/juce_gui_basics.h"

/**
 * @brief Component that draw horizontal lines on top of child
 * at the frequency chosen as reference.
 */
class FrequencyLinesDrawer : public juce::Component
{
  public:
    FrequencyLinesDrawer(NormalizedUnitTransformer &ut, int maxDrawableFreqArg)
        : maxDrawableFreq(maxDrawableFreqArg), unitTransformer(ut)
    {
        setOpaque(false);
        setInterceptsMouseClicks(false, false);
        frequenciesToDraw.push_back(100);
        frequenciesToDrawThinner.push_back(350);
        frequenciesToDraw.push_back(1000);
        frequenciesToDrawThinner.push_back(3500);
        frequenciesToDraw.push_back(10000);
    }

    void paint(juce::Graphics &g) override
    {
        g.setColour(KHOLORS_COLOR_GRIDS_LEVEL_0);
        int const halfScreen = getHeight() >> 1;
        for (long const i : frequenciesToDraw)
        {
            float const freqPosition = static_cast<float>(i) / static_cast<float>(maxDrawableFreq);
            float const screenPosition = unitTransformer.transform(freqPosition);
            float const lineCenterOffset = screenPosition * static_cast<float>(halfScreen);
            float linePos = halfScreen - static_cast<int>(lineCenterOffset);
            auto const topLine = getLocalBounds().withTrimmedTop(linePos).withHeight(1);
            g.fillRect(topLine);
            if (i > 0)
            {
                linePos = halfScreen + static_cast<int>(lineCenterOffset);
                auto const bottomLine = getLocalBounds().withTrimmedTop(linePos).withHeight(1);
                g.fillRect(bottomLine);
            }
        }

        g.setColour(KHOLORS_COLOR_GRIDS_LEVEL_0);
        for (long const i : frequenciesToDrawThinner)
        {
            float const freqPosition = static_cast<float>(i) / static_cast<float>(maxDrawableFreq);
            float const screenPosition = unitTransformer.transform(freqPosition);
            float const lineCenterOffset = screenPosition * static_cast<float>(halfScreen);
            float linePos = halfScreen - static_cast<int>(lineCenterOffset);
            auto const topLine = getLocalBounds().withTrimmedTop(linePos).withHeight(1);
            g.fillRect(topLine);
            if (i > 0)
            {
                linePos = halfScreen + static_cast<int>(lineCenterOffset);
                auto const bottomLine = getLocalBounds().withTrimmedTop(linePos).withHeight(1);
                g.fillRect(bottomLine);
            }
        }
    }

  private:
    int maxDrawableFreq;
    std::vector<int64_t> frequenciesToDraw;
    std::vector<int64_t> frequenciesToDrawThinner;
    NormalizedUnitTransformer &unitTransformer;
};