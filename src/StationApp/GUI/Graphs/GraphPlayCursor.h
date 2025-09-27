#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include <cstdint>
#include <mutex>

class GraphPlayCursor
{
  public:
    GraphPlayCursor();
    ~GraphPlayCursor();

    void paint(juce::Graphics &g, const juce::Rectangle<int> &bounds, int64_t viewPosition, int64_t viewScale);
    int64_t getPlayCursorPosition();
    void submitNewPlayCursorPosition(int64_t samplePosition, uint32_t sampleRate);

  private:
    int64_t playCursorPosition;
    std::mutex playCursorMutex;
};