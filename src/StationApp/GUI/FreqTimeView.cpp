#include "FreqTimeView.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/CpuImageDrawingBackend.h"
#include "StationApp/GUI/GpuTextureDrawingBackend.h"
#include "StationApp/Maths/NormalizedBijectiveProjection.h"
#include <memory>
#include <spdlog/spdlog.h>

FreqTimeView::FreqTimeView(TrackInfoStore &tis) : trackInfoStore(tis), viewPosition(0), viewScale(150)
{
    fftDrawBackend = std::make_shared<GpuTextureDrawingBackend>(trackInfoStore);
    addAndMakeVisible(fftDrawBackend.get());

    auto freqProjection = std::make_shared<Log10Projection>(0.005);
    frequencyTransformer.setProjection(freqProjection);
    fftDrawBackend->setFrequencyTransformer(&frequencyTransformer);

    auto intensityProjection = std::make_shared<SigmoidProjection>();
    intensityTransformer.setProjection(intensityProjection);
    fftDrawBackend->setIntensityTransformer(&intensityTransformer);
}

FreqTimeView::~FreqTimeView()
{
}

void FreqTimeView::paint(juce::Graphics &g)
{
}

void FreqTimeView::paintOverChildren(juce::Graphics &)
{
    // TODO: draw labels here
}

void FreqTimeView::resized()
{
    fftDrawBackend->setBounds(getLocalBounds());
    fftDrawBackend->updateViewPosition(viewPosition);
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
            fftDrawBackend->displayNewFftData(newFftDataTask);
        }
        newFftDataTask->setCompleted(true);
        // TODO: under lock, update if necessary latest data location
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
}

void FreqTimeView::mouseDrag(const juce::MouseEvent &e)
{
    if (e.mods.isMiddleButtonDown())
    {
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

            needRepaint = true;
        }

        if (needRepaint)
        {
            repaint();
        }
    }
}