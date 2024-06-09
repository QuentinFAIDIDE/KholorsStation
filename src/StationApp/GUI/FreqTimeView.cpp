#include "FreqTimeView.h"

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