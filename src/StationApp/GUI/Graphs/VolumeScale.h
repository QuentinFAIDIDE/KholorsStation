#pragma once

#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

#define AXIS_TITLE_PIXELS_FROM_LEFT 14
#define MAXIMUM_DRAWABLE_LABEL_POSITION_RATIO 0.93
#define LABEL_HEIGHT 16
#define LABEL_FONT_HEIGHT (KHOLORS_DEFAULT_FONT_SIZE - 2)
#define LABEL_RIGHT_PADDING 10

/**
 * @brief A volume scale that will paint tick and
 * volume number depending on the NormalizedUnitTransformer.
 */
class VolumeScale : public juce::Component
{
  public:
    /**
     * @brief Construct a new Volume Scale object
     */
    VolumeScale();

    ~VolumeScale();

    void paint(juce::Graphics &g) override;

  private:
    /**
     * @brief Draw the rotated "VOLUME" axis title.
     * @param g juce graphic context from paint.
     */
    void drawRotatedTitle(juce::Graphics &g);

    void drawRotatedChannelNames(juce::Graphics &g);
};