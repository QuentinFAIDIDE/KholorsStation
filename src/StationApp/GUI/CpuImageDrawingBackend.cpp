#include "CpuImageDrawingBackend.h"
#include "GUIToolkit/Consts.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

CpuImageDrawingBackend::CpuImageDrawingBackend() : viewPosition(0), viewScale(150)
{
    startTimer(CPU_IMAGE_FFT_BACKEND_UPDATE_INTERVAL_MS);
}

CpuImageDrawingBackend::~CpuImageDrawingBackend()
{
}

void CpuImageDrawingBackend::paint(juce::Graphics &g)
{
}

void CpuImageDrawingBackend::updateViewPosition(uint32_t samplePosition, float screenHorizontalPosition)
{
}

void CpuImageDrawingBackend::updateViewScale(uint32_t samplesPerPixel, float referenceHorizontalScreenPosition)
{
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
            drawFftOnTile(fftData->trackIdentifier, j, tileStartSample, tileEndSample, fftSize, fftDataPointer);
        }
    }
}

void CpuImageDrawingBackend::drawFftOnTile(uint64_t trackIdentifier, int64_t secondTileIndex, int64_t begin,
                                           int64_t end, int fftSize, float *data)
{
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
        (int)((float(begin) / float(VISUAL_SAMPLE_RATE)) * float(SECOND_TILE_WIDTH)), 0, SECOND_TILE_WIDTH - 1);
    size_t endPixel = (size_t)juce::jlimit((int)((float(end) / float(VISUAL_SAMPLE_RATE)) * float(SECOND_TILE_WIDTH)),
                                           0, SECOND_TILE_WIDTH - 1);
}

void CpuImageDrawingBackend::timerCallback()
{
    // TODO: if possible to get the lock, check if the nonce changed and eventually trigger a repaint
}