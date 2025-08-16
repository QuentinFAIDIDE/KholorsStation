#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

class HelpDialogContent : public juce::Component
{

  public:
    HelpDialogContent();
    ~HelpDialogContent();

    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpDialogContent);

    int getMaxWidthFromTexts();

    juce::Rectangle<int> helpTipPos;
    juce::Rectangle<int> addVstTipPos;
    juce::Rectangle<int> mouseMoveTipPos;
    juce::Rectangle<int> automationTipPos;
    juce::Rectangle<int> focusTrackTipPos;
    juce::Rectangle<int> precisionTipPos;
    juce::Rectangle<int> freqTipPos;
    juce::Rectangle<int> volumeTipPos;
    juce::Rectangle<int> issuesTipPos;

    juce::Font font;

    std::vector<std::pair<juce::Rectangle<int> *, std::string>> tipsAndText;

    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
};