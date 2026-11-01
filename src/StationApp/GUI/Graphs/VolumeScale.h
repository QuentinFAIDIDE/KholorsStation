#pragma once

#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

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

    ~VolumeScale() override;

    void paint(juce::Graphics &g) override;

  private:
    /**
     * @brief Draw the rotated "VOLUME" axis title.
     * @param g juce graphic context from paint.
     */
    void drawRotatedTitle(juce::Graphics &g);

    void drawRotatedChannelNames(juce::Graphics &g);
};