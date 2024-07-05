#include "TrackList.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include <limits>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

TrackList::TrackList()
{
    tilesRingBuffer.reserve(TILES_RING_BUFFER_SIZE);
    nextTileIndex = 0;
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
            std::lock_guard lock(tilesMutex);
            // if the tile does not exist, create it
            size_t tileIndex = getOrCreateTile(j);
            //   if track not recorded in tile
            int64_t trackIndexInTile = getTrackIndexInTileOrInsert(tileIndex, newSffts->trackIdentifier);
            if (trackIndexInTile < 0)
            {
                // this tile is full, we need to abort :'(
                spdlog::warn("Aborted recording track activity in tile because a one-second tile had more than maximum "
                             "number of tracks active.");
                continue;
            }
            // add tile summed intensities to the channel(s)
            // add tile average freq to the channel(s)
            // note for summing to average that if we got previous count, we have:
            // previous avg value A = (1/n) + sum_n(a_i)
            // if new value is a_n+1
            // new avg value B = (1/(n+1)) + sum_n+1(a_i) = (n/n+1)*A + (1/(n+1))*a_n+1
            // We will use that to keep it averaged so that channel with different
            // number of FFT per left/right channels can be compared on the spot safely and efficiently.

            if (newSffts->totalNoChannels == 1 || newSffts->channelIndex == 0)
            {
                tilesRingBuffer[tileIndex].summedLeftChanIntensities[trackIndexInTile] += dBsum;
                float n = float(tilesRingBuffer[tileIndex].leftChanFftCount[trackIndexInTile]);
                tilesRingBuffer[tileIndex].averageLeftChanFrequencies[trackIndexInTile] =
                    ((1.0f / (n + 1.0f)) * avgFreq) +
                    ((n / (n + 1.0f)) * tilesRingBuffer[tileIndex].averageLeftChanFrequencies[trackIndexInTile]);
                tilesRingBuffer[tileIndex].leftChanFftCount[trackIndexInTile] += 1;
            }

            if (newSffts->totalNoChannels == 1 || newSffts->channelIndex == 1)
            {
                tilesRingBuffer[tileIndex].summedRightChanIntensities[trackIndexInTile] += dBsum;
                float n = float(tilesRingBuffer[tileIndex].rightChanFftCount[trackIndexInTile]);
                tilesRingBuffer[tileIndex].averageRightChanFrequencies[trackIndexInTile] =
                    ((1.0f / (n + 1.0f)) * avgFreq) +
                    ((n / (n + 1.0f)) * tilesRingBuffer[tileIndex].averageRightChanFrequencies[trackIndexInTile]);
                tilesRingBuffer[tileIndex].rightChanFftCount[trackIndexInTile] += 1;
            }
        }
    }
}

size_t TrackList::getOrCreateTile(int64_t tilePosition)
{
    auto existingTile = indexOfTilesWithData.find(tilePosition);
    if (existingTile != indexOfTilesWithData.end())
    {
        return existingTile->second;
    }
    else
    {
        size_t newTileIndex = nextTileIndex;

        if (tilesRingBuffer.size() < TILES_RING_BUFFER_SIZE)
        {
            // ring buffer has not yet reached its full size
            // we will extend it
            tilesRingBuffer.emplace_back();
        }

        // clear the previous (replaced) tile from the tile index
        indexOfTilesWithData.erase(tilesRingBuffer[newTileIndex].tilePosition);
        indexOfTilesWithData[tilePosition] = newTileIndex;

        // clear all values from the tile
        tilesRingBuffer[newTileIndex].tilePosition = tilePosition;
        tilesRingBuffer[newTileIndex].numSeenTracks = 0;
        for (size_t i = 0; i < MAX_TRACKS_PER_TILE; i++)
        {
            tilesRingBuffer[newTileIndex].summedLeftChanIntensities[i] = 0.0f;
            tilesRingBuffer[newTileIndex].summedRightChanIntensities[i] = 0.0f;
            tilesRingBuffer[newTileIndex].averageLeftChanFrequencies[i] = 0.0f;
            tilesRingBuffer[newTileIndex].averageRightChanFrequencies[i] = 0.0f;
            tilesRingBuffer[newTileIndex].leftChanFftCount[i] = 0;
            tilesRingBuffer[newTileIndex].rightChanFftCount[i] = 0;
        }

        nextTileIndex++;
        if (nextTileIndex >= TILES_RING_BUFFER_SIZE)
        {
            nextTileIndex = 0;
        }

        return newTileIndex;
    }
}

int64_t TrackList::getTrackIndexInTileOrInsert(size_t tileIndex, uint64_t trackIdentifier)
{
    // first step is to search for the track in the tile
    for (size_t i = 0; i < tilesRingBuffer[tileIndex].numSeenTracks; i++)
    {
        if (tilesRingBuffer[tileIndex].trackIdentifiers[i] == trackIdentifier)
        {
            return (int64_t)i;
        }
    }
    // if we reach here, the track was not found
    // if there is no more space abort with -1
    if (tilesRingBuffer[tileIndex].numSeenTracks == MAX_TRACKS_PER_TILE)
    {
        return -1;
    }
    // otherwise we insert it
    size_t newTrackIndex = tilesRingBuffer[tileIndex].numSeenTracks;
    tilesRingBuffer[tileIndex].trackIdentifiers[newTrackIndex] = trackIdentifier;
    tilesRingBuffer[tileIndex].numSeenTracks += 1;

    return (int64_t)newTrackIndex;
}

void TrackList::clear()
{
    std::lock_guard lock(tilesMutex);
    for (size_t i = 0; i < tilesRingBuffer.size(); i++)
    {
        tilesRingBuffer[i].numSeenTracks = 0;
    }
}

std::vector<TrackList::TrackPresenceSummary> TrackList::getVisibleTracks(int64_t startTile, int64_t endTile)
{
    std::vector<TrackPresenceSummary> response;
    std::map<int64_t, size_t> existingTracks;
    for (int64_t currentTile = startTile; currentTile <= endTile; currentTile++)
    {
        // if the tile exist, get its index, otherwise skip
        // for each track in the tile
        //   if not existing in response array yet, create an entry
        //   add left channel volume, remove right channel ones
        //   add total of both channels
        //   sum average frequencies and append to noFreqAvg
    }
    // for each track
    //   compute final stereo balance
    //   divide and set avgFreq

    return response;
}