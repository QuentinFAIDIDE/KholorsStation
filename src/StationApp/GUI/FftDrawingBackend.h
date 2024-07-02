#pragma once

#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstdint>
#include <memory>

#define VISUAL_SAMPLE_RATE 48000
#define IMAGES_RING_BUFFER_SIZE 128
#define MIN_DB -64.0f
#define SECOND_TILE_WIDTH 64
#define SECOND_TILE_HEIGHT 512

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
    void displayNewFftData(std::shared_ptr<NewFftDataTask> fftData)
    {
        if (freqTransformer == nullptr || intensityTransformer == nullptr)
        {
            throw std::invalid_argument(
                "Image drawing backend is being used without freq or intensity transformers set");
        }

        int fftSize = fftData->fftData->size() / fftData->noFFTs;
        int64_t fftSampleWidth = (int64_t)fftData->segmentSampleLength / (int64_t)fftData->noFFTs;
        // for each fft in the received set
        for (size_t i = 0; i < fftData->noFFTs; i++)
        {
            // pointer to the raw data for this fft
            float *fftDataPointer = fftData->fftData->data() + ((size_t)fftSize * i);
            // compute its position and tile index
            int64_t startSample = fftData->segmentStartSample + ((int64_t)i * fftSampleWidth);
            int64_t endSample = startSample + fftSampleWidth;
            if (fftData->sampleRate != VISUAL_SAMPLE_RATE)
            {
                float sampleRateRatio = float(VISUAL_SAMPLE_RATE) / float(fftData->sampleRate);
                startSample = float(startSample) * sampleRateRatio;
                endSample = float(endSample) * sampleRateRatio;
            }
            int64_t secondTileIndexStartSample = startSample / VISUAL_SAMPLE_RATE;
            int64_t secondTileIndexEndSample = endSample / VISUAL_SAMPLE_RATE;
            // fill the tile (one or many) with the fft data
            for (int64_t j = secondTileIndexStartSample; j <= secondTileIndexEndSample; j++)
            {
                if (j < 0)
                {
                    continue;
                }
                // if it's the first tile, the start sample is the modulo of the startSample
                // otherwise it's the tile start
                int64_t tileStartSample = 0;
                if (j == secondTileIndexStartSample)
                {
                    tileStartSample = startSample % VISUAL_SAMPLE_RATE;
                }
                // same reasoning for the end sample within the tile
                int64_t tileEndSample = VISUAL_SAMPLE_RATE - 1;
                if (j == secondTileIndexEndSample)
                {
                    tileEndSample = endSample % VISUAL_SAMPLE_RATE;
                }
                // we can now write the fft into the tile (and eventually create it)
                int channelIndex = 2;              // 0 for left, 1 for right, 2 for both
                if (fftData->totalNoChannels == 2) // if there are two channel (not mono), this is for one specific
                {
                    if (fftData->channelIndex == 0)
                    {
                        channelIndex = 0;
                    }
                    else
                    {
                        channelIndex = 1;
                    }
                }
                drawFftOnTile(fftData->trackIdentifier, j, tileStartSample, tileEndSample, fftSize, fftDataPointer,
                              channelIndex);
            }
        }
    }

    /**
     * @brief Set the color of a track. Used only by GPU drawing backend.
     *
     * @param trackIdentifier identifier of the track to change color of
     * @param col color to apply to the track
     */
    virtual void setTrackColor(uint64_t trackIdentifier, juce::Colour col)
    {
    }

  protected:
    /**
     * @brief Draws the provided FFT (there's only one) on the TrackSecondTile;
     *
     * @param trackIdentifier identifier of the track this fft is for
     * @param secondTileIndex index of the second-tile (in seconds starting at zero)
     * @param begin start sample in the tile
     * @param end end sample in the tile
     * @param fftSize number of frequency bins in the provided fft
     * @param data pointer to the floats containing fft bins intensities in decibels
     * @param channel 0 for left, 1 for right, 2 for both
     */
    virtual void drawFftOnTile(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t begin, int64_t end,
                               int fftSize, float *data, int channel) = 0;

    TrackInfoStore &trackInfoStore;
    NormalizedUnitTransformer *freqTransformer;
    NormalizedUnitTransformer *intensityTransformer;

    int64_t tilesNonce;          /**< A nonce that is incremented when the tiles are updated */
    std::mutex imageAccessMutex; /**< Mutex to protect image access */
};