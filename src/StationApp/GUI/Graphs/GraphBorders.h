#pragma once

#include "juce_graphics/juce_graphics.h"
#include <unordered_map>

inline constexpr int FREQVIEW_ROUNDED_CORNERS_WIDTH = 7;
inline constexpr int FREQVIEW_BORDER_WIDTH = 3;

class GraphBorders
{
  public:
    ~GraphBorders();
    void draw(juce::Graphics &g, juce::Rectangle<int> bounds, bool drawMiddleLine = false);

  private:
    static uint64_t getBorderCacheKey(juce::Rectangle<int> bounds, bool drawMiddleLine);
    std::unordered_map<uint64_t, juce::Image> borderCache;
};