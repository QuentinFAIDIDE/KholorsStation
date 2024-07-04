#include "TrackList.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include <limits>
#include <memory>

TrackList::TrackList()
{
    tilesRingBuffer.reserve(TILES_RING_BUFFER_SIZE);
}

void TrackList::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(COLOR_WHITE.withAlpha(0.2f));
    g.fillRect(getLocalBounds().removeFromLeft(1));
}

void TrackList::recordSfft(std::shared_ptr<NewFftDataTask> newSffts)
{

    size_t fftNumFreqBins = newSffts->fftData->size() / newSffts->noFFTs;
    float activeTrackThresold = fftNumFreqBins * ACTIVE_TRACK_DB_THRESOLD_PER_BIN;
    int64_t fftSampleWidth = (int64_t)newSffts->segmentSampleLength / (int64_t)newSffts->noFFTs;

    // for each sfft
    for (size_t i = 0; i < newSffts->noFFTs; i++)
    {
        //   compute sum and avg freq
        float dBsum = 0.0f;
        float avgFreq = 0.0f;
        for (size_t j = 0; j < fftNumFreqBins; j++)
        {
            float intensityAtFreqBin = (*newSffts->fftData)[(i * fftNumFreqBins) + j];
            float binAvgFrequency = float(VISUAL_SAMPLE_RATE >> 1) * ((float(j) + 0.5f) / float(fftNumFreqBins));
            dBsum += intensityAtFreqBin;
            avgFreq += binAvgFrequency * intensityAtFreqBin;
        }
        if (dBsum > std::numeric_limits<float>::epsilon())
        {
            avgFreq /= dBsum;
        }
        else
        {
            avgFreq = VISUAL_SAMPLE_RATE >> 2;
        }
        //   if below a thresold of volume, skip it
        if (dBsum < activeTrackThresold)
        {
            continue;
        }
        // compute its position and tile index
        int64_t startSample = newSffts->segmentStartSample + ((int64_t)i * fftSampleWidth);
        int64_t endSample = startSample + fftSampleWidth;
        if (newSffts->sampleRate != VISUAL_SAMPLE_RATE)
        {
            float sampleRateRatio = float(VISUAL_SAMPLE_RATE) / float(newSffts->sampleRate);
            startSample = float(startSample) * sampleRateRatio;
            endSample = float(endSample) * sampleRateRatio;
        }
        int64_t secondTileIndexStartSample = startSample / VISUAL_SAMPLE_RATE;
        int64_t secondTileIndexEndSample = endSample / VISUAL_SAMPLE_RATE;
        // for each second tile it covers
        for (int64_t j = secondTileIndexStartSample; j <= secondTileIndexEndSample; j++)
        {
            if (j < 0)
            {
                continue;
            }
            // if the tile does not exist, create it
            size_t tileIndex = getOrCreateTile(j);
            //   if track not recorded in tile
            int64_t trackIndexInTile = getTrackIndexInTile(tileIndex, newSffts->trackIdentifier);
            if (trackIndexInTile < 0)
            {
                // this tile is full, we need to abort :'(
                continue;
            }
            // TODO: add tile summed intensities to the right channel(s)
            // TODO: add tile average freq to the right channel(s)
        }
    }
}