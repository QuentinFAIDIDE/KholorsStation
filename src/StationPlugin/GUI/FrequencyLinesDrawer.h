#pragma once

#include "GUIToolkit/Consts.h"
#include "StationPlugin/GUI/NormalizedUnitTransformer.h"
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

    void paint(juce::Graphics &g)
    {
        g.setColour(KHOLORS_COLOR_GRIDS_LEVEL_0);
        int halfScreen = getHeight() >> 1;
        for (size_t i = 0; i < frequenciesToDraw.size(); i++)
        {
            float freqPosition = float(frequenciesToDraw[i]) / float(maxDrawableFreq);
            float screenPosition = unitTransformer.transform(freqPosition);
            float lineCenterOffset = screenPosition * float(halfScreen);
            float linePos = halfScreen - (int)lineCenterOffset;
            auto topLine = getLocalBounds().withTrimmedTop(linePos).withHeight(1);
            g.fillRect(topLine);
            if (frequenciesToDraw[i] > 0)
            {
                linePos = halfScreen + (int)lineCenterOffset;
                auto bottomLine = getLocalBounds().withTrimmedTop(linePos).withHeight(1);
                g.fillRect(bottomLine);
            }
        }

        g.setColour(KHOLORS_COLOR_GRIDS_LEVEL_0);
        for (size_t i = 0; i < frequenciesToDrawThinner.size(); i++)
        {
            float freqPosition = float(frequenciesToDrawThinner[i]) / float(maxDrawableFreq);
            float screenPosition = unitTransformer.transform(freqPosition);
            float lineCenterOffset = screenPosition * float(halfScreen);
            float linePos = halfScreen - (int)lineCenterOffset;
            auto topLine = getLocalBounds().withTrimmedTop(linePos).withHeight(1);
            g.fillRect(topLine);
            if (frequenciesToDrawThinner[i] > 0)
            {
                linePos = halfScreen + (int)lineCenterOffset;
                auto bottomLine = getLocalBounds().withTrimmedTop(linePos).withHeight(1);
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