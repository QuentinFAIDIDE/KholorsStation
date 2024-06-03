#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "GUIToolkit/Consts.h"

#define BOTTOM_BAR_MINIMUM_HEIGHT TABS_HEIGHT
#define BOTTOM_BAR_MAXIMUM_HEIGHT 800
#define RESIZE_BAR_HEIGHT 2

class BottomPanel : public juce::Component
{
  public:
    BottomPanel();
    void resized() override;
    void paint(juce::Graphics &g) override;

  private:
    std::shared_ptr<juce::ComponentBoundsConstrainer> sizeConstrainer;
    std::shared_ptr<juce::ResizableEdgeComponent> resizableEdge;
    juce::TabbedComponent tabs;
};