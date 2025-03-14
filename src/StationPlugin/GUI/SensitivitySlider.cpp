#include "SensitivitySlider.h"
#include "StationPlugin/Audio/VolumeSensitivityTask.h"
#include "TaskManagement/TaskingManager.h"
#include <memory>

SensitivitySlider::SensitivitySlider(TaskingManager &tm) : taskingManager(tm)
{
    setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    setRange(juce::Range<double>(INTENSITY_SENSITIVITY_MIN, INTENSITY_SENSITIVITY_MAX), INTENSITY_SENSITIVITY_STEP);
    setValue(6.0f);
    setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
}

void SensitivitySlider::valueChanged()
{
    float newValue = (float)getValue();
    // we flip the value as more is actually less
    newValue = INTENSITY_SENSITIVITY_MAX - (newValue - INTENSITY_SENSITIVITY_MIN);
    auto updateTask = std::make_shared<VolumeSensitivityTask>(newValue);
    taskingManager.broadcastTask(updateTask);
}