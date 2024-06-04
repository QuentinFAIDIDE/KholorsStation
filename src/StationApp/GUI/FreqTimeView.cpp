#include "FreqTimeView.h"

FreqTimeView::FreqTimeView()
{
}

FreqTimeView::~FreqTimeView()
{
}

void FreqTimeView::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}
