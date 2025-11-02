#pragma once

#include "juce_graphics/juce_graphics.h"
#include <unordered_map>

#define FREQVIEW_ROUNDED_CORNERS_WIDTH 7
#define FREQVIEW_BORDER_WIDTH 3

class GraphBorders
{
  public:
    ~GraphBorders();
    void draw(juce::Graphics &g, juce::Rectangle<int> bounds, bool drawMiddleLine = false);

  private:
    uint64_t getBorderCacheKey(juce::Rectangle<int> bounds, bool drawMiddleLine);
    std::unordered_map<uint64_t, juce::Image> borderCache;
};