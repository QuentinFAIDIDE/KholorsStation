#include "CpuImageDrawingBackend.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

CpuImageDrawingBackend::CpuImageDrawingBackend(TrackInfoStore &tis)
    : FftDrawingBackend(tis), viewPosition(0), viewScale(150), secondTileNextIndex(0), tilesNonce(0),
      lastDrawTilesNonce(0)
{
    startTimer(CPU_IMAGE_FFT_BACKEND_UPDATE_INTERVAL_MS);
}

CpuImageDrawingBackend::~CpuImageDrawingBackend()
{
}

void CpuImageDrawingBackend::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    std::lock_guard lock(imageAccessMutex);
    lastDrawTilesNonce = tilesNonce;
    int64_t tileWidth = float(VISUAL_SAMPLE_RATE) / viewScale;
    for (size_t i = 0; i < secondTilesRingBuffer.size(); i++)
    {
        auto trackColor = trackInfoStore.getTrackColor(secondTilesRingBuffer[i].trackIdentifer);
        g.setColour(juce::Colour(trackColor->red, trackColor->green, trackColor->blue));
        TrackSecondTile &tileToDraw = secondTilesRingBuffer[i];
        int64_t horizontalPixelPos = int64_t(float(tileToDraw.samplePosition - viewPosition) / float(viewScale));
        auto imagePosition = getLocalBounds().withX(getLocalBounds().getX() + horizontalPixelPos);
        imagePosition.setWidth(tileWidth);
        if (getLocalBounds().intersects(imagePosition))
        {
            g.drawImage(tileToDraw.img, imagePosition.toFloat(), juce::RectanglePlacement::stretchToFit, true);
        }
    }
}

void CpuImageDrawingBackend::updateViewPosition(uint32_t samplePosition)
{
    viewPosition = samplePosition;
}

void CpuImageDrawingBackend::updateViewScale(uint32_t samplesPerPixel)
{
    viewScale = samplesPerPixel;
}

void CpuImageDrawingBackend::displayNewFftData(std::shared_ptr<NewFftDataTask> fftData)
{
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

void CpuImageDrawingBackend::drawFftOnTile(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t begin,
                                           int64_t end, int fftSize, float *data, int channel)
{
    std::lock_guard lock(imageAccessMutex);
    // if the tile does not exists, create it
    TrackSecondTile *tileToDrawIn = nullptr;
    auto existingTrackTile =
        tileIndexByTrackIdAndPosition.find(std::pair<uint64_t, int64_t>(trackIdentifier, secondTileIndex));
    if (existingTrackTile != tileIndexByTrackIdAndPosition.end())
    {
        tileToDrawIn = &secondTilesRingBuffer[existingTrackTile->second];
    }
    else
    {
        tileToDrawIn = createSecondTile(trackIdentifier, secondTileIndex);
    }
    // draw the fft inside the tile
    size_t startPixel = (size_t)juce::jlimit(
        0, SECOND_TILE_WIDTH - 1, (int)((float(begin) / float(VISUAL_SAMPLE_RATE)) * float(SECOND_TILE_WIDTH)));
    size_t endPixel = (size_t)juce::jlimit(0, SECOND_TILE_WIDTH - 1,
                                           (int)((float(end) / float(VISUAL_SAMPLE_RATE)) * float(SECOND_TILE_WIDTH)));
    // iterate from left to right
    for (size_t horizontalPixel = startPixel; horizontalPixel <= endPixel; horizontalPixel++)
    {
        // iterate from center towards borders
        for (size_t verticalPos = 0; verticalPos < (SECOND_TILE_HEIGHT >> 1); verticalPos++)
        {
            size_t frequencyBinIndex = ((float(verticalPos) * float(fftSize)) / float(SECOND_TILE_HEIGHT >> 1) + 0.5f);
            frequencyBinIndex = (size_t)juce::jlimit(0, fftSize - 1, (int)frequencyBinIndex);
            float intensityDb = data[frequencyBinIndex];
            float intensityNormalized = juce::jmap(intensityDb, MIN_DB, 0.0f, 0.0f, 1.0f);
            if (channel == 0 || channel == 2)
            {
                tileToDrawIn->img.setPixelAt(horizontalPixel, (SECOND_TILE_HEIGHT >> 1) - verticalPos,
                                             juce::Colour(0.0f, 0.0f, 0.0f, intensityNormalized));
            }
            if (channel == 1 || channel == 2)
            {
                tileToDrawIn->img.setPixelAt(horizontalPixel, (SECOND_TILE_HEIGHT >> 1) + verticalPos,
                                             juce::Colour(0.0f, 0.0f, 0.0f, intensityNormalized));
            }
        }
    }
    // increment the nonce
    tilesNonce++;
}

CpuImageDrawingBackend::TrackSecondTile *CpuImageDrawingBackend::createSecondTile(uint64_t trackIdentifier,
                                                                                  int64_t secondTileIndex)
{
    // throw invalid argument error if the tile already exist at that position for this track
    auto existingTile =
        tileIndexByTrackIdAndPosition.find(std::pair<uint64_t, int64_t>(trackIdentifier, secondTileIndex));
    if (existingTile != tileIndexByTrackIdAndPosition.end())
    {
        throw std::invalid_argument("called createSecondTile for an already existing tile");
    }
    size_t newTileIndex;
    // if the ring buffer is not full, expand it with a new datum, clear it and return it
    if (secondTilesRingBuffer.size() < IMAGES_RING_BUFFER_SIZE)
    {
        secondTilesRingBuffer.push_back(TrackSecondTile());
        if (secondTilesRingBuffer.size() - 1 != secondTileNextIndex)
        {
            throw std::runtime_error(
                "expanding secondTilesRingBuffer but secondTileNextIndex does not match vector size");
        }
        newTileIndex = secondTileNextIndex;
    }
    // if the ring buffer is full, remove nextItem, clear its index and replace it with cleared one before returning it
    else
    {
        newTileIndex = secondTileNextIndex;
        // remove the indexing by track id and position for the previous tile
        tileIndexByTrackIdAndPosition.erase(std::pair<uint64_t, int64_t>(
            secondTilesRingBuffer[newTileIndex].trackIdentifer, secondTilesRingBuffer[newTileIndex].tileIndexPosition));
        // clear signal from the previous object
        for (size_t i = 0; i < SECOND_TILE_WIDTH; i++)
        {
            for (size_t j = 0; j < SECOND_TILE_HEIGHT; j++)
            {
                secondTilesRingBuffer[newTileIndex].img.setPixelAt(i, j, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
            }
        }
    }
    // initialize the new tile metadata and tracking in sets
    secondTilesRingBuffer[newTileIndex].tileIndexPosition = secondTileIndex;
    secondTilesRingBuffer[newTileIndex].samplePosition = secondTileIndex * VISUAL_SAMPLE_RATE;
    secondTilesRingBuffer[newTileIndex].trackIdentifer = trackIdentifier;
    auto newIndex = std::pair<uint64_t, int64_t>(trackIdentifier, secondTileIndex);
    auto newSetEntry = std::pair<std::pair<uint64_t, int64_t>, size_t>(newIndex, newTileIndex);
    tileIndexByTrackIdAndPosition.insert(newSetEntry);
    // increment the index of the next to be allocated
    secondTileNextIndex++;
    if (secondTileNextIndex == IMAGES_RING_BUFFER_SIZE)
    {
        secondTileNextIndex = 0;
    }
    return &secondTilesRingBuffer[newTileIndex];
}

void CpuImageDrawingBackend::timerCallback()
{
    if (imageAccessMutex.try_lock())
    {
        if (tilesNonce != lastDrawTilesNonce)
        {
            repaint();
        }
        imageAccessMutex.unlock();
    }
}