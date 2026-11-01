#pragma once

#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

/**
 * @brief A frequency scale that will paint tick and
 * frequency number depending on the NormalizedUnitTransformer.
 */
class FrequencyScale : public juce::Component
{
  public:
    /**
     * @brief Construct a new Frequency Scale object
     *
     * @param ut transformer for frequencies
     * @param maxDrawableFreqA maximum drawable frequency
     */
    FrequencyScale(NormalizedUnitTransformer &ut, float maxDrawableFreqA);

    ~FrequencyScale() override;

    void paint(juce::Graphics &g) override;

    struct LabelBox
    {
        LabelBox(juce::Rectangle<int> b, std::string t, std::string u)
        {
            bounds = b;
            text = t;
            unit = u;
        }
        juce::Rectangle<int> bounds;
        std::string text;
        std::string unit;
    };

  private:
    /**
     * @brief Draw the rotated "FREQUENCIES" axis title.
     * @param g juce graphic context from paint.
     */
    void drawRotatedTitle(juce::Graphics &g);

    void drawRotatedChannelNames(juce::Graphics &g);

    /**
     * @brief Generate label boxes with position, text and unit.
     *
     * @param frequencyHz frequency corresponding to the box position
     * @param text text to draw, eg "0" in "0 Hz"
     * @param unit unit to draw, eg "Hz" in "0 Hz"
     * @return std::vector<LabelBox> vector of boxes to be later drawn
     */
    std::vector<LabelBox> generateLabelBox(float frequencyHz, std::string text, std::string unit);

    static void drawLabels(juce::Graphics &g, std::vector<LabelBox> &labs);

    NormalizedUnitTransformer &unitTransformer;
    float maxDrawableFreq; /**< maximum frequency shown on screen */
};