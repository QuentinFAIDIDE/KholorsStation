#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#define TICK_TOP_PADDING 5
#define TICK_LABEL_MARGIN 4
#define TICK_LABEL_WIDTH 30

#define TITLE_PIXELS_FROM_BOTTOM 12
#define TITLE_PIXELS_HEIGHT 20

/**
 * @brief A component that draws beat bars
 * and numbers. Basically a time axis scale.
 */
class TimeScale : public juce::Component
{
  public:
    TimeScale();
    void setViewPosition(int64_t viewPosition);
    void setViewScale(int64_t viewScale);
    void setBpm(float bpm);

    void paint(juce::Graphics &g) override;

  private:
    void drawTicks(juce::Graphics &g, int64_t currentViewPosition, int64_t currentViewScale, float currentBpm);
    void drawTickLevel(juce::Graphics &g, int height, int width, juce::Colour color, float pixelStepWidth,
                       int pixelStepShift, int firstBarIndex);

    std::mutex mutex;
    int64_t viewPosition, viewScale;
    float bpm;
};