#include "DashboardView.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/BpmUpdateTask.h"
#include "StationApp/Audio/FftResultVectorReuseTask.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/ProcessingTimer.h"
#include "StationApp/Audio/TimeSignatureUpdateTask.h"
#include "StationApp/Audio/TrackColorUpdateTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/Audio/VolumeSensitivityTask.h"
#include "StationApp/GUI/AudioConstants.h"
#include "StationApp/GUI/ClearTask.h"
#include "StationApp/GUI/Graphs/FftOverTimeGraph.h"
#include "StationApp/GUI/Graphs/FrequencyScale.h"
#include "StationApp/GUI/Graphs/VolumeOverTimeGraph.h"
#include "StationApp/GUI/MouseCursorInfoTask.h"
#include "StationApp/GUI/TrackList.h"
#include "StationApp/GUI/TrackSelectionTask.h"
#include "StationApp/Maths/NormalizedBijectiveProjection.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

DashboardView::DashboardView(TrackInfoStore &tis, TaskingManager &tm)
    : taskingManager(tm), processingTimer(tm), trackInfoStore(tis),
      frequencyScale(frequencyTransformer, VISUAL_SAMPLE_RATE >> 1), trackList(trackInfoStore, frequencyTransformer, tm)
{
    lastFftMousePosX = 0;
    lastFftMousePosY = 0;
    lastCursorShowStatus = false;
    isViewMoving = false;

    setOpaque(true);

    freqOverTimeGraph = std::make_shared<FftOverTimeGraph>(trackInfoStore, frequencyTransformer, intensityTransformer);
    addAndMakeVisible(freqOverTimeGraph.get());

    volumeOverTimeGraph = std::make_shared<VolumeOverTimeGraph>(trackInfoStore);
    addAndMakeVisible(volumeOverTimeGraph.get());

    auto const freqProjection = std::make_shared<Log10Projection>(0.005);
    frequencyTransformer.setProjection(freqProjection);

    auto const intensityProjection = std::make_shared<SigmoidProjection>(6.0f);
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
    addAndMakeVisible(volumeScale);

    startTimer(VIEW_MOVE_TIME_INTERVAL_MS);
}

DashboardView::~DashboardView() = default;

void DashboardView::paint(juce::Graphics &g)
{
    g.setColour(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.fillRect(unpaintedArea1);
    g.fillRect(unpaintedArea2);
    g.fillRect(unpaintedArea3);
    g.fillRect(timeTicksAxisNameArea);

    // draw the bottom axis name
    g.setFont(juce::Font(AXIS_TITLE_PIXELS_HEIGHT));
    g.setColour(KHOLORS_COLOR_WHITE);
    g.drawText(TRANS("Project Time").toUpperCase(), timeTicksAxisNameArea.withTrimmedBottom(TITLE_PIXELS_FROM_BOTTOM),
               juce::Justification::centredBottom, false);
}

void DashboardView::paintOverChildren(juce::Graphics &g)
{
}

void DashboardView::updateGraphsViewPositions(int64_t newPosition)
{
    viewPosition = newPosition < 0 ? 0 : newPosition;
    freqOverTimeGraph->updateViewPosition(viewPosition);
    volumeOverTimeGraph->updateViewPosition(viewPosition);
    timeScale.setViewPosition(viewPosition);
    trackList.setViewPosition(viewPosition);
}

void DashboardView::updateGraphsViewScale(int64_t newScale)
{
    viewScale = newScale;
    freqOverTimeGraph->updateViewScale(viewScale);
    volumeOverTimeGraph->updateViewScale(viewScale);
    timeScale.setViewScale(viewScale);
    trackList.setViewScale(viewScale);
}

bool DashboardView::handleZoom(int dragY, int mouseX)
{
    int64_t const oldViewScale = viewScale;
    viewScale = juce::jlimit(
        MIN_SCALE_SAMPLE_PER_PIXEL, MAX_SCALE_SAMPLE_PER_PIXEL,
        static_cast<int>(static_cast<float>(viewScale) * (1.0f + (static_cast<float>(dragY) * PIXEL_SCALE_SPEED))));
    updateGraphsViewScale(viewScale);

    int64_t const oldCursorSamplePos = viewPosition + (mouseX * oldViewScale);
    int64_t const newCursorSamplePos = viewPosition + (mouseX * viewScale);
    int const sampleShiftToAlignZoomToCursor = oldCursorSamplePos - newCursorSamplePos;

    viewPosition += sampleShiftToAlignZoomToCursor;
    viewPosition = std::max<int64_t>(viewPosition, 0);
    updateGraphsViewPositions(viewPosition);
    return true;
}

bool DashboardView::handlePan(int dragX)
{
    int64_t const samplesDiff = dragX * viewScale;
    viewPosition -= samplesDiff;
    viewPosition = std::max<int64_t>(viewPosition, 0);
    updateGraphsViewPositions(viewPosition);
    return true;
}

void DashboardView::updateViewMouseDrag(const juce::MouseEvent &e)
{
    std::scoped_lock const lock(viewMutex);

    int const dragX = e.x - lastMouseDragX;
    int const dragY = e.y - lastMouseDragY;

    lastMouseDragX = e.x;
    lastMouseDragY = e.y;

    bool needRepaint = false;

    if (dragY != 0)
    {
        needRepaint = handleZoom(dragY, e.x);
    }

    if (dragX != 0)
    {
        needRepaint = handlePan(dragX) || needRepaint;
    }

    if (needRepaint)
    {
        freqOverTimeGraph->repaint();
        volumeOverTimeGraph->repaint();
        timeScale.repaint();
    }
}

void DashboardView::updateAutoscroll(int64_t currentTime, int64_t elapsedSinceLastCallMs, int64_t lastFftDrawTimeMsCopy)
{
    if (!isViewMoving && (currentTime - lastFftDrawTimeMsCopy) < MAX_TIME_SINCE_FFT_UPDATE_TO_CENTER_VIEW_MS)
    {
        int64_t const lastPlayCursorPos = freqOverTimeGraph->getPlayCursorPosition();
        std::scoped_lock const lock(viewMutex);
        int64_t const leftScreenSideSamplePos = viewPosition;
        int64_t const rightScreenSideSamplePos = viewPosition + (freqOverTimeGraph->getBounds().getWidth() * viewScale);
        int64_t const screenQuarter = (rightScreenSideSamplePos - leftScreenSideSamplePos) / 4;

        if (lastPlayCursorPos < leftScreenSideSamplePos || lastPlayCursorPos > rightScreenSideSamplePos)
        {
            updateGraphsViewPositions(lastPlayCursorPos - (3 * screenQuarter));
        }
        else if (lastPlayCursorPos >= (rightScreenSideSamplePos - screenQuarter + 1) &&
                 lastPlayCursorPos < (rightScreenSideSamplePos - (screenQuarter >> 1)))
        {
            auto const increment = static_cast<int64_t>(std::llround(
                (static_cast<float>(elapsedSinceLastCallMs) / 1000.0) * static_cast<float>(VISUAL_SAMPLE_RATE)));
            updateGraphsViewPositions(viewPosition + increment);
        }
    }
}

void DashboardView::propagateClearedFft()
{
    auto const tracksClearedInMainView = freqOverTimeGraph->getClearedTrackRanges();
    for (auto const &i : tracksClearedInMainView)
    {
        trackList.clearTrackFromRange(i.trackIdentifier, i.startSample, i.length);
        volumeOverTimeGraph->clearTracksFromRange(i.startSample, i.length);
    }
}

bool DashboardView::handleNewFftDataTask(std::shared_ptr<NewFftDataTask> task)
{
    if (!task->skip)
    {
        auto const processingTimeWaitgroup = processingTimer.getNewProcessingTimerWaitgroup(task->sentTimeUnixMs);
        if (processingTimeWaitgroup != nullptr)
        {
            processingTimeWaitgroup->add();
            int64_t const currentTime = juce::Time().getCurrentTime().toMilliseconds();
            int64_t lastFftDrawTimeMsCopy = 0;
            {
                std::scoped_lock const lock(lastFftDrawTimeMutex);
                lastFftDrawTimeMsCopy = lastFftDrawTimeMs;
            }
            if ((currentTime - lastFftDrawTimeMsCopy) > MAX_IDLE_MS_TIME_BEFORE_CLEAR)
            {
                freqOverTimeGraph->clear();
                trackList.clear();
                volumeOverTimeGraph->clear();
            }
            freqOverTimeGraph->displayNewFftData(task, processingTimeWaitgroup);
            int64_t const playHeadPosition =
                static_cast<int64_t>(task->segmentStartSample) + static_cast<int64_t>(task->segmentSampleLength);
            freqOverTimeGraph->submitNewPlayCursorPosition(playHeadPosition, task->sampleRate);
            volumeOverTimeGraph->submitNewPlayCursorPosition(playHeadPosition, task->sampleRate);
            trackList.recordSfft(task);
            {
                std::scoped_lock const lock(lastFftDrawTimeMutex);
                lastFftDrawTimeMs = currentTime;
            }
            processingTimeWaitgroup->recordCompletion();
        }
        else
        {
            spdlog::warn("A FFT drawing was skipped because too much FFTs are pending drawing.");
        }
    }
    else
    {
        processingTimer.recordCompletion(-1, juce::Time::currentTimeMillis() - task->sentTimeUnixMs);
    }

    // this is replacing the array with the fft inside the memory pool to avoid reallocating at every FFT
    auto const reuseResultArrayTask = std::make_shared<FftResultVectorReuseTask>(task->fftData);
    taskingManager.broadcastNestedTaskNow(reuseResultArrayTask);

    task->setCompleted(true);
    return false;
}

bool DashboardView::handleNewVolumeDataTask(std::shared_ptr<NewTrackVolumeDataTask> task)
{
    volumeOverTimeGraph->displayNewVolumeData(task);
    task->setCompleted(true);
    return false;
}

bool DashboardView::handleTrackColorUpdateTask(std::shared_ptr<TrackColorUpdateTask> task)
{
    juce::Colour const col(task->redColorLevel, task->greenColorLevel, task->blueColorLevel);
    freqOverTimeGraph->setTrackColor(task->identifier, col);
    volumeOverTimeGraph->setTrackColor(task->identifier, col);
    // NOTE: the track list directly takes colours from the trackInfoStore,
    // so we do not propagate to it.
    task->setCompleted(true);
    return true;
}

bool DashboardView::handleBpmUpdateTask(std::shared_ptr<BpmUpdateTask> task)
{
    if (std::abs(lastReceivedBpm - task->bpm) >= std::numeric_limits<float>::epsilon())
    {
        freqOverTimeGraph->updateBpm(task->bpm, task->getTaskingManager());
        volumeOverTimeGraph->updateBpm(task->bpm, task->getTaskingManager());
        timeScale.setBpm(task->bpm);
        lastReceivedBpm = task->bpm;
    }
    task->setCompleted(true);
    return false;
}

bool DashboardView::handleTimeSignatureUpdateTask(std::shared_ptr<TimeSignatureUpdateTask> task)
{
    freqOverTimeGraph->timeSignatureNumeratorUpdate(task->numerator);
    volumeOverTimeGraph->timeSignatureNumeratorUpdate(task->numerator);
    task->setCompleted(true);
    return false;
}

bool DashboardView::handleTrackSelectionTask(std::shared_ptr<TrackSelectionTask> task)
{
    freqOverTimeGraph->setSelectedTrack(task->selectedTrack, task->getTaskingManager());
    volumeOverTimeGraph->setSelectedTrack(task->selectedTrack, task->getTaskingManager());
    task->setCompleted(true);
    return false;
}

bool DashboardView::handleClearTask(std::shared_ptr<ClearTask> task)
{
    freqOverTimeGraph->clear();
    volumeOverTimeGraph->clear();
    trackList.clear();
    task->setCompleted(true);
    return false;
}

bool DashboardView::handleVolumeSensitivityTask(std::shared_ptr<VolumeSensitivityTask> task)
{
    auto const intensityProjection = std::make_shared<SigmoidProjection>(task->sensitivity);
    intensityTransformer.setProjection(intensityProjection);
    task->setCompleted(true);
    return false;
}

void DashboardView::resized()
{
    auto fftBounds = getLocalBounds();
    auto const trackListBounds = fftBounds.removeFromRight(TRACK_LIST_WIDTH).withTrimmedBottom(TIME_GRID_HEIGHT);
    auto scalesArea = fftBounds.removeFromLeft(FREQUENCY_GRID_WIDTH).withTrimmedBottom(TIME_GRID_HEIGHT);
    auto timeGridBounds = fftBounds.removeFromBottom(TIME_GRID_HEIGHT);
    auto const volumeBounds = fftBounds.removeFromBottom(VOLUME_GRAPH_HEIGHT);
    unpaintedArea3 = fftBounds.removeFromBottom(TIME_GRAPHS_PADDING);

    auto const volumeGridBounds = scalesArea.removeFromBottom(VOLUME_GRAPH_HEIGHT);
    auto const frequencyGridBounds = scalesArea.withTrimmedBottom(TIME_GRAPHS_PADDING);

    freqOverTimeGraph->setBounds(fftBounds);
    volumeOverTimeGraph->setBounds(volumeBounds);
    frequencyScale.setBounds(frequencyGridBounds);
    volumeScale.setBounds(volumeGridBounds);
    timeScale.setBounds(timeGridBounds.removeFromTop(KHOLORS_DEFAULT_FONT_SIZE));
    trackList.setBounds(trackListBounds);

    timeTicksAxisNameArea = timeGridBounds;

    trackList.setFreqViewWidth(fftBounds.getWidth());

    std::scoped_lock const lock(viewMutex);
    freqOverTimeGraph->updateViewPosition(viewPosition);
    volumeOverTimeGraph->updateViewPosition(viewPosition);

    unpaintedArea1 = frequencyGridBounds.withY(frequencyGridBounds.getY() + frequencyGridBounds.getHeight());
    unpaintedArea1.setHeight(getLocalBounds().getHeight() - frequencyGridBounds.getHeight());

    unpaintedArea2 = trackListBounds.withY(trackListBounds.getY() + trackListBounds.getHeight());
    unpaintedArea2.setHeight(getLocalBounds().getHeight() - trackListBounds.getHeight());
}

void DashboardView::timerCallback()
{
    // get the time since the timer was last called
    int64_t const currentTime = juce::Time().getCurrentTime().toMilliseconds();
    int64_t const elapsedSinceLastCallMs = currentTime - lastTimerCallMs;

    // get the time since the fft were last drawn
    int64_t lastFftDrawTimeMsCopy = 0;
    {
        std::scoped_lock const lock(lastFftDrawTimeMutex);
        lastFftDrawTimeMsCopy = lastFftDrawTimeMs;
    }

    // repaint track list if it was not repainted recently
    if ((currentTime - trackList.getLastRedrawMs()) > MAX_TIME_WITHOUT_TRACK_LIST_PAINT_MS)
    {
        trackList.repaint();
    }

    // move the view position based on the play cursor position
    updateAutoscroll(currentTime, elapsedSinceLastCallMs, lastFftDrawTimeMsCopy);

    // checks the list if FFTs that were cleared from the main graph, and propagate to
    // other components that are storing them (so deletions are in sync)
    propagateClearedFft();

    lastTimerCallMs = currentTime;

    freqOverTimeGraph->repaint();
    volumeOverTimeGraph->repaint();
    timeScale.repaint();
}

bool DashboardView::taskHandler(std::shared_ptr<Task> task)
{
    if (auto const newFftDataTask = std::dynamic_pointer_cast<NewFftDataTask>(task))
    {
        if (!newFftDataTask->isCompleted() && !newFftDataTask->hasFailed())
        {
            return handleNewFftDataTask(newFftDataTask);
        }
    }

    if (auto const newTrackVolumeDataTask = std::dynamic_pointer_cast<NewTrackVolumeDataTask>(task))
    {
        if (!newTrackVolumeDataTask->isCompleted() && !newTrackVolumeDataTask->hasFailed())
        {
            return handleNewVolumeDataTask(newTrackVolumeDataTask);
        }
    }

    if (auto const colorUpdateTask = std::dynamic_pointer_cast<TrackColorUpdateTask>(task))
    {
        return handleTrackColorUpdateTask(colorUpdateTask);
    }

    if (auto const bpmUpdateTask = std::dynamic_pointer_cast<BpmUpdateTask>(task))
    {
        if (!bpmUpdateTask->isCompleted())
        {
            return handleBpmUpdateTask(bpmUpdateTask);
        }
    }

    if (auto const timeSignatureUpdate = std::dynamic_pointer_cast<TimeSignatureUpdateTask>(task))
    {
        if (!timeSignatureUpdate->isCompleted())
        {
            return handleTimeSignatureUpdateTask(timeSignatureUpdate);
        }
    }

    if (auto const selectionUpdate = std::dynamic_pointer_cast<TrackSelectionTask>(task))
    {
        if (!selectionUpdate->isCompleted())
        {
            return handleTrackSelectionTask(selectionUpdate);
        }
    }

    if (auto const clearTask = std::dynamic_pointer_cast<ClearTask>(task))
    {
        if (!clearTask->isCompleted())
        {
            return handleClearTask(clearTask);
        }
    }

    if (auto const volumeSensitivityUpdateTask = std::dynamic_pointer_cast<VolumeSensitivityTask>(task))
    {
        if (!volumeSensitivityUpdateTask->isCompleted())
        {
            return handleVolumeSensitivityTask(volumeSensitivityUpdateTask);
        }
    }

    return false;
}

void DashboardView::mouseDown(const juce::MouseEvent &e)
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

void DashboardView::mouseUp(const juce::MouseEvent &e)
{
    if (e.mods.isMiddleButtonDown())
    {
        isViewMoving = false;
    }
}

void DashboardView::mouseDrag(const juce::MouseEvent &e)
{
    broadcastMouseEventInfo(e);

    if (e.mods.isMiddleButtonDown())
    {
        updateViewMouseDrag(e);
    }
}

void DashboardView::mouseMove(const juce::MouseEvent &me)
{
    broadcastMouseEventInfo(me);
}

void DashboardView::broadcastMouseEventInfo(const juce::MouseEvent &me)
{
    auto const positionRelativeToDashboardView = me.getEventRelativeTo(freqOverTimeGraph.get());
    bool const showCursor = freqOverTimeGraph->getBounds().contains(me.getPosition());
    freqOverTimeGraph->setMouseCursor(showCursor, positionRelativeToDashboardView.position.x,
                                      positionRelativeToDashboardView.position.y);

    emitMousePositionInfoTask(showCursor, positionRelativeToDashboardView.x, positionRelativeToDashboardView.y);

    freqOverTimeGraph->repaint();
}

void DashboardView::emitMousePositionInfoTask(bool mouseOverFreqTimeGraph, int x, int y)
{
    lastFftMousePosX = x;
    lastFftMousePosY = y;
    lastCursorShowStatus = mouseOverFreqTimeGraph;

    float positionInFreq = 0.0f;
    int64_t timeInSample = 0;

    if (mouseOverFreqTimeGraph)
    {
        float positionInChannel = 0.0f;
        float const halfHeight = 0.5f * static_cast<float>(freqOverTimeGraph->getHeight());
        bool const isOnTopChannel = y < halfHeight;
        if (isOnTopChannel)
        {
            positionInChannel = 1.0f - (static_cast<float>(y) / halfHeight);
        }
        else
        {
            positionInChannel = (static_cast<float>(y) - halfHeight) / halfHeight;
        }
        positionInChannel = juce::jlimit(0.0f, 1.0f, positionInChannel);
        positionInFreq = frequencyTransformer.transformInv(positionInChannel);
    }

    timeInSample = viewPosition + (viewScale * x);

    auto const cursorUpdateTask = std::make_shared<MouseCursorInfoTask>(
        mouseOverFreqTimeGraph, positionInFreq * 0.5f * static_cast<float>(VISUAL_SAMPLE_RATE), timeInSample);
    taskingManager.broadcastTask(cursorUpdateTask);
}

void DashboardView::mouseExit(const juce::MouseEvent & /*event*/)
{
    freqOverTimeGraph->setMouseCursor(false, -1, -1);
    auto const cursorUpdateTask = std::make_shared<MouseCursorInfoTask>(false, 0, 0);
    taskingManager.broadcastTask(cursorUpdateTask);
    freqOverTimeGraph->repaint();
}