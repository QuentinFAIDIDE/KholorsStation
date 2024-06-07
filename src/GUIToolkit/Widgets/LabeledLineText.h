#pragma once

#include "GUIToolkit/Consts.h"
#include "GUIToolkit/FontsLoader.h"
#include "juce_gui_basics/juce_gui_basics.h"

class LabeledLineText : public juce::Component
{
  public:
    LabeledLineText(std::string txt) : text(txt)
    {
    }

    void paint(juce::Graphics &g)
    {
        g.setFont(sharedFonts->monospaceFont.withHeight(DEFAULT_FONT_SIZE));
        g.setColour(COLOR_TEXT);
        g.drawText(text, getLocalBounds(), juce::Justification::centredRight, true);
    }

  private:
    juce::SharedResourcePointer<FontsLoader> sharedFonts;
    std::string text;
};