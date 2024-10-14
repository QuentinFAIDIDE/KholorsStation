#include "SensitivitySlider.h"

SensitivitySlider::SensitivitySlider()
{
    setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    setRange(juce::Range<double>(INTENSITY_SENSITIVITY_MIN, INTENSITY_SENSITIVITY_MAX), INTENSITY_SENSITIVITY_STEP);
    setValue(6.0f);
    setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
}

void SensitivitySlider::valueChanged()
{
}