#pragma once

#include "StationApp/GUI/FftDrawingBackend.h"
#include <memory>

#define IMAGES_RING_BUFFER_SIZE 512
#define VISUAL_SAMPLE_RATE 48000
#define FFT_TILE_WIDTH 128
#define FFT_TILE_HEIGHT 512
#define MIN_DB -64.0f

class CpuImageDrawingBackend : public FftDrawingBackend
{
  public:
    CpuImageDrawingBackend();

    void paint(juce::Graphics &g) override;

    /**
     * @brief Tells whether an in image is positioned
     * in the top or bottom section (can also be across both).
     * Top is left and right is bottom.
     */
    enum ImageChannelPosition
    {
        TOP,
        BOTTOM,
        TOP_AND_BOTTOM,
    };

    /**
     * @brief Image to show with its position.
     */
    struct ImageWithPosition
    {
        uint64_t trackIdentifer;
        juce::Image img;
        int64_t samplePosition;
        int64_t duration;
        ImageChannelPosition verticalPos;
    };

    /**
     * @brief Move the view so that the position at the screenHorizontalPosition (between 0 and 1)
     * matches the samplePosition (audio sample offset of the song).
     *
     * @param samplePosition audio sample position (audio sample offset of the song) to match
     * @param screenHorizontalPosition screen horizontal position (between 0 and 1) to match
     */
    void updateViewPosition(uint32_t samplePosition, float screenHorizontalPosition) override;

    /**
     * @brief Scale the view (zooming in or out) to samplesPerPixel, and ensure that
     * referenceHorizontalScreenPosition (a horizontal screen position between 0 and 1) stays
     * at the same position (allow to zoom on a specific section).
     * @param samplesPerPixel number of audio samples per pixel to display in the viewer
     * @param referenceHorizontalScreenPosition horizontal screen position between 0 and 1 of a reference point that
     * should not move
     */
    void updateViewScale(uint32_t samplesPerPixel, float referenceHorizontalScreenPosition) override;

    /**
     * @brief Add the fft data inside the task struct to the currently displayed data.
     *
     * @param fftData struct containing the FFt data position, length, channel info and data
     */
    void displayNewFftData(std::shared_ptr<NewFftDataTask> fftData) override;

  private:
    std::vector<std::shared_ptr<ImageWithPosition>>
        imageRingBuffer;                 /**< Vector of image to show witth their position  */
    size_t imageRingBufferNextItemIndex; /**< Index of next image to set */
    std::mutex imagesMutex;              /**< Mutex to protect concurrent image access */

    int64_t viewPosition;
    int64_t viewScale;
};