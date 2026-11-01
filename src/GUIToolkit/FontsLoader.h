#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

inline constexpr int DIMMED_LABEL_FONT_HEIGHT = 12;
inline constexpr float DIMMED_LABEL_KERNING = 0.15f;

struct FontsLoader
{
    FontsLoader();

    juce::Font monospaceFont;
    juce::Font roboto;
    juce::Font robotoMedium;
    juce::Font robotoBold;
    juce::Font robotoBlack;
    juce::Font dimmedLabelFont; /**< small spaced-out monospace font of card titles and secondary labels */

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FontsLoader)
};
