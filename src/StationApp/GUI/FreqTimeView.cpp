#include "FreqTimeView.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include <memory>

FreqTimeView::FreqTimeView()
{
    // TODO: in the future, allow user to switch these and add a mutex
    // TODO: allocate a FftDrawingBackend implementation
}

FreqTimeView::~FreqTimeView()
{
}

void FreqTimeView::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void FreqTimeView::paintOverChildren(juce::Graphics &)
{
    // TODO: draw labels here
}

void FreqTimeView::resized()
{
    if (fftDrawBackend != nullptr)
    {
        fftDrawBackend->setBounds(getLocalBounds());
    }
}

bool FreqTimeView::taskHandler(std::shared_ptr<Task> task)
{
    auto newFftDataTask = std::dynamic_pointer_cast<NewFftDataTask>(task);
    if (newFftDataTask != nullptr && !newFftDataTask->isCompleted() && !newFftDataTask->hasFailed())
    {
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