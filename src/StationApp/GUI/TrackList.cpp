#include "TrackList.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/AudioConstants.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include "StationApp/GUI/TrackSelectionTask.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <utility>

// this is the number of tiles per seconds
constexpr int SECOND_TILE_RATIO = 10;
constexpr float TRACK_NOT_SELECTED_ALPHA = 0.35f;

constexpr auto TILE_WIDTH = (VISUAL_SAMPLE_RATE / 4);
constexpr int LABEL_HEIGHT = 40;

TrackList::TrackList(TrackInfoStore &tis, NormalizedUnitTransformer &nut, TaskingManager &tm)
    : taskingManager(tm), trackInfoStore(tis), frequencyTransformer(nut)
{
    lastMouseX = 0;
    lastMouseY = 0;
    setOpaque(true);
    tilesRingBuffer.reserve(TILES_RING_BUFFER_SIZE);
    nextTileIndex = 0;
}

void TrackList::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    int startTile = 0;
    int endTile = 0;
    {
        std::scoped_lock const lock(viewMutex);
        startTile = viewPosition / TILE_WIDTH;
        // note that getParentWidth is not exactly the freqView width but it's not very important
        // to be precise to select which 1 second tile to use.
        endTile = 1 + ((viewPosition + (getParentWidth() * viewScale)) / TILE_WIDTH);
    }

    std::vector<TrackPresenceSummary> tracks;

    if (selectedTrack.has_value())
    {
        tracks = lastUsedTrackList;
    }
    else
    {
        tracks = getVisibleTracks(startTile, endTile);
        lastUsedTrackList = tracks;
    }

    std::vector<juce::Rectangle<int>> drawnAreas;
    auto const drawableBounds = getLocalBounds().reduced(LABELS_AREA_X_PADDING, LABELS_AREA_Y_PADDING);
    juce::Rectangle<int> const labelArea = drawableBounds.withHeight(LABEL_HEIGHT);

    int const drawableHeightHalf = drawableBounds.getHeight() / 2;

    tracksLabelPositions.resize(0);

    for (auto const &track : tracks)
    {
        auto trackNameOpt = trackInfoStore.getTrackName(track.trackIdentifier);
        if (!trackNameOpt.has_value())
        {
            spdlog::warn("A track label was not drawn because its name could not be found at track info store!");
            continue;
        }
        std::string const trackName = trackNameOpt.value();

        auto trackColorOpt = trackInfoStore.getTrackColor(track.trackIdentifier);
        if (!trackColorOpt.has_value())
        {
            spdlog::warn("A track label was not drawn because its color could not be found at track info store!");
            continue;
        }
        auto const trackColor = juce::Colour(trackColorOpt->red, trackColorOpt->green, trackColorOpt->blue);

        // which channel do we allow to draw based on its usage ?
        bool leftChannelWillBeDrawn = false;
        bool rightChannelWillBeDrawn = false;
        bool const leftChannelCanBeDrawn = track.stereoBalance == 0 || track.stereoBalance == 2;
        bool const rightChannelCanBeDrawn = track.stereoBalance == 1 || track.stereoBalance == 2;

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
            bool const allowedToSwitchChanSide = rightChannelCanBeDrawn;
            // freq ratio from DC to Nyquist
            float frequencyRatio = NAN;
            // if we wish to alternate side, we take average of both chans if possible
            if (allowedToSwitchChanSide)
            {
                frequencyRatio =
                    ((track.leftChannelAvgFreq + track.rightChannelAvgFreq)) / static_cast<float>(VISUAL_SAMPLE_RATE);
            }
            else
            {
                frequencyRatio = (0.5f * track.leftChannelAvgFreq) / static_cast<float>(VISUAL_SAMPLE_RATE);
            }
            float const freqPositionRatio = frequencyTransformer.transform(frequencyRatio);

            float const originalSidePosition = 1.0f - freqPositionRatio;

            auto labelAreaOpt =
                tryPositioningTrackLabel(labelArea, drawableBounds, allowedToSwitchChanSide, freqPositionRatio,
                                         originalSidePosition, drawableHeightHalf, drawnAreas, track.trackIdentifier);

            if (labelAreaOpt.has_value())
            {
                drawLabel(g, labelAreaOpt.value(), trackName, trackColor, track.trackIdentifier);
                drawnAreas.push_back(labelAreaOpt.value());
                std::pair<uint64_t, juce::Rectangle<int>> const trackLabelPos(track.trackIdentifier,
                                                                              labelAreaOpt.value());
                tracksLabelPositions.push_back(trackLabelPos);
            }
        }

        if (rightChannelWillBeDrawn)
        {
            // do we allow the label to be drawn on any (left/right) side ?
            bool const allowedToSwitchChanSide = leftChannelCanBeDrawn;
            // freq ratio from DC to Nyquist
            float frequencyRatio = NAN;
            // if we wish to alternate side, we take average of both chans if possible
            if (allowedToSwitchChanSide)
            {
                frequencyRatio =
                    ((track.leftChannelAvgFreq + track.rightChannelAvgFreq)) / static_cast<float>(VISUAL_SAMPLE_RATE);
            }
            else
            {
                frequencyRatio = (0.5f * track.rightChannelAvgFreq) / static_cast<float>(VISUAL_SAMPLE_RATE);
            }
            float const freqPositionRatio = frequencyTransformer.transform(frequencyRatio);

            float const originalSidePosition = 1.0f + freqPositionRatio;

            auto labelAreaOpt =
                tryPositioningTrackLabel(labelArea, drawableBounds, allowedToSwitchChanSide, freqPositionRatio,
                                         originalSidePosition, drawableHeightHalf, drawnAreas, track.trackIdentifier);

            if (labelAreaOpt.has_value())
            {
                drawLabel(g, labelAreaOpt.value(), trackName, trackColor, track.trackIdentifier);
                drawnAreas.push_back(labelAreaOpt.value());
                std::pair<uint64_t, juce::Rectangle<int>> const trackLabelPos(track.trackIdentifier,
                                                                              labelAreaOpt.value());
                tracksLabelPositions.push_back(trackLabelPos);
            }
        }
    }
    lastRedrawMs = juce::Time().getCurrentTime().toMilliseconds();

    updateSelectedTrack(lastMouseX, lastMouseY);
}

int64_t TrackList::getLastRedrawMs() const
{
    return lastRedrawMs;
}

void TrackList::updateSelectedTrack(int mouseX, int mouseY)
{
    // TODO: see currently selected track, if changed after running this func, broadcast new TrackSelectionTask
    std::optional<uint64_t> const previouslySelectedTrack = selectedTrack;
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    if (!mouseOnComponent)
    {
        selectedTrack = std::nullopt;
        if (selectedTrack != previouslySelectedTrack)
        {
            broadcastSelectedTrack();
        }
        return;
    }
    for (auto &tracksLabelPosition : tracksLabelPositions)
    {
        if (tracksLabelPosition.second.contains(lastMouseX, lastMouseY))
        {
            selectedTrack = tracksLabelPosition.first;
            if (selectedTrack != previouslySelectedTrack)
            {
                broadcastSelectedTrack();
            }
            return;
        }
    }
    selectedTrack = std::nullopt;
    if (selectedTrack != previouslySelectedTrack)
    {
        broadcastSelectedTrack();
    }
}

void TrackList::broadcastSelectedTrack()
{
    auto const selectionUpdate = std::make_shared<TrackSelectionTask>(selectedTrack);
    taskingManager.broadcastTask(selectionUpdate);
}

void TrackList::mouseMove(const juce::MouseEvent &me)
{
    mouseOnComponent = true;
    updateSelectedTrack(me.position.x, me.position.y);
}

void TrackList::mouseDrag(const juce::MouseEvent &me)
{
    mouseOnComponent = true;
    updateSelectedTrack(me.position.x, me.position.y);
}

void TrackList::mouseExit(const juce::MouseEvent & /*event*/)
{
    mouseOnComponent = false;
}

void TrackList::drawLabel(juce::Graphics &g, juce::Rectangle<int> bounds, std::string trackName,
                          juce::Colour trackColor, uint64_t trackIdentifier)
{
    auto reducedBounds = bounds.reduced(LABELS_INNER_PADDING);

    if (selectedTrack.has_value() && selectedTrack.value() != trackIdentifier)
    {
        g.setColour(trackColor.withAlpha(TRACK_NOT_SELECTED_ALPHA));
    }
    else
    {
        g.setColour(trackColor);
    }
    auto circleBounds = reducedBounds.removeFromLeft(reducedBounds.getHeight() + LABELS_CIRCLE_TEXT_PADDING);
    circleBounds.removeFromRight(LABELS_CIRCLE_TEXT_PADDING);
    circleBounds.reduce(CIRCLE_PADDING, CIRCLE_PADDING);
    g.fillEllipse(circleBounds.toFloat());

    if (selectedTrack.has_value() && selectedTrack.value() != trackIdentifier)
    {
        g.setColour(KHOLORS_COLOR_WHITE.withAlpha(TRACK_NOT_SELECTED_ALPHA));
    }
    else
    {
        g.setColour(KHOLORS_COLOR_WHITE);
    }

    g.drawEllipse(circleBounds.toFloat(), 1.0f);
    g.setFont(juce::Font(LABELS_FONT_HEIGHT));

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

    int const sideSeed = trackIdentifier % 2;

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

        int newPos = 0;

        if (allowedToSwitchChanSide)
        {
            int channelPosDiff = 0;
            float sidePosition = NAN;

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

            newPos = channelPosDiff + static_cast<int>(static_cast<float>(drawableHeightHalf) * sidePosition) -
                     (LABEL_HEIGHT / 2);
        }
        else
        {
            if (iter % 2 == 0)
            {
                newPos = static_cast<int>(static_cast<float>(drawableHeightHalf) * labelPositionRatioInDrawableArea) +
                         ((((int)iter / 2)) * LABEL_HEIGHT);
            }
            else
            {
                newPos = static_cast<int>(static_cast<float>(drawableHeightHalf) * labelPositionRatioInDrawableArea) -
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
    for (auto const i : rectangleList)
    {
        if (rect.intersects(i))
        {
            return true;
        }
    }
    return false;
}

void TrackList::setViewPosition(int64_t newVP)
{
    std::scoped_lock const lock(viewMutex);
    viewPosition = newVP;
}

void TrackList::setViewScale(int64_t newVS)
{
    std::scoped_lock const lock(viewMutex);
    viewScale = newVS;
}

void TrackList::setFreqViewWidth(int64_t newFVW)
{
    std::scoped_lock const lock(viewMutex);
    freqViewWidth = newFVW;
}

void TrackList::recordSfft(std::shared_ptr<NewFftDataTask> newSffts)
{
    size_t const fftNumFreqBins = newSffts->fftData->size() / newSffts->noFFTs;
    int64_t const fftSampleWidth =
        static_cast<int64_t>(newSffts->segmentSampleLength) / static_cast<int64_t>(newSffts->noFFTs);

    // if new, compute width of each FFT bin shown on screen given
    // the frequency transformation in order to correct
    // average frequency (right now bass are underrepresented)
    // and compute the bin position in frequency.

    if (lastUsedFftSize != fftNumFreqBins || std::cmp_not_equal(lastUsedSampleRate, newSffts->sampleRate) ||
        lastUsedTransformerNonce != frequencyTransformer.getNonce())
    {

        lastUsedFftSize = fftNumFreqBins;
        lastUsedSampleRate = newSffts->sampleRate;
        lastUsedTransformerNonce = frequencyTransformer.getNonce();

        freqWeight.resize(fftNumFreqBins);
        binFrequencies.resize(fftNumFreqBins);

        float const linearBinWidth = 1.0f / static_cast<float>(fftNumFreqBins);
        float const maxFreq = static_cast<float>(newSffts->sampleRate) / 2.0f;

        for (size_t j = 0; j < fftNumFreqBins; j++)
        {
            float const lowerBinBound = frequencyTransformer.transform(static_cast<float>(j) * linearBinWidth);
            float const upperBinBound = frequencyTransformer.transform(static_cast<float>(j + 1) * linearBinWidth);
            float const binWidth = upperBinBound - lowerBinBound;
            freqWeight[j] = binWidth;
            binFrequencies[j] = maxFreq * ((static_cast<float>(j) + 0.5f) / static_cast<float>(fftNumFreqBins));
        }

        sampleRateRatio = static_cast<float>(VISUAL_SAMPLE_RATE) / static_cast<float>(newSffts->sampleRate);
    }

    float *fftShift = newSffts->fftData->data();
    float *currentFftDataPos = nullptr;
    float *freqWeightPos = nullptr;
    float *binFreqsPtr = nullptr;

    int64_t startSampleFftShift = 0;

    // for each sfft
    for (size_t i = 0; i < newSffts->noFFTs; i++)
    {
        float avgIntensity = 0.0f;
        float avgFreq = 0.0f;

        currentFftDataPos = fftShift;
        freqWeightPos = freqWeight.data();
        binFreqsPtr = binFrequencies.data();

        for (size_t j = 0; j < fftNumFreqBins; j += 4)
        {
            // we normalize the db intensity between 0 and 1
            float const dbIntensity = *currentFftDataPos;
            float const intensityAtFreqBin = -MIN_DB + dbIntensity;
            float const weightedIntensity = intensityAtFreqBin * (*freqWeightPos);
            avgIntensity += weightedIntensity;
            avgFreq += (*binFreqsPtr) * weightedIntensity;

            currentFftDataPos += 4;
            freqWeightPos += 4;
            binFreqsPtr += 4;
        }
        // we moved this division of dbIntensity out of the loop to save perf
        avgIntensity = avgIntensity / (-MIN_DB);
        avgFreq = avgFreq / (-MIN_DB);

        avgFreq = avgFreq / avgIntensity;

        //   if below a thresold of volume, skip it
        if (avgIntensity < ACTIVE_TRACK_THRESOLD_PER_BIN)
        {
            continue;
        }
        // compute its position and tile index
        int64_t startSample = newSffts->segmentStartSample + startSampleFftShift;
        int64_t endSample = startSample + fftSampleWidth;
        if (newSffts->sampleRate != VISUAL_SAMPLE_RATE)
        {
            startSample = static_cast<float>(startSample) * sampleRateRatio;
            endSample = static_cast<float>(endSample) * sampleRateRatio;
        }
        int64_t const secondTileIndexStartSample = startSample / TILE_WIDTH;
        int64_t const secondTileIndexEndSample = endSample / TILE_WIDTH;
        // for each second tile it covers
        for (int64_t j = secondTileIndexStartSample; j <= secondTileIndexEndSample; j++)
        {
            if (j < 0)
            {
                continue;
            }
            std::scoped_lock const lock(tilesMutex);
            // if the tile does not exist, create it
            size_t const tileIndex = getOrCreateTile(j);
            //   if track not recorded in tile
            int64_t const trackIndexInTile = getTrackIndexInTileOrInsert(tileIndex, newSffts->trackIdentifier);
            // this is triggered if we reached the maximum number of displayable tiles
            if (trackIndexInTile < 0)
            {
                continue;
            }
            // add tile summed intensities to the channel(s)
            // add tile average freq to the channel(s)
            TracksPlaybackInfoTile &tile = tilesRingBuffer[tileIndex];
            if (newSffts->totalNoChannels == 1 || newSffts->channelIndex == 0)
            {
                tile.summedLeftChanIntensities[trackIndexInTile] += avgIntensity;
                tile.averageLeftChanFrequencies[trackIndexInTile] += avgFreq;
                tile.leftChanFftCount[trackIndexInTile] += 1;
            }

            if (newSffts->totalNoChannels == 1 || newSffts->channelIndex == 1)
            {
                tile.summedRightChanIntensities[trackIndexInTile] += avgIntensity;
                tile.averageRightChanFrequencies[trackIndexInTile] += avgFreq;
                tile.rightChanFftCount[trackIndexInTile] += 1;
            }
        }

        fftShift += fftNumFreqBins;
        startSampleFftShift += fftSampleWidth;
    }
}

size_t TrackList::getOrCreateTile(int64_t tilePosition)
{
    auto const existingTile = tileIndexesPerPosition.find(tilePosition);
    if (existingTile != tileIndexesPerPosition.end())
    {
        return existingTile->second;
    }

    size_t const newTileIndex = nextTileIndex;

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

    nextTileIndex++;
    if (nextTileIndex >= TILES_RING_BUFFER_SIZE)
    {
        nextTileIndex = 0;
    }

    return newTileIndex;
}

int64_t TrackList::getTrackIndexInTileOrInsert(size_t tileIndex, uint64_t trackIdentifier)
{
    // first step is to search for the track in the tile
    for (size_t i = 0; i < tilesRingBuffer[tileIndex].numSeenTracks; i++)
    {
        if (tilesRingBuffer[tileIndex].trackIdentifiers[i] == trackIdentifier)
        {
            return static_cast<int64_t>(i);
        }
    }
    // if we reach here, the track was not found
    // if there is no more space abort with -1
    if (tilesRingBuffer[tileIndex].numSeenTracks == MAX_TRACKS_PER_TILE)
    {
        return -1;
    }
    // otherwise we insert it
    size_t const newTrackIndex = tilesRingBuffer[tileIndex].numSeenTracks;
    tilesRingBuffer[tileIndex].trackIdentifiers[newTrackIndex] = trackIdentifier;
    tilesRingBuffer[tileIndex].summedLeftChanIntensities[newTrackIndex] = 0.0f;
    tilesRingBuffer[tileIndex].averageLeftChanFrequencies[newTrackIndex] = 0.0f;
    tilesRingBuffer[tileIndex].summedRightChanIntensities[newTrackIndex] = 0.0f;
    tilesRingBuffer[tileIndex].averageRightChanFrequencies[newTrackIndex] = 0.0f;
    tilesRingBuffer[tileIndex].leftChanFftCount[newTrackIndex] = 0;
    tilesRingBuffer[tileIndex].rightChanFftCount[newTrackIndex] = 0;
    tilesRingBuffer[tileIndex].numSeenTracks += 1;

    return static_cast<int64_t>(newTrackIndex);
}

void TrackList::clear()
{
    std::scoped_lock const lock(tilesMutex);
    for (auto &i : tilesRingBuffer)
    {
        i.numSeenTracks = 0;
    }
}

std::vector<TrackList::TrackPresenceSummary> TrackList::getVisibleTracks(int64_t startTile, int64_t endTile)
{
    std::scoped_lock const lock(tilesMutex);
    std::vector<TrackPresenceSummary> response;
    std::map<uint64_t, size_t> existingTracks;
    for (int64_t currentTile = startTile; currentTile <= endTile; currentTile++)
    {
        // if the tile exist, get its index, otherwise skip
        auto const existingTile = tileIndexesPerPosition.find(currentTile);
        if (existingTile == tileIndexesPerPosition.end())
        {
            continue;
        }
        TracksPlaybackInfoTile const &tile = tilesRingBuffer[existingTile->second];
        // for each track in the tile
        for (size_t i = 0; i < tile.numSeenTracks; i++)
        {
            // if not existing in response array yet, create an entry
            auto const existingTrackInTile = existingTracks.find(tile.trackIdentifiers[i]);
            size_t trackIndexInResponse = 0;
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
                trackSummary.leftChannelAvgFreq +=
                    (tile.averageLeftChanFrequencies[i] / static_cast<float>(tile.leftChanFftCount[i]));
            }
            if (tile.rightChanFftCount[i] != 0)
            {
                trackSummary.noFreqsAveragedRight++;
                trackSummary.rightChannelAvgFreq +=
                    (tile.averageRightChanFrequencies[i] / static_cast<float>(tile.leftChanFftCount[i]));
            }
        }
    }
    // for each track
    for (auto &i : response)
    {
        // set left channel average freq
        if (i.noFreqsAveragedLeft > 0)
        {
            i.leftChannelAvgFreq = i.leftChannelAvgFreq / i.noFreqsAveragedLeft;
        }
        else
        {
            i.leftChannelAvgFreq = VISUAL_SAMPLE_RATE / 2.0f;
        }

        // set right channel average freq
        if (i.noFreqsAveragedRight > 0)
        {
            i.rightChannelAvgFreq = i.rightChannelAvgFreq / i.noFreqsAveragedRight;
        }
        else
        {
            i.leftChannelAvgFreq = VISUAL_SAMPLE_RATE / 2.0f;
        }

        // decide on stereo balance
        float const balance = i.stereoDiff / i.allChannel;
        if (balance < (-BALANCE_SWITCH_THRESOLD))
        {
            i.stereoBalance = 0;
        }
        else if (balance > BALANCE_SWITCH_THRESOLD)
        {
            i.stereoBalance = 1;
        }
        else
        {
            i.stereoBalance = 2;
        }
    }

    std::ranges::sort(response, TrackList::compareTrackPresence);
    return response;
}

bool TrackList::compareTrackPresence(const TrackPresenceSummary &a, const TrackPresenceSummary &b)
{
    return a.trackIdentifier < b.trackIdentifier;
}

void TrackList::clearTrackFromRange(uint64_t trackIdientifier, int64_t startSample, uint64_t length)
{
    std::scoped_lock const lock(tilesMutex);

    // generate indices in TrackList tiles positions
    int64_t const startTile = (static_cast<float>(startSample) / static_cast<float>(TILE_WIDTH)) + 0.5f;
    int64_t const endTile =
        startTile + static_cast<int64_t>((static_cast<float>(length) / static_cast<float>(TILE_WIDTH)) + 0.5f) - 1;

    for (int64_t i = startTile; i <= endTile; i++)
    {
        // if a TrackList tile exist with that position index get it
        auto const existingTrackListTile = tileIndexesPerPosition.find(i);
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

            // otherwise we move last tile to this index and shrink array size (numSeenTracks)
            size_t const lastTileIndex = numSeenTracks - 1;
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