#include "FreqTimeView.h"
#include "StationApp/Audio/BpmUpdateTask.h"
#include "StationApp/Audio/FftResultVectorReuseTask.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/ProcessingTimer.h"
#include "StationApp/Audio/TimeSignatureUpdateTask.h"
#include "StationApp/Audio/TrackColorUpdateTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/Audio/VolumeSensitivityTask.h"
#include "StationApp/GUI/ClearTask.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/FrequencyScale.h"
#include "StationApp/GUI/GpuTextureDrawingBackend.h"
#include "StationApp/GUI/MouseCursorInfoTask.h"
#include "StationApp/GUI/TrackList.h"
#include "StationApp/GUI/TrackSelectionTask.h"
#include "StationApp/Maths/NormalizedBijectiveProjection.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstddef>
#include <ctime>
#include <limits>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

FreqTimeView::FreqTimeView(TrackInfoStore &tis, TaskingManager &tm)
    : taskingManager(tm), processingTimer(tm), trackInfoStore(tis), viewPosition(0), viewScale(150),
      frequencyScale(frequencyTransformer, VISUAL_SAMPLE_RATE >> 1), trackList(trackInfoStore, frequencyTransformer, tm)
{
    lastFftMousePosX = 0;
    lastFftMousePosY = 0;
    lastCursorShowStatus = false;
    isViewMoving = false;

    setOpaque(true);

    fftDrawBackend =
        std::make_shared<GpuTextureDrawingBackend>(trackInfoStore, frequencyTransformer, intensityTransformer);
    // fftDrawBackend = std::make_shared<CpuImageDrawingBackend>(trackInfoStore);
    addAndMakeVisible(fftDrawBackend.get());

    auto freqProjection = std::make_shared<Log10Projection>(0.005);
    frequencyTransformer.setProjection(freqProjection);

    auto intensityProjection = std::make_shared<SigmoidProjection>(6.0f);
    intensityTransformer.setProjection(intensityProjection);

    lastTimerCallMs = juce::Time().getCurrentTime().toMilliseconds();

    timeScale.setBpm(120);
    timeScale.setViewScale(viewScale);
    timeScale.setViewPosition(viewScale);

    trackList.setViewPosition(viewPosition);
    trackList.setViewScale(viewScale);

    addAndMakeVisible(frequencyScale);
    addAndMakeVisible(timeScale);
    addAndMakeVisible(trackList);

    startTimer(VIEW_MOVE_TIME_INTERVAL_MS);
}

FreqTimeView::~FreqTimeView()
{
}

void FreqTimeView::paint(juce::Graphics &g)
{
    g.setColour(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.fillRect(unpaintedArea1);
    g.fillRect(unpaintedArea2);
}

void FreqTimeView::paintOverChildren(juce::Graphics &g)
{
}

void FreqTimeView::resized()
{
    auto fftBounds = getLocalBounds();
    auto trackListBounds = fftBounds.removeFromRight(TRACK_LIST_WIDTH).withTrimmedBottom(TIME_GRID_HEIGHT);
    auto frequencyGridBounds = fftBounds.removeFromLeft(FREQUENCY_GRID_WIDTH).withTrimmedBottom(TIME_GRID_HEIGHT);
    auto timeGridBounds = fftBounds.removeFromBottom(TIME_GRID_HEIGHT);
    fftDrawBackend->setBounds(fftBounds);
    frequencyScale.setBounds(frequencyGridBounds);
    timeScale.setBounds(timeGridBounds);
    trackList.setBounds(trackListBounds);

    trackList.setFreqViewWidth(fftBounds.getWidth());

    std::lock_guard lock(viewMutex);
    fftDrawBackend->updateViewPosition(viewPosition);

    unpaintedArea1 = frequencyGridBounds.withY(frequencyGridBounds.getY() + frequencyGridBounds.getHeight());
    unpaintedArea1.setHeight(getLocalBounds().getHeight() - frequencyGridBounds.getHeight());

    unpaintedArea2 = trackListBounds.withY(trackListBounds.getY() + trackListBounds.getHeight());
    unpaintedArea2.setHeight(getLocalBounds().getHeight() - trackListBounds.getHeight());
}

void FreqTimeView::timerCallback()
{
    int64_t currentTime = juce::Time().getCurrentTime().toMilliseconds();
    int64_t elapsedSinceLastCallMs = currentTime - lastTimerCallMs;
    int64_t lastFftDrawTimeMsCopy;
    {
        std::lock_guard lock(lastFftDrawTimeMutex);
        lastFftDrawTimeMsCopy = lastFftDrawTimeMs;
    }

    // if last FFT was recent but last redraw of track names was not
    if ((currentTime - trackList.getLastRedrawMs()) > MAX_TIME_WITHOUT_TRACK_LIST_PAINT_MS)
    {
        trackList.repaint();
    }

    // if user is not currently moving the view and that last fft was received recently enough
    if (!isViewMoving && (currentTime - lastFftDrawTimeMsCopy) < MAX_TIME_SINCE_FFT_UPDATE_TO_CENTER_VIEW_MS)
    {
        // first check that play cursor is in bounds
        int64_t lastPlayCursorPos = fftDrawBackend->getPlayCursorPosition();
        std::lock_guard lock(viewMutex);
        int64_t leftScreenSideSamplePos = viewPosition;
        int64_t rightScreenSideSamplePos = viewPosition + (fftDrawBackend->getBounds().getWidth() * viewScale);
        int64_t screenQuarter = (rightScreenSideSamplePos - leftScreenSideSamplePos) / 4;
        // if play cursor is outside the view, reset view position where play cursor is at 3/4
        if (lastPlayCursorPos < leftScreenSideSamplePos || lastPlayCursorPos > rightScreenSideSamplePos)
        {
            viewPosition = lastPlayCursorPos - (3 * screenQuarter);
            if (viewPosition < 0)
            {
                viewPosition = 0;
            }
            fftDrawBackend->updateViewPosition(viewPosition);
            timeScale.setViewPosition(viewPosition);
            trackList.setViewPosition(viewPosition);
            // if we show the cursor, we should update its value shown on screen in the tips
            if (lastCursorShowStatus)
            {
                emitMousePositionInfoTask(true, lastFftMousePosX, lastFftMousePosY);
            }
        }
        // if play cursor is between 3/4 of view and right side, apply constant view moving speed
        else if (lastPlayCursorPos >= (rightScreenSideSamplePos - screenQuarter + 1) &&
                 lastPlayCursorPos < (rightScreenSideSamplePos - (screenQuarter >> 1)))
        {
            viewPosition += (int64_t)((float(elapsedSinceLastCallMs) / 1000.0) * float(VISUAL_SAMPLE_RATE) + 0.5f);
            if (viewPosition < 0)
            {
                viewPosition = 0;
            }
            fftDrawBackend->updateViewPosition(viewPosition);
            timeScale.setViewPosition(viewPosition);
            trackList.setViewPosition(viewPosition);
            // if we show the cursor, we should update its value shown on screen in the tips
            if (lastCursorShowStatus)
            {
                emitMousePositionInfoTask(true, lastFftMousePosX, lastFftMousePosY);
            }
        }
    }
    // if some tracks have been cleared from some ranges on fftDrawing backend, replicate that
    // on other related components
    auto tracksClearedInMainView = fftDrawBackend->getClearedTrackRanges();
    for (size_t i = 0; i < tracksClearedInMainView.size(); i++)
    {
        trackList.clearTrackFromRange(tracksClearedInMainView[i].trackIdentifier,
                                      tracksClearedInMainView[i].startSample, tracksClearedInMainView[i].length);
    }
    lastTimerCallMs = currentTime;

    fftDrawBackend->repaint();
    timeScale.repaint();
}

bool FreqTimeView::taskHandler(std::shared_ptr<Task> task)
{
    auto newFftDataTask = std::dynamic_pointer_cast<NewFftDataTask>(task);
    if (newFftDataTask != nullptr && !newFftDataTask->isCompleted() && !newFftDataTask->hasFailed())
    {

        // call on the drawing backend to add the FFT data to the displayed textures
        if (!newFftDataTask->skip)
        {
            // create a segment processing time waitgroup and pass it to fft drawing backend
            auto processingTimeWaitgroup =
                processingTimer.getNewProcessingTimerWaitgroup(newFftDataTask->sentTimeUnixMs);

            // Only proceed if we have been able to get a preallocated processing timer.
            // If not that means we are on overload.
            if (processingTimeWaitgroup != nullptr)
            {
                processingTimeWaitgroup->add();

                // if time since last drawing was too large, clear all past data
                int64_t currentTime = juce::Time().getCurrentTime().toMilliseconds();

                int64_t lastFftDrawTimeMsCopy = 0;
                {
                    std::lock_guard lock(lastFftDrawTimeMutex);
                    lastFftDrawTimeMsCopy = lastFftDrawTimeMs;
                }

                if ((currentTime - lastFftDrawTimeMsCopy) > MAX_IDLE_MS_TIME_BEFORE_CLEAR)
                {
                    fftDrawBackend->clearDisplayedFFTs();
                    trackList.clear();
                }

                // send fft data to drawing backend and update play cursor
                fftDrawBackend->displayNewFftData(newFftDataTask, processingTimeWaitgroup);
                fftDrawBackend->submitNewPlayCursorPosition((int64_t)newFftDataTask->segmentStartSample +
                                                                (int64_t)newFftDataTask->segmentSampleLength,
                                                            newFftDataTask->sampleRate);

                // record which track is playing and where to display labels
                trackList.recordSfft(newFftDataTask);

                // record last drawing time
                {
                    std::lock_guard lock(lastFftDrawTimeMutex);
                    lastFftDrawTimeMs = currentTime;
                }

                // complete first half of segment processing time waitgroup here
                processingTimeWaitgroup->recordCompletion();
            }
            else
            {
                spdlog::warn("A FFT drawing was skipped because too much FFTs are pending drawing.");
            }
        }
        else
        {
            // if we skip displaying this fft, we still report the processing time
            processingTimer.recordCompletion(-1, juce::Time::currentTimeMillis() - newFftDataTask->sentTimeUnixMs);
        }

        auto reuseResultArrayTask = std::make_shared<FftResultVectorReuseTask>(newFftDataTask->fftData);
        taskingManager.broadcastNestedTaskNow(reuseResultArrayTask);

        newFftDataTask->setCompleted(true);
        return false;
    }

    auto colorUpdateTask = std::dynamic_pointer_cast<TrackColorUpdateTask>(task);
    if (colorUpdateTask != nullptr)
    {
        juce::Colour col(colorUpdateTask->redColorLevel, colorUpdateTask->greenColorLevel,
                         colorUpdateTask->blueColorLevel);
        fftDrawBackend->setTrackColor(colorUpdateTask->identifier, col);
        colorUpdateTask->setCompleted(true);
        return true;
    }

    auto bpmUpdateTask = std::dynamic_pointer_cast<BpmUpdateTask>(task);
    if (bpmUpdateTask != nullptr && !bpmUpdateTask->isCompleted())
    {
        if (std::abs(lastReceivedBpm - bpmUpdateTask->bpm) >= std::numeric_limits<float>::epsilon())
        {
            fftDrawBackend->updateBpm(bpmUpdateTask->bpm, bpmUpdateTask->getTaskingManager());
            timeScale.setBpm(bpmUpdateTask->bpm);
            lastReceivedBpm = bpmUpdateTask->bpm;
        }
        bpmUpdateTask->setCompleted(true);
        return false;
    }

    auto timeSignatureUpdate = std::dynamic_pointer_cast<TimeSignatureUpdateTask>(task);
    if (timeSignatureUpdate != nullptr && !timeSignatureUpdate->isCompleted())
    {
        fftDrawBackend->timeSignatureNumeratorUpdate(timeSignatureUpdate->numerator);
        timeSignatureUpdate->setCompleted(true);
        return false;
    }

    auto selectionUpdate = std::dynamic_pointer_cast<TrackSelectionTask>(task);
    if (selectionUpdate != nullptr && !selectionUpdate->isCompleted())
    {
        fftDrawBackend->setSelectedTrack(selectionUpdate->selectedTrack, selectionUpdate->getTaskingManager());
        selectionUpdate->setCompleted(true);
        return false;
    }

    auto clearTask = std::dynamic_pointer_cast<ClearTask>(task);
    if (clearTask != nullptr && !clearTask->isCompleted())
    {
        fftDrawBackend->clearDisplayedFFTs();
        trackList.clear();
        clearTask->setCompleted(true);
        return false;
    };

    auto volumeSensitivityUpdateTask = std::dynamic_pointer_cast<VolumeSensitivityTask>(task);
    if (volumeSensitivityUpdateTask != nullptr && !volumeSensitivityUpdateTask->isCompleted())
    {
        auto intensityProjection = std::make_shared<SigmoidProjection>(volumeSensitivityUpdateTask->sensitivity);
        intensityTransformer.setProjection(intensityProjection);
        volumeSensitivityUpdateTask->setCompleted(true);
        return false;
    }

    // we are not stopping any tasks from being broadcasted further
    return false;
}

void FreqTimeView::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isAnyMouseButtonDown())
    {
        lastMouseDragX = e.getMouseDownPosition().getX();
        lastMouseDragY = e.getMouseDownPosition().getY();
    }
    if (e.mods.isMiddleButtonDown())
    {
        isViewMoving = true;
    }
}

void FreqTimeView::mouseUp(const juce::MouseEvent &e)
{
    if (e.mods.isMiddleButtonDown())
    {
        isViewMoving = false;
    }
}

void FreqTimeView::mouseDrag(const juce::MouseEvent &e)
{
    broadcastMouseEventInfo(e);

    if (e.mods.isMiddleButtonDown())
    {
        std::lock_guard lock(viewMutex);

        int dragX = e.x - lastMouseDragX;
        int dragY = e.y - lastMouseDragY;

        lastMouseDragX = e.x;
        lastMouseDragY = e.y;

        bool needRepaint = false;

        if (dragY != 0)
        {
            int64_t oldViewScale = viewScale;

            viewScale = juce::jlimit(MIN_SCALE_SAMPLE_PER_PIXEL, MAX_SCALE_SAMPLE_PER_PIXEL,
                                     int(float(viewScale) * (1.0f + (float(dragY) * PIXEL_SCALE_SPEED))));
            fftDrawBackend->updateViewScale(viewScale);
            timeScale.setViewScale(viewScale);
            trackList.setViewScale(viewScale);

            // we compute view position shift to maintain the same point under cursor after zooming
            int64_t oldCursorSamplePos = viewPosition + (e.x * oldViewScale);
            int64_t newCursorSamplePos = viewPosition + (e.x * viewScale);
            int sampleShiftToAlignZoomToCursor = oldCursorSamplePos - newCursorSamplePos;

            viewPosition += sampleShiftToAlignZoomToCursor;
            if (viewPosition < 0)
            {
                viewPosition = 0;
            }
            fftDrawBackend->updateViewPosition(viewPosition);
            timeScale.setViewPosition(viewPosition);
            trackList.setViewPosition(viewPosition);

            needRepaint = true;
        }

        if (dragX != 0)
        {
            int64_t samplesDiff = dragX * viewScale;
            viewPosition -= samplesDiff;
            if (viewPosition < 0)
            {
                viewPosition = 0;
            }
            fftDrawBackend->updateViewPosition(viewPosition);
            timeScale.setViewPosition(viewPosition);

            needRepaint = true;
        }

        if (needRepaint)
        {
            fftDrawBackend->repaint();
            timeScale.repaint();
        }
    }
}

void FreqTimeView::mouseMove(const juce::MouseEvent &me)
{
    broadcastMouseEventInfo(me);
}

void FreqTimeView::broadcastMouseEventInfo(const juce::MouseEvent &me)
{
    auto positionRelativeToFreqTimeView = me.getEventRelativeTo(fftDrawBackend.get());
    bool showCursor = fftDrawBackend->getBounds().contains(me.getPosition());
    fftDrawBackend->setMouseCursor(showCursor, positionRelativeToFreqTimeView.position.x,
                                   positionRelativeToFreqTimeView.position.y);

    emitMousePositionInfoTask(showCursor, positionRelativeToFreqTimeView.x, positionRelativeToFreqTimeView.y);

    fftDrawBackend->repaint();
}

void FreqTimeView::emitMousePositionInfoTask(bool shouldShow, int x, int y)
{
    lastFftMousePosX = x;
    lastFftMousePosY = y;
    lastCursorShowStatus = shouldShow;

    float positionInFreq = 0.0f;
    int64_t timeInSample = 0;

    if (shouldShow)
    {
        float positionInChannel = 0.0f;
        float halfHeight = 0.5f * float(fftDrawBackend->getHeight());
        bool isOnTopChannel = y < halfHeight;
        if (isOnTopChannel)
        {
            positionInChannel = 1.0f - (float(y) / halfHeight);
        }
        else
        {
            positionInChannel = (float(y) - halfHeight) / halfHeight;
        }
        positionInChannel = juce::jlimit(0.0f, 1.0f, positionInChannel);
        positionInFreq = frequencyTransformer.transformInv(positionInChannel);
    }

    timeInSample = viewPosition + (viewScale * x);

    auto cursorUpdateTask = std::make_shared<MouseCursorInfoTask>(
        shouldShow, positionInFreq * 0.5f * float(VISUAL_SAMPLE_RATE), timeInSample);
    taskingManager.broadcastTask(cursorUpdateTask);
}

void FreqTimeView::mouseExit(const juce::MouseEvent &)
{
    fftDrawBackend->setMouseCursor(false, -1, -1);
    auto cursorUpdateTask = std::make_shared<MouseCursorInfoTask>(false, 0, 0);
    taskingManager.broadcastTask(cursorUpdateTask);
    fftDrawBackend->repaint();
}