#pragma once

#include "StationApp/Audio/NewFftDataTask.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstdint>
#include <memory>

/**
 * @brief Abstract class that receives the audio data (sfft freqs intensities) to display
 * on screen.
 */
class FftDrawingBackend : public juce::Component
{
  public:
    /**
     * @brief Move the view so that the position at the screenHorizontalPosition (between 0 and 1)
     * matches the samplePosition (audio sample offset of the song).
     *
     * @param samplePosition audio sample position (audio sample offset of the song) to match
     * @param screenHorizontalPosition screen horizontal position (between 0 and 1) to match
     */
    virtual void updateViewPosition(uint32_t samplePosition, float screenHorizontalPosition) = 0;

    /**
     * @brief Scale the view (zooming in or out) to samplesPerPixel, and ensure that
     * referenceHorizontalScreenPosition (a horizontal screen position between 0 and 1) stays
     * at the same position (allow to zoom on a specific section).
     * @param samplesPerPixel number of audio samples per pixel to display in the viewer
     * @param referenceHorizontalScreenPosition horizontal screen position between 0 and 1 of a reference point that
     * should not move
     */
    virtual void updateViewScale(uint32_t samplesPerPixel, float referenceHorizontalScreenPosition) = 0;

    /**
     * @brief Add the fft data inside the task struct to the currently displayed data.
     *
     * @param fftData struct containing the FFt data position, length, channel info and data
     */
    virtual void displayNewFftData(std::shared_ptr<NewFftDataTask> fftData) = 0;
};