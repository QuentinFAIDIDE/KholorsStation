#include "FreqTimeView.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/GUI/CpuImageDrawingBackend.h"
#include <memory>
#include <spdlog/spdlog.h>

FreqTimeView::FreqTimeView()
{
    fftDrawBackend = std::make_shared<CpuImageDrawingBackend>();
    addAndMakeVisible(fftDrawBackend.get());
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