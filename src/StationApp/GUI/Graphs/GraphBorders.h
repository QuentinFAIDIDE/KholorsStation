#pragma once

#include "juce_graphics/juce_graphics.h"

#define FREQVIEW_ROUNDED_CORNERS_WIDTH 7
#define FREQVIEW_BORDER_WIDTH 3

void drawGraphBorders(juce::Graphics &g, juce::Rectangle<int> bounds, bool drawMiddleLine = false);