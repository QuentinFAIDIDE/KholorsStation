#include "FreqTimeView.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/BpmUpdateTask.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/TrackColorUpdateTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/CpuImageDrawingBackend.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/FrequencyScale.h"
#include "StationApp/GUI/GpuTextureDrawingBackend.h"
#include "StationApp/Maths/NormalizedBijectiveProjection.h"
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

FreqTimeView::FreqTimeView(TrackInfoStore &tis)
    : trackInfoStore(tis), viewPosition(0), viewScale(150),
      frequencyScale(frequencyTransformer, VISUAL_SAMPLE_RATE >> 1)
{
    fftDrawBackend =
        std::make_shared<GpuTextureDrawingBackend>(trackInfoStore, frequencyTransformer, intensityTransformer);
    // fftDrawBackend = std::make_shared<CpuImageDrawingBackend>(trackInfoStore);
    addAndMakeVisible(fftDrawBackend.get());

    auto freqProjection = std::make_shared<Log10Projection>(0.005);
    frequencyTransformer.setProjection(freqProjection);

    auto intensityProjection = std::make_shared<SigmoidProjection>();
    auto invertIntensity = std::make_shared<InvertProjection>(intensityProjection);
    intensityTransformer.setProjection(invertIntensity);

    lastTimerCallMs = juce::Time().getCurrentTime().toMilliseconds();

    timeScale.setBpm(120);
    timeScale.setViewScale(viewScale);
    timeScale.setViewPosition(viewScale);

    addAndMakeVisible(frequencyScale);
    addAndMakeVisible(timeScale);

    startTimer(VIEW_MOVE_TIME_INTERVAL_MS);
}

FreqTimeView::~FreqTimeView()
{
}

void FreqTimeView::paint(juce::Graphics &g)
{
}

void FreqTimeView::paintOverChildren(juce::Graphics &g)
{
}

void FreqTimeView::resized()
{

    auto fftBounds = getLocalBounds();
    auto frequencyGridBounds = fftBounds.removeFromLeft(FREQUENCY_GRID_WIDTH).withTrimmedBottom(TIME_GRID_HEIGHT);
    auto timeGridBounds = fftBounds.removeFromBottom(TIME_GRID_HEIGHT);
    fftDrawBackend->setBounds(fftBounds);
    frequencyScale.setBounds(frequencyGridBounds);
    timeScale.setBounds(timeGridBounds);

    std::lock_guard lock(viewMutex);
    fftDrawBackend->updateViewPosition(viewPosition);
}

void FreqTimeView::timerCallback()
{
    // if last fft draw time is less than MAX_TIME_SINCE_FFT_UPDATE_TO_CENTER_VIEW_MS ms ago
    int64_t currentTime = juce::Time().getCurrentTime().toMilliseconds();
    int64_t elapsedSinceLastCallMs = currentTime - lastTimerCallMs;
    int64_t lastFftDrawTimeMsCopy;
    {
        std::lock_guard lock(lastFftDrawTimeMutex);
        lastFftDrawTimeMsCopy = lastFftDrawTimeMs;
    }
    // if user is not currently moving the view and that last fft was received recently enough
    if (!isViewMoving && (currentTime - lastFftDrawTimeMsCopy) < MAX_TIME_SINCE_FFT_UPDATE_TO_CENTER_VIEW_MS)
    {
        // first check that play cursor is in bounds
        int64_t lastPlayCursorPos = fftDrawBackend->getPlayCursorPosition();
        std::lock_guard lock(viewMutex);
        int64_t leftScreenSideSamplePos = viewPosition;
        int64_t rightScreenSideSamplePos = viewPosition + (getLocalBounds().getWidth() * viewScale);
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
            repaint();
        }
        // if play cursor is between 3/4 of view and right side, apply constant view moving speed
        else if (lastPlayCursorPos >= (rightScreenSideSamplePos - screenQuarter + 1))
        {
            viewPosition += (int64_t)((float(elapsedSinceLastCallMs) / 1000.0) * float(VISUAL_SAMPLE_RATE) + 0.5f);
            if (viewPosition < 0)
            {
                viewPosition = 0;
            }
            fftDrawBackend->updateViewPosition(viewPosition);
            timeScale.setViewPosition(viewPosition);
            repaint();
        }
    }
    lastTimerCallMs = currentTime;
}

bool FreqTimeView::taskHandler(std::shared_ptr<Task> task)
{
    auto newFftDataTask = std::dynamic_pointer_cast<NewFftDataTask>(task);
    if (newFftDataTask != nullptr && !newFftDataTask->isCompleted() && !newFftDataTask->hasFailed())
    {
        spdlog::debug("Received a NewFftData task in the FreqTimeView");
        // TODO: if inside a loop and crossing the end, also pass the part that is beyond loop end to the beginning of
        // the loop
        // call on the drawing backend to add the FFT data to the displayed textures
        if (fftDrawBackend != nullptr)
        {
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
            }

            // send fft data to drawing backend and update play cursor
            fftDrawBackend->displayNewFftData(newFftDataTask);
            fftDrawBackend->submitNewPlayCursorPosition((int64_t)newFftDataTask->segmentStartSample +
                                                            (int64_t)newFftDataTask->segmentSampleLength,
                                                        newFftDataTask->sampleRate);

            // record last drawing time
            {
                std::lock_guard lock(lastFftDrawTimeMutex);
                lastFftDrawTimeMs = currentTime;
            }
        }
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
            fftDrawBackend->updateBpm(bpmUpdateTask->bpm);
            timeScale.setBpm(bpmUpdateTask->bpm);
            lastReceivedBpm = bpmUpdateTask->bpm;
        }
        bpmUpdateTask->setCompleted(true);
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
            repaint();
        }
    }
}