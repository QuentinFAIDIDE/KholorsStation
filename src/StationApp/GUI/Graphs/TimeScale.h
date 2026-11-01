#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <mutex>
#include <string>
#include <unordered_map>

inline constexpr int TICK_TOP_PADDING = 5;
inline constexpr int TICK_LABEL_MARGIN = 4;
inline constexpr int TICK_LABEL_WIDTH = 30;

inline constexpr int MAXIMUM_GLYPH_CACHE_SIZE = 512;

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

    void resized() override;
    void paint(juce::Graphics &g) override;

  private:
    void drawTicks(juce::Graphics &g, int64_t currentViewPosition, int64_t currentViewScale, float currentBpm);
    void drawTickLevel(juce::Graphics &g, int height, int width, juce::Colour color, float pixelStepWidth,
                       int pixelStepShift, int firstBarIndex);
    juce::Rectangle<int> getTickTextArea(int height) const;

    int width{}, lastPaintedWidth{};
    std::mutex mutex;
    int64_t viewPosition{}, viewScale{};
    float bpm{};
    std::unordered_map<std::string, juce::GlyphArrangement> glyphCache;
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
};