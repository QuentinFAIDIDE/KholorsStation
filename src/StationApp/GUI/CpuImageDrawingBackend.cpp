#include "CpuImageDrawingBackend.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include <cstddef>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>

CpuImageDrawingBackend::CpuImageDrawingBackend(TrackInfoStore &tis, NormalizedUnitTransformer &ft,
                                               NormalizedUnitTransformer &it)
    : FftDrawingBackend(tis, ft, it), viewPosition(0), viewScale(150), secondTileNextIndex(0), lastDrawTilesNonce(0)
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

int64_t CpuImageDrawingBackend::getTileIndexIfExists(uint64_t trackIdentifier, int64_t secondTileIndex)
{
    auto searchTerms = std::pair<uint64_t, int64_t>(trackIdentifier, secondTileIndex);
    auto tileSearched = tileIndexByTrackIdAndPosition.find(searchTerms);
    if (tileSearched != tileIndexByTrackIdAndPosition.end())
    {
        return (int64_t)tileSearched->second;
    }
    else
    {
        return -1;
    }
}

void CpuImageDrawingBackend::setTilePixelIntensity(size_t tileRingBufferIndex, int x, int y, float intensity)
{
    secondTilesRingBuffer[tileRingBufferIndex].img.setPixelAt(x, y, juce::Colour(0.0f, 0.0f, 0.0f, intensity));
}

size_t CpuImageDrawingBackend::createSecondTile(uint64_t trackIdentifier, int64_t secondTileIndex)
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
        secondTilesRingBuffer.emplace_back();
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
    return newTileIndex;
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

void CpuImageDrawingBackend::drawFftOnTile(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t begin,
                                           int64_t end, int fftSize, float *data, int channel)
{
    std::lock_guard lock(imageAccessMutex);
    // if the tile does not exists, create it
    size_t tileToDrawIn;
    auto existingTrackTile = getTileIndexIfExists(trackIdentifier, secondTileIndex);
    if (existingTrackTile >= 0)
    {
        tileToDrawIn = (size_t)existingTrackTile;
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
            size_t frequencyBinIndex =
                ((float(fftSize) * freqTransformer.transformInv(float(verticalPos) / float(SECOND_TILE_HEIGHT >> 1))) +
                 0.5f);
            frequencyBinIndex = (size_t)juce::jlimit(0, fftSize - 1, (int)frequencyBinIndex);
            float intensityDb = data[frequencyBinIndex];
            float intensityNormalized = juce::jmap(intensityDb, MIN_DB, 0.0f, 0.0f, 1.0f);
            intensityNormalized = juce::jlimit(0.0f, 1.0f, intensityNormalized);
            intensityNormalized = intensityTransformer.transformInv(intensityNormalized);
            if (channel == 0 || channel == 2)
            {
                setTilePixelIntensity(tileToDrawIn, horizontalPixel, (SECOND_TILE_HEIGHT >> 1) - verticalPos,
                                      intensityNormalized);
            }
            if (channel == 1 || channel == 2)
            {
                setTilePixelIntensity(tileToDrawIn, horizontalPixel, (SECOND_TILE_HEIGHT >> 1) + verticalPos,
                                      intensityNormalized);
            }
        }
    }
    // increment the nonce to let timer know we need to trigger an update
    tilesNonce++;
}