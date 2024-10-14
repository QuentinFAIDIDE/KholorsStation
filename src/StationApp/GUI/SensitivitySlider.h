#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#define INTENSITY_SENSITIVITY_MIN 4.0f
#define INTENSITY_SENSITIVITY_MAX 8.0f
#define INTENSITY_SENSITIVITY_STEP 0.25f

/**
 * @brief A slider that lets users change the sensitivity
 * of the volume (intensity) transformation.
 */
class SensitivitySlider : public juce::Slider
{
  public:
    SensitivitySlider();
    void valueChanged() override;
};