#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

//
class EmptyTab : public juce::Component
{
  public:
    EmptyTab()
    {
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }
};
