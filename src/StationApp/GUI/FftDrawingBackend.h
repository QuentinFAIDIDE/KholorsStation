#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include <cstdint>

/**
 * @brief Abstract class that receives the audio data (sfft freqs intensities) to display
 * on screen.
 */
class FftDrawingBackend : public juce::Component
{
  public:
    virtual ~FftDrawingBackend() = 0;

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
     * @brief Add an intensity point to be displayed on screen. It will only be showed when regerateTextures
     * will have been called.
     *
     * @param trackIdentifier identifier of the track to store intensities for.
     * @param samplePosition position in audioSamples at which this fft appears
     * @param frequency frequency at which to add an intensity in Hz
     * @param intensityDB intensity in dB of the band in kHz
     * @param isTopSide if the drawing is made on the top or bottom band (left and right for stereo)
     */
    virtual void addIntensity(uint64_t trackIdentifier, uint32_t samplePosition, float frequency, float intensityDB,
                              bool isTopSide) = 0;

    /**
     * @brief Coalesce intensities into textures to be displayed on screen.
     */
    virtual void generateTextures() = 0;

    /**
     * @brief Clear all intensities in that range of audio samples position.
     *
     * @param startSamplePosition start position in audio samples to clear
     * @param stopSamplePosition end position in audio samples to clear
     */
    virtual void clearMemoryInRange(uint32_t startSamplePosition, uint32_t stopSamplePosition) = 0;
};