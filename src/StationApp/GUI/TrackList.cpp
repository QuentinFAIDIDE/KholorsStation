#include "TrackList.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>

// this is the number of tiles per seconds
#define SECOND_TILE_RATIO 10

#define TILE_WIDTH (VISUAL_SAMPLE_RATE / 4)

TrackList::TrackList(TrackInfoStore &tis, NormalizedUnitTransformer &nut)
    : viewPosition(0), viewScale(200), freqViewWidth(1000), trackInfoStore(tis), frequencyTransformer(nut)
{
    tilesRingBuffer.reserve(TILES_RING_BUFFER_SIZE);
    nextTileIndex = 0;
}

void TrackList::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(COLOR_WHITE.withAlpha(0.2f));
    g.fillRect(getLocalBounds().removeFromLeft(1));

    int startTile, endTile;
    {
        std::lock_guard lock(viewMutex);
        startTile = viewPosition / TILE_WIDTH;
        // note that getParentWidth is not exactly the freqView width but it's not very important
        // to be precise to select which 1 second tile to use.
        endTile = 1 + ((viewPosition + (getParentWidth() * viewScale)) / TILE_WIDTH);
    }
    auto tracks = getVisibleTracks(startTile, endTile);

    std::vector<juce::Rectangle<int>> drawnAreas;
    auto drawableBounds = getLocalBounds().reduced(LABELS_AREA_X_PADDING, LABELS_AREA_Y_PADDING);
    juce::Rectangle<int> labelArea = drawableBounds.withHeight(LABEL_HEIGHT);

    int drawableHeightHalf = drawableBounds.getHeight() / 2;

    for (size_t i = 0; i < tracks.size(); i++)
    {
        auto trackNameOpt = trackInfoStore.getTrackName(tracks[i].trackIdentifier);
        if (!trackNameOpt.has_value())
        {
            spdlog::warn("A track label was not drawn because its name could not be found at track info store!");
            continue;
        }
        std::string trackName = trackNameOpt.value();

        auto trackColorOpt = trackInfoStore.getTrackColor(tracks[i].trackIdentifier);
        if (!trackColorOpt.has_value())
        {
            spdlog::warn("A track label was not drawn because its color could not be found at track info store!");
            continue;
        }
        auto trackColor = juce::Colour(trackColorOpt->red, trackColorOpt->green, trackColorOpt->blue);

        // which channel do we allow to draw based on its usage ?
        bool leftChannelWillBeDrawn, rightChannelWillBeDrawn;
        bool leftChannelCanBeDrawn = tracks[i].stereoBalance == 0 || tracks[i].stereoBalance == 2;
        bool rightChannelCanBeDrawn = tracks[i].stereoBalance == 1 || tracks[i].stereoBalance == 2;

        // We are only allowed one channel, so we take the first one we can.
        // Note that we iteratively try whichever side has space so user will not necessarely
        // see more labels drawn on the left channel.
        if (leftChannelCanBeDrawn)
        {
            leftChannelWillBeDrawn = true;
            rightChannelWillBeDrawn = false;
        }
        else if (rightChannelCanBeDrawn)
        {
            rightChannelWillBeDrawn = true;
            leftChannelWillBeDrawn = false;
        }
        else
        {
            throw std::runtime_error("unexpected boolean error in track labels drawing");
        }

        if (leftChannelWillBeDrawn)
        {
            // do we allow the label to be drawn on any (left/right) side ?
            bool allowedToSwitchChanSide = rightChannelCanBeDrawn;
            // freq ratio from DC to Nyquist
            float frequencyRatio;
            // if we wish to alternate side, we take average of both chans if possible
            if (allowedToSwitchChanSide)
            {
                frequencyRatio =
                    ((tracks[i].leftChannelAvgFreq + tracks[i].rightChannelAvgFreq)) / float(VISUAL_SAMPLE_RATE);
            }
            else
            {
                frequencyRatio = (0.5f * tracks[i].leftChannelAvgFreq) / float(VISUAL_SAMPLE_RATE);
            }
            float freqPositionRatio = frequencyTransformer.transform(frequencyRatio);

            float originalSidePosition = 1.0f - freqPositionRatio;

            auto labelAreaOpt = tryPositioningTrackLabel(labelArea, drawableBounds, allowedToSwitchChanSide,
                                                         freqPositionRatio, originalSidePosition, drawableHeightHalf,
                                                         drawnAreas, tracks[i].trackIdentifier);

            if (labelAreaOpt.has_value())
            {
                drawLabel(g, labelAreaOpt.value(), trackName, trackColor);
                drawnAreas.push_back(labelAreaOpt.value());
            }
        }

        if (rightChannelWillBeDrawn)
        {
            // do we allow the label to be drawn on any (left/right) side ?
            bool allowedToSwitchChanSide = leftChannelCanBeDrawn;
            // freq ratio from DC to Nyquist
            float frequencyRatio;
            // if we wish to alternate side, we take average of both chans if possible
            if (allowedToSwitchChanSide)
            {
                frequencyRatio =
                    ((tracks[i].leftChannelAvgFreq + tracks[i].rightChannelAvgFreq)) / float(VISUAL_SAMPLE_RATE);
            }
            else
            {
                frequencyRatio = (0.5f * tracks[i].rightChannelAvgFreq) / float(VISUAL_SAMPLE_RATE);
            }
            float freqPositionRatio = frequencyTransformer.transform(frequencyRatio);

            float originalSidePosition = 1.0f + freqPositionRatio;

            auto labelAreaOpt = tryPositioningTrackLabel(labelArea, drawableBounds, allowedToSwitchChanSide,
                                                         freqPositionRatio, originalSidePosition, drawableHeightHalf,
                                                         drawnAreas, tracks[i].trackIdentifier);

            if (labelAreaOpt.has_value())
            {
                drawLabel(g, labelAreaOpt.value(), trackName, trackColor);
                drawnAreas.push_back(labelAreaOpt.value());
            }
        }
    }
}

void TrackList::drawLabel(juce::Graphics &g, juce::Rectangle<int> bounds, std::string trackName,
                          juce::Colour trackColor)
{
    auto reducedBounds = bounds.reduced(LABELS_INNER_PADDING);

    g.setColour(trackColor);
    auto circleBounds = reducedBounds.removeFromLeft(reducedBounds.getHeight() + LABELS_CIRCLE_TEXT_PADDING);
    circleBounds.removeFromRight(LABELS_CIRCLE_TEXT_PADDING);
    circleBounds.reduce(CIRCLE_PADDING, CIRCLE_PADDING);
    g.fillEllipse(circleBounds.toFloat());

    g.setColour(COLOR_WHITE);
    g.drawEllipse(circleBounds.toFloat(), 1.0f);

    g.drawText(trackName, reducedBounds, juce::Justification::centredLeft, true);
}

std::optional<juce::Rectangle<int>> TrackList::tryPositioningTrackLabel(
    juce::Rectangle<int> labelRectangle, juce::Rectangle<int> drawableBounds, bool allowedToSwitchChanSide,
    float freqPositionRatio, float labelPositionRatioInDrawableArea, int drawableHeightHalf,
    std::vector<juce::Rectangle<int>> &drawnAreas, uint64_t trackIdentifier)
{
    // This seed is based on track identifier and is either 0 or 1.
    // It is used to choose whether this track label will be shown on
    // left or right channel.

    int sideSeed = trackIdentifier % 2;

    // this is the label area we will try to position
    juce::Rectangle<int> labelCandidatePosition = labelRectangle;

    // reposition while the label candidate intersects already drawn ones or is out of bounds
    // and stop after MAX_ITER_LABEL_POSITIONING
    int iter = 0;
    do
    {
        if (iter >= MAX_ITER_LABEL_POSITIONING)
        {
            return std::nullopt;
        }

        int newPos;

        if (allowedToSwitchChanSide)
        {
            int channelPosDiff;
            float sidePosition;

            // loop on 4 positions below or above current label:
            // UP(iter) - UP(iter) - DOWN(iter) - DOWN(iter)
            if (iter % 4 <= 1)
            {
                channelPosDiff = -((((iter / 4)) * LABEL_HEIGHT));
            }
            else
            {
                channelPosDiff = +((((iter / 4)) * LABEL_HEIGHT));
            }

            // loop on flipping the channel
            // NORMAL - FLIPPED
            if ((iter + sideSeed) % 2 == 0)
            {
                // we flip
                sidePosition = (1.0f + freqPositionRatio);
            }
            else
            {
                // we don't flip
                sidePosition = (1.0f - freqPositionRatio);
            }

            newPos = channelPosDiff + int(float(drawableHeightHalf) * sidePosition) - (LABEL_HEIGHT / 2);
        }
        else
        {
            if (iter % 2 == 0)
            {
                newPos = int(float(drawableHeightHalf) * labelPositionRatioInDrawableArea) +
                         ((((int)iter / 2)) * LABEL_HEIGHT);
            }
            else
            {
                newPos = int(float(drawableHeightHalf) * labelPositionRatioInDrawableArea) -
                         ((((int)iter / 2)) * LABEL_HEIGHT);
            }
        }
        newPos = newPos - (newPos % LABEL_HEIGHT);
        labelCandidatePosition.setY(drawableBounds.getY() + newPos);
        iter++;

    } while (rectIntersectsRectList(labelCandidatePosition, drawnAreas) ||
             !drawableBounds.contains(labelCandidatePosition));

    return labelCandidatePosition;
}

bool TrackList::rectIntersectsRectList(juce::Rectangle<int> &rect, std::vector<juce::Rectangle<int>> &rectangleList)
{
    for (size_t i = 0; i < rectangleList.size(); i++)
    {
        if (rect.intersects(rectangleList[i]))
        {
            return true;
        }
    }
    return false;
}

void TrackList::setViewPosition(int64_t newVP)
{
    std::lock_guard lock(viewMutex);
    viewPosition = newVP;
}

void TrackList::setViewScale(int64_t newVS)
{
    std::lock_guard lock(viewMutex);
    viewScale = newVS;
}

void TrackList::setFreqViewWidth(int64_t newFVW)
{
    std::lock_guard lock(viewMutex);
    freqViewWidth = newFVW;
}

void TrackList::recordSfft(std::shared_ptr<NewFftDataTask> newSffts)
{

    size_t fftNumFreqBins = newSffts->fftData->size() / newSffts->noFFTs;
    int64_t fftSampleWidth = (int64_t)newSffts->segmentSampleLength / (int64_t)newSffts->noFFTs;

    // compute width of each FFT bin shown on screen given
    // the frequency transformation in order to correct
    // average frequency (right now bass are underrepresented)
    std::vector<float> freqWeight;
    freqWeight.reserve(fftNumFreqBins);
    float linearBinWidth = 1.0f / float(fftNumFreqBins);
    for (size_t j = 0; j < fftNumFreqBins; j++)
    {
        float lowerBinBound = frequencyTransformer.transform(j * linearBinWidth);
        float upperBinBound = frequencyTransformer.transform((j + 1) * linearBinWidth);
        float binWidth = upperBinBound - lowerBinBound;
        freqWeight.push_back(binWidth);
    }

    // for each sfft
    for (size_t i = 0; i < newSffts->noFFTs; i++)
    {
        float avgIntensity = 0.0f;
        float avgFreq = 0.0f;
        float totalIntensity = 0.0f;

        for (size_t j = 0; j < fftNumFreqBins; j++)
        {
            // we normalize the db intensity between 0 and 1
            float dbIntensity = (*newSffts->fftData)[(i * fftNumFreqBins) + j];
            float intensityAtFreqBin = (std::abs(MIN_DB) + dbIntensity) / std::abs(MIN_DB);
            intensityAtFreqBin = juce::jlimit(0.0f, 1.0f, intensityAtFreqBin);
            float weightedIntensity = intensityAtFreqBin * freqWeight[j];
            avgIntensity += weightedIntensity;

            float binFrequency = float(VISUAL_SAMPLE_RATE >> 1) * ((float(j) + 0.5f) / float(fftNumFreqBins));
            avgFreq += binFrequency * weightedIntensity;
        }
        avgFreq = avgFreq / avgIntensity;

        //   if below a thresold of volume, skip it
        if (avgIntensity < ACTIVE_TRACK_THRESOLD_PER_BIN)
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
        int64_t secondTileIndexStartSample = startSample / TILE_WIDTH;
        int64_t secondTileIndexEndSample = endSample / TILE_WIDTH;
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
            TracksPlaybackInfoTile &tile = tilesRingBuffer[tileIndex];
            if (newSffts->totalNoChannels == 1 || newSffts->channelIndex == 0)
            {
                tile.summedLeftChanIntensities[trackIndexInTile] += avgIntensity;
                float n = float(tile.leftChanFftCount[trackIndexInTile]);
                tile.averageLeftChanFrequencies[trackIndexInTile] =
                    ((1.0f / (n + 1.0f)) * avgFreq) +
                    ((n / (n + 1.0f)) * tile.averageLeftChanFrequencies[trackIndexInTile]);
                tile.leftChanFftCount[trackIndexInTile] += 1;
            }

            if (newSffts->totalNoChannels == 1 || newSffts->channelIndex == 1)
            {
                tile.summedRightChanIntensities[trackIndexInTile] += avgIntensity;
                float n = float(tile.rightChanFftCount[trackIndexInTile]);
                tile.averageRightChanFrequencies[trackIndexInTile] =
                    ((1.0f / (n + 1.0f)) * avgFreq) +
                    ((n / (n + 1.0f)) * tile.averageRightChanFrequencies[trackIndexInTile]);
                tile.rightChanFftCount[trackIndexInTile] += 1;
            }
        }
    }
}

size_t TrackList::getOrCreateTile(int64_t tilePosition)
{
    auto existingTile = tileIndexesPerPosition.find(tilePosition);
    if (existingTile != tileIndexesPerPosition.end())
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
        tileIndexesPerPosition.erase(tilesRingBuffer[newTileIndex].tilePosition);
        tileIndexesPerPosition[tilePosition] = newTileIndex;

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
    tilesRingBuffer[tileIndex].summedLeftChanIntensities[newTrackIndex] = 0.0f;
    tilesRingBuffer[tileIndex].averageLeftChanFrequencies[newTrackIndex] = 0.0f;
    tilesRingBuffer[tileIndex].summedRightChanIntensities[newTrackIndex] = 0.0f;
    tilesRingBuffer[tileIndex].averageRightChanFrequencies[newTrackIndex] = 0.0f;
    tilesRingBuffer[tileIndex].leftChanFftCount[newTrackIndex] = 0;
    tilesRingBuffer[tileIndex].rightChanFftCount[newTrackIndex] = 0;
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
    std::lock_guard lock(tilesMutex);
    std::vector<TrackPresenceSummary> response;
    std::map<uint64_t, size_t> existingTracks;
    for (int64_t currentTile = startTile; currentTile <= endTile; currentTile++)
    {
        // if the tile exist, get its index, otherwise skip
        auto existingTile = tileIndexesPerPosition.find(currentTile);
        if (existingTile == tileIndexesPerPosition.end())
        {
            continue;
        }
        TracksPlaybackInfoTile &tile = tilesRingBuffer[existingTile->second];
        // for each track in the tile
        for (size_t i = 0; i < tile.numSeenTracks; i++)
        {
            // if not existing in response array yet, create an entry
            auto existingTrackInTile = existingTracks.find(tile.trackIdentifiers[i]);
            size_t trackIndexInResponse;
            if (existingTrackInTile == existingTracks.end())
            {
                trackIndexInResponse = response.size();
                existingTracks[tile.trackIdentifiers[i]] = trackIndexInResponse;
                response.emplace_back();
                response[trackIndexInResponse].trackIdentifier = tile.trackIdentifiers[i];
            }
            else
            {
                trackIndexInResponse = existingTrackInTile->second;
            }
            TrackPresenceSummary &trackSummary = response[trackIndexInResponse];
            // add channels volumes
            trackSummary.allChannel += tile.summedLeftChanIntensities[i];
            trackSummary.allChannel += tile.summedRightChanIntensities[i];
            trackSummary.stereoDiff -= tile.summedLeftChanIntensities[i];
            trackSummary.stereoDiff += tile.summedRightChanIntensities[i];
            // add channels average frequencies
            if (tile.leftChanFftCount[i] != 0)
            {
                trackSummary.noFreqsAveragedLeft++;
                trackSummary.leftChannelAvgFreq += tile.averageLeftChanFrequencies[i];
            }
            if (tile.rightChanFftCount[i] != 0)
            {
                trackSummary.noFreqsAveragedRight++;
                trackSummary.rightChannelAvgFreq += tile.averageRightChanFrequencies[i];
            }
        }
    }
    // for each track
    for (size_t i = 0; i < response.size(); i++)
    {
        // set left channel average freq
        if (response[i].noFreqsAveragedLeft > 0)
        {
            response[i].leftChannelAvgFreq = response[i].leftChannelAvgFreq / float(response[i].noFreqsAveragedLeft);
        }
        else
        {
            response[i].leftChannelAvgFreq = VISUAL_SAMPLE_RATE / 2.0f;
        }

        // set right channel average freq
        if (response[i].noFreqsAveragedRight > 0)
        {
            response[i].rightChannelAvgFreq = response[i].rightChannelAvgFreq / float(response[i].noFreqsAveragedRight);
        }
        else
        {
            response[i].leftChannelAvgFreq = VISUAL_SAMPLE_RATE / 2.0f;
        }

        // decide on stereo balance
        float balance = response[i].stereoDiff / response[i].allChannel;
        if (balance < (-BALANCE_SWITCH_THRESOLD))
        {
            response[i].stereoBalance = 0;
        }
        else if (balance > BALANCE_SWITCH_THRESOLD)
        {
            response[i].stereoBalance = 1;
        }
        else
        {
            response[i].stereoBalance = 2;
        }
    }

    std::sort(response.begin(), response.end(), TrackList::compareTrackPresence);
    return response;
}

bool TrackList::compareTrackPresence(const TrackPresenceSummary &a, const TrackPresenceSummary &b)
{
    return a.trackIdentifier < b.trackIdentifier;
}

void TrackList::clearTrackFromRange(uint64_t trackIdientifier, int64_t startSample, uint64_t length)
{
    std::lock_guard lock(tilesMutex);

    // generate indices in TrackList tiles positions
    int64_t startTile = (float(startSample) / float(TILE_WIDTH)) + 0.5f;
    int64_t endTile = startTile + int64_t((float(length) / float(TILE_WIDTH)) + 0.5f) - 1;

    for (int64_t i = startTile; i <= endTile; i++)
    {
        // if a TrackList tile exist with that position index get it
        auto existingTrackListTile = tileIndexesPerPosition.find(i);
        if (existingTrackListTile == tileIndexesPerPosition.end())
        {
            continue;
        }
        TracksPlaybackInfoTile &tile = tilesRingBuffer[existingTrackListTile->second];
        tile.clearFromTrackInfo(trackIdientifier);
    }
}

void TrackList::TracksPlaybackInfoTile::clearFromTrackInfo(uint64_t trackIdentifier)
{
    for (size_t i = 0; i < numSeenTracks; i++)
    {
        if (trackIdentifiers[i] == trackIdentifier)
        {
            // if it's the last tile, just shrink the array size
            if (i == numSeenTracks - 1)
            {
                numSeenTracks--;
                return;
            }
            else
            {
                // otherwise we move last tile to this index and shrink array size (numSeenTracks)
                size_t lastTileIndex = numSeenTracks - 1;
                trackIdentifiers[i] = trackIdentifiers[lastTileIndex];
                summedLeftChanIntensities[i] = summedLeftChanIntensities[lastTileIndex];
                averageLeftChanFrequencies[i] = averageLeftChanFrequencies[lastTileIndex];
                summedRightChanIntensities[i] = summedRightChanIntensities[lastTileIndex];
                averageRightChanFrequencies[i] = averageRightChanFrequencies[lastTileIndex];
                leftChanFftCount[i] = leftChanFftCount[lastTileIndex];
                rightChanFftCount[i] = rightChanFftCount[lastTileIndex];
                numSeenTracks = numSeenTracks - 1;
                return;
            }
        }
    }
}