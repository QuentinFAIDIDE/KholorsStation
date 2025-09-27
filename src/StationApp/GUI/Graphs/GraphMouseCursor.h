#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class GraphMouseCursor
{
  public:
    GraphMouseCursor();
    ~GraphMouseCursor();

    void paint(juce::Graphics &g, const juce::Rectangle<int> &bounds);
    void setMouseCursor(bool onComponent, int x, int y);

  private:
    int lastMouseX, lastMouseY;
    bool mouseOnComponent;
};