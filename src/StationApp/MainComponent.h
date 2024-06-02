#pragma once

#include "GUIToolkit/KholorsLookAndFeel.h"
#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component
{
  public:
    MainComponent();

    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    KholorsLookAndFeel appLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};