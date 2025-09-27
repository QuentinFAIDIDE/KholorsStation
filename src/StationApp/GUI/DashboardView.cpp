#include "DashboardView.h"
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
#include "StationApp/GUI/Graphs/FreqOverTimeGraph.h"
#include "StationApp/GUI/Graphs/FrequencyScale.h"
#include "StationApp/GUI/Graphs/VolumeOverTimeGraph.h"
#include "StationApp/GUI/MouseCursorInfoTask.h"
#include "StationApp/GUI/TrackList.h"
#include "StationApp/GUI/TrackSelectionTask.h"
#include "StationApp/Maths/NormalizedBijectiveProjection.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

DashboardView::DashboardView(TrackInfoStore &tis, TaskingManager &tm)
    : taskingManager(tm), processingTimer(tm), trackInfoStore(tis), viewPosition(0), viewScale(150),
      frequencyScale(frequencyTransformer, VISUAL_SAMPLE_RATE >> 1), trackList(trackInfoStore, frequencyTransformer, tm)
{
    lastFftMousePosX = 0;
    lastFftMousePosY = 0;
    lastCursorShowStatus = false;
    isViewMoving = false;

    setOpaque(true);

    freqOverTimeGraph = std::make_shared<FreqOverTimeGraph>(trackInfoStore, frequencyTransformer, intensityTransformer);
    addAndMakeVisible(freqOverTimeGraph.get());

    volumeOverTimeGraph = std::make_shared<VolumeOverTimeGraph>(trackInfoStore);
    addAndMakeVisible(volumeOverTimeGraph.get());

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

DashboardView::~DashboardView()
{
}

void DashboardView::paint(juce::Graphics &g)
{
    g.setColour(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.fillRect(unpaintedArea1);
    g.fillRect(unpaintedArea2);
    g.fillRect(unpaintedArea3);
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
    int64_t oldViewScale = viewScale;
    viewScale = juce::jlimit(MIN_SCALE_SAMPLE_PER_PIXEL, MAX_SCALE_SAMPLE_PER_PIXEL,
                             int(float(viewScale) * (1.0f + (float(dragY) * PIXEL_SCALE_SPEED))));
    updateGraphsViewScale(viewScale);

    int64_t oldCursorSamplePos = viewPosition + (mouseX * oldViewScale);
    int64_t newCursorSamplePos = viewPosition + (mouseX * viewScale);
    int sampleShiftToAlignZoomToCursor = oldCursorSamplePos - newCursorSamplePos;

    viewPosition += sampleShiftToAlignZoomToCursor;
    if (viewPosition < 0)
    {
        viewPosition = 0;
    }
    updateGraphsViewPositions(viewPosition);
    return true;
}

bool DashboardView::handlePan(int dragX)
{
    int64_t samplesDiff = dragX * viewScale;
    viewPosition -= samplesDiff;
    if (viewPosition < 0)
    {
        viewPosition = 0;
    }
    updateGraphsViewPositions(viewPosition);
    return true;
}

void DashboardView::updateViewMouseDrag(const juce::MouseEvent &e)
{
    std::lock_guard lock(viewMutex);

    int dragX = e.x - lastMouseDragX;
    int dragY = e.y - lastMouseDragY;

    lastMouseDragX = e.x;
    lastMouseDragY = e.y;

    bool needRepaint = false;

    if (dragY != 0)
        needRepaint = handleZoom(dragY, e.x);

    if (dragX != 0)
        needRepaint = handlePan(dragX) || needRepaint;

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
        int64_t lastPlayCursorPos = freqOverTimeGraph->getPlayCursorPosition();
        std::lock_guard lock(viewMutex);
        int64_t leftScreenSideSamplePos = viewPosition;
        int64_t rightScreenSideSamplePos = viewPosition + (freqOverTimeGraph->getBounds().getWidth() * viewScale);
        int64_t screenQuarter = (rightScreenSideSamplePos - leftScreenSideSamplePos) / 4;

        if (lastPlayCursorPos < leftScreenSideSamplePos || lastPlayCursorPos > rightScreenSideSamplePos)
        {
            updateGraphsViewPositions(lastPlayCursorPos - (3 * screenQuarter));
        }
        else if (lastPlayCursorPos >= (rightScreenSideSamplePos - screenQuarter + 1) &&
                 lastPlayCursorPos < (rightScreenSideSamplePos - (screenQuarter >> 1)))
        {
            int64_t increment = (int64_t)((float(elapsedSinceLastCallMs) / 1000.0) * float(VISUAL_SAMPLE_RATE) + 0.5f);
            updateGraphsViewPositions(viewPosition + increment);
        }
    }
}

void DashboardView::propagateClearedFft()
{
    auto tracksClearedInMainView = freqOverTimeGraph->getClearedTrackRanges();
    for (size_t i = 0; i < tracksClearedInMainView.size(); i++)
    {
        trackList.clearTrackFromRange(tracksClearedInMainView[i].trackIdentifier,
                                      tracksClearedInMainView[i].startSample, tracksClearedInMainView[i].length);
        volumeOverTimeGraph->clearTrackFromRange(tracksClearedInMainView[i].trackIdentifier,
                                                 tracksClearedInMainView[i].startSample,
                                                 tracksClearedInMainView[i].length);
    }
}

bool DashboardView::handleNewFftDataTask(std::shared_ptr<NewFftDataTask> task)
{
    if (!task->skip)
    {
        auto processingTimeWaitgroup = processingTimer.getNewProcessingTimerWaitgroup(task->sentTimeUnixMs);
        if (processingTimeWaitgroup != nullptr)
        {
            processingTimeWaitgroup->add();
            int64_t currentTime = juce::Time().getCurrentTime().toMilliseconds();
            int64_t lastFftDrawTimeMsCopy = 0;
            {
                std::lock_guard lock(lastFftDrawTimeMutex);
                lastFftDrawTimeMsCopy = lastFftDrawTimeMs;
            }
            if ((currentTime - lastFftDrawTimeMsCopy) > MAX_IDLE_MS_TIME_BEFORE_CLEAR)
            {
                freqOverTimeGraph->clearDisplayedFFTs();
                trackList.clear();
                volumeOverTimeGraph->clear();
            }
            freqOverTimeGraph->displayNewFftData(task, processingTimeWaitgroup);
            int64_t playHeadPosition = (int64_t)task->segmentStartSample + (int64_t)task->segmentSampleLength;
            freqOverTimeGraph->submitNewPlayCursorPosition(playHeadPosition, task->sampleRate);
            volumeOverTimeGraph->submitNewPlayCursorPosition(playHeadPosition, task->sampleRate);
            trackList.recordSfft(task);
            {
                std::lock_guard lock(lastFftDrawTimeMutex);
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
    auto reuseResultArrayTask = std::make_shared<FftResultVectorReuseTask>(task->fftData);
    taskingManager.broadcastNestedTaskNow(reuseResultArrayTask);

    task->setCompleted(true);
    return false;
}

bool DashboardView::handleTrackColorUpdateTask(std::shared_ptr<TrackColorUpdateTask> task)
{
    juce::Colour col(task->redColorLevel, task->greenColorLevel, task->blueColorLevel);
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
    freqOverTimeGraph->clearDisplayedFFTs();
    volumeOverTimeGraph->clear();
    trackList.clear();
    task->setCompleted(true);
    return false;
}

bool DashboardView::handleVolumeSensitivityTask(std::shared_ptr<VolumeSensitivityTask> task)
{
    auto intensityProjection = std::make_shared<SigmoidProjection>(task->sensitivity);
    intensityTransformer.setProjection(intensityProjection);
    task->setCompleted(true);
    return false;
}

void DashboardView::resized()
{
    auto fftBounds = getLocalBounds();
    auto trackListBounds = fftBounds.removeFromRight(TRACK_LIST_WIDTH).withTrimmedBottom(TIME_GRID_HEIGHT);
    auto frequencyGridBounds = fftBounds.removeFromLeft(FREQUENCY_GRID_WIDTH).withTrimmedBottom(TIME_GRID_HEIGHT);
    auto timeGridBounds = fftBounds.removeFromBottom(TIME_GRID_HEIGHT);
    auto volumeBounds = fftBounds.removeFromBottom(VOLUME_GRAPH_HEIGHT);
    unpaintedArea3 = fftBounds.removeFromBottom(TIME_GRAPHS_PADDING);
    freqOverTimeGraph->setBounds(fftBounds);
    volumeOverTimeGraph->setBounds(volumeBounds);
    frequencyScale.setBounds(frequencyGridBounds);
    timeScale.setBounds(timeGridBounds);
    trackList.setBounds(trackListBounds);

    trackList.setFreqViewWidth(fftBounds.getWidth());

    std::lock_guard lock(viewMutex);
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
    int64_t currentTime = juce::Time().getCurrentTime().toMilliseconds();
    int64_t elapsedSinceLastCallMs = currentTime - lastTimerCallMs;

    // get the time since the fft were last drawn
    int64_t lastFftDrawTimeMsCopy;
    {
        std::lock_guard lock(lastFftDrawTimeMutex);
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
    if (auto newFftDataTask = std::dynamic_pointer_cast<NewFftDataTask>(task))
        if (!newFftDataTask->isCompleted() && !newFftDataTask->hasFailed())
            return handleNewFftDataTask(newFftDataTask);

    if (auto colorUpdateTask = std::dynamic_pointer_cast<TrackColorUpdateTask>(task))
        return handleTrackColorUpdateTask(colorUpdateTask);

    if (auto bpmUpdateTask = std::dynamic_pointer_cast<BpmUpdateTask>(task))
        if (!bpmUpdateTask->isCompleted())
            return handleBpmUpdateTask(bpmUpdateTask);

    if (auto timeSignatureUpdate = std::dynamic_pointer_cast<TimeSignatureUpdateTask>(task))
        if (!timeSignatureUpdate->isCompleted())
            return handleTimeSignatureUpdateTask(timeSignatureUpdate);

    if (auto selectionUpdate = std::dynamic_pointer_cast<TrackSelectionTask>(task))
        if (!selectionUpdate->isCompleted())
            return handleTrackSelectionTask(selectionUpdate);

    if (auto clearTask = std::dynamic_pointer_cast<ClearTask>(task))
        if (!clearTask->isCompleted())
            return handleClearTask(clearTask);

    if (auto volumeSensitivityUpdateTask = std::dynamic_pointer_cast<VolumeSensitivityTask>(task))
        if (!volumeSensitivityUpdateTask->isCompleted())
            return handleVolumeSensitivityTask(volumeSensitivityUpdateTask);

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
    auto positionRelativeToDashboardView = me.getEventRelativeTo(freqOverTimeGraph.get());
    bool showCursor = freqOverTimeGraph->getBounds().contains(me.getPosition());
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
        float halfHeight = 0.5f * float(freqOverTimeGraph->getHeight());
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
        mouseOverFreqTimeGraph, positionInFreq * 0.5f * float(VISUAL_SAMPLE_RATE), timeInSample);
    taskingManager.broadcastTask(cursorUpdateTask);
}

void DashboardView::mouseExit(const juce::MouseEvent &)
{
    freqOverTimeGraph->setMouseCursor(false, -1, -1);
    auto cursorUpdateTask = std::make_shared<MouseCursorInfoTask>(false, 0, 0);
    taskingManager.broadcastTask(cursorUpdateTask);
    freqOverTimeGraph->repaint();
}