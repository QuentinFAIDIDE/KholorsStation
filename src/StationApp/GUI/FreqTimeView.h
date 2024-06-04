#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class FreqTimeView : public juce::Component
{
  public:
    FreqTimeView();
    ~FreqTimeView();

    void paint(juce::Graphics &g) override;
};