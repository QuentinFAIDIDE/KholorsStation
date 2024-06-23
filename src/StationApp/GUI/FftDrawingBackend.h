#pragma once

#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
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
    FftDrawingBackend(TrackInfoStore &tis) : trackInfoStore(tis)
    {
        setInterceptsMouseClicks(false, false);
    };

    /**
     * @brief Set the Frequency Transformer object that will be applied to frequencies displayed.
     * Not meant to be changed as not under any lock and used by multiple threads.
     *
     * @param ut new frequency transformer
     */
    void setFrequencyTransformer(NormalizedUnitTransformer *ut)
    {
        freqTransformer = ut;
    }

    /**
     * @brief Set the Intensity Transformer object that will be applied to FFT intensities displayed.
     * Not meant to be changed as not under any lock and used by multiple threads.
     *
     * @param it new intensity transformer
     */
    void setIntensityTransformer(NormalizedUnitTransformer *it)
    {
        intensityTransformer = it;
    }

    /**
     * @brief Move the view so that the position at the component left
     * matches the samplePosition (audio sample offset of the song).
     *
     * @param samplePosition audio sample position (audio sample offset of the song) to match
     */
    virtual void updateViewPosition(uint32_t samplePosition) = 0;

    /**
     * @brief Scale the view
     * @param samplesPerPixel number of audio samples per pixel to display in the viewer
     */
    virtual void updateViewScale(uint32_t samplesPerPixel) = 0;

    /**
     * @brief Add the fft data inside the task struct to the currently displayed data.
     *
     * @param fftData struct containing the FFt data position, length, channel info and data
     */
    virtual void displayNewFftData(std::shared_ptr<NewFftDataTask> fftData) = 0;

  protected:
    TrackInfoStore &trackInfoStore;
    NormalizedUnitTransformer *freqTransformer;
    NormalizedUnitTransformer *intensityTransformer;
};