#include "CpuImageDrawingBackend.h"
#include "GUIToolkit/Consts.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>

CpuImageDrawingBackend::CpuImageDrawingBackend() : viewPosition(0), viewScale(500)
{
    imageRingBuffer.resize(IMAGES_RING_BUFFER_SIZE);
    for (size_t i = 0; i < IMAGES_RING_BUFFER_SIZE; i++)
    {
        imageRingBuffer[i] = nullptr;
    }
    imageRingBufferNextItemIndex = 0;
}

void CpuImageDrawingBackend::paint(juce::Graphics &g)
{
    std::lock_guard lock(imagesMutex);

    // TODO: replace this with a color fetching per track identifier (require color storage)
    g.setColour(COLOR_WHITE);

    auto bounds = getBounds();
    for (size_t i = 0; i < imageRingBuffer.size(); i++)
    {
        if (imageRingBuffer[i] != nullptr)
        {
            auto imagePosition = bounds;
            // set the horizontal position
            auto startPixel = (imageRingBuffer[i]->samplePosition - viewPosition) / viewScale;
            auto endPixel = startPixel + (imageRingBuffer[i]->duration / viewScale);
            imagePosition.setX(bounds.getX() + startPixel);
            imagePosition.setWidth(endPixel);
            // ignore if out of bounds
            if (!bounds.intersects(imagePosition))
            {
                continue;
            }
            // set vertical position
            if (imageRingBuffer[i]->verticalPos == TOP)
            {
                imagePosition = imagePosition.removeFromTop(imagePosition.getHeight() / 2);
            }
            else if (imageRingBuffer[i]->verticalPos == BOTTOM)
            {
                imagePosition = imagePosition.removeFromBottom(imagePosition.getHeight() / 2);
            }
            // proceed to draw the image
            g.drawImage(imageRingBuffer[i]->img, imagePosition.toFloat(), juce::RectanglePlacement::stretchToFit, true);
        }
    }
}

void CpuImageDrawingBackend::updateViewPosition(uint32_t samplePosition, float screenHorizontalPosition)
{
}

void CpuImageDrawingBackend::updateViewScale(uint32_t samplesPerPixel, float referenceHorizontalScreenPosition)
{
}

void CpuImageDrawingBackend::displayNewFftData(std::shared_ptr<NewFftDataTask> fftData)
{
    std::lock_guard lock(imagesMutex);
    imageRingBuffer[imageRingBufferNextItemIndex] = std::make_shared<ImageWithPosition>();

    // compute the ratio of sample rate change between received and displayed
    if (fftData->sampleRate != VISUAL_SAMPLE_RATE)
    {
        float sampleRateRatio = float(VISUAL_SAMPLE_RATE) / float(fftData->sampleRate);
        imageRingBuffer[imageRingBufferNextItemIndex]->samplePosition =
            (int64_t)(float(fftData->segmentStartSample) * sampleRateRatio);
        imageRingBuffer[imageRingBufferNextItemIndex]->duration =
            (int64_t)(float(fftData->segmentSampleLength) * sampleRateRatio);
    }
    else
    {
        imageRingBuffer[imageRingBufferNextItemIndex]->samplePosition = (int64_t)(fftData->segmentStartSample);
        imageRingBuffer[imageRingBufferNextItemIndex]->duration = (int64_t)(fftData->segmentSampleLength);
    }

    // set track channel info for vertical pos and track identifier
    imageRingBuffer[imageRingBufferNextItemIndex]->trackIdentifer = fftData->trackIdentifier;
    if (fftData->totalNoChannels == 1)
    {
        imageRingBuffer[imageRingBufferNextItemIndex]->verticalPos = TOP_AND_BOTTOM;
    }
    else if (fftData->channelIndex == 0)
    {
        imageRingBuffer[imageRingBufferNextItemIndex]->verticalPos = TOP;
    }
    else if (fftData->channelIndex == 1)
    {
        imageRingBuffer[imageRingBufferNextItemIndex]->verticalPos = BOTTOM;
    }
    else
    {
        throw std::invalid_argument("displayNewFftData received a channel index that is neither 0 or 1");
    }

    // draw the ffts image
    uint32_t noFfts = fftData->noFFTs;
    uint32_t noFreqs = fftData->fftData->size() / noFfts;
    imageRingBuffer[imageRingBufferNextItemIndex]->img =
        juce::Image(juce::Image::PixelFormat::SingleChannel, FFT_TILE_WIDTH, FFT_TILE_HEIGHT, true);
    for (size_t i = 0; i < FFT_TILE_WIDTH; i++)
    {
        int horizontalFftPos = int(float(noFfts) * (float(i) / float(FFT_TILE_WIDTH)));
        for (size_t j = 0; j < FFT_TILE_HEIGHT; j++)
        {
            int verticalFrequencyPos = (int)(float(noFreqs) * (float(j) / float(FFT_TILE_HEIGHT)));
            float intensityDb =
                (*fftData->fftData)[((uint32_t)horizontalFftPos * noFreqs) + (uint32_t)verticalFrequencyPos];
            float intensity = juce::jmap(intensityDb, -64.0f, 0.0f);

            switch (imageRingBuffer[imageRingBufferNextItemIndex]->verticalPos)
            {
            case TOP:
                imageRingBuffer[imageRingBufferNextItemIndex]->img.setPixelAt(i, (FFT_TILE_HEIGHT - 1) - j,
                                                                              juce::Colour(0.0f, 0.f, 0.0f, intensity));
                break;

            case BOTTOM:
                imageRingBuffer[imageRingBufferNextItemIndex]->img.setPixelAt(i, j,
                                                                              juce::Colour(0.0f, 0.f, 0.0f, intensity));
                break;

            case TOP_AND_BOTTOM:
                imageRingBuffer[imageRingBufferNextItemIndex]->img.setPixelAt(i, (int)(float(j) / 2.0f),
                                                                              juce::Colour(0.0f, 0.f, 0.0f, intensity));
                imageRingBuffer[imageRingBufferNextItemIndex]->img.setPixelAt(
                    i, (FFT_TILE_HEIGHT) - (int)(float(j) / 2.0f), juce::Colour(0.0f, 0.f, 0.0f, intensity));
                break;
            }
        }
    }
}