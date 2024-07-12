#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class TextEntry : public juce::Component
{
  public:
    TextEntry();
    void paint(juce::Graphics &g) override;
    void resized() override;
    void setText(std::string newText);
};