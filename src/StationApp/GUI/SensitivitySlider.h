#pragma once

#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

#define INTENSITY_SENSITIVITY_MIN 2.0f
#define INTENSITY_SENSITIVITY_MAX 10.0f
#define INTENSITY_SENSITIVITY_STEP 0.25f

/**
 * @brief A slider that lets users change the sensitivity
 * of the volume (intensity) transformation.
 */
class SensitivitySlider : public juce::Slider
{
  public:
    SensitivitySlider(TaskingManager &tm);
    void valueChanged() override;

  private:
    TaskingManager &taskingManager;
};