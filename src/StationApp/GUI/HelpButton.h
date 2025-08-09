#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class HelpButton : public juce::TextButton
{
  public:
    HelpButton();
    ~HelpButton();

    void clicked() override;
};