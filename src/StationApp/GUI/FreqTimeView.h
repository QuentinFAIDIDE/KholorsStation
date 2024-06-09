#pragma once

#include "FftDrawingBackend.h"
#include "juce_gui_basics/juce_gui_basics.h"

/**
 * @brief Describe a class which displays a timeline, and
 * which own a drawing backend that will draw FFT of signal
 * received. It will draw labels and eventually more info over drawing widgets.
 */
class FreqTimeView : public juce::Component
{
  public:
    FreqTimeView();
    ~FreqTimeView();

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;

  private:
    std::shared_ptr<FftDrawingBackend> fftDrawBackend;
};