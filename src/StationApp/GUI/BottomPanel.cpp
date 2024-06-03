#include "BottomPanel.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/EmptyTab.h"
#include <memory>

BottomPanel::BottomPanel() : tabs(juce::TabbedButtonBar::Orientation::TabsAtTop)
{
    sizeConstrainer = std::make_shared<juce::ComponentBoundsConstrainer>();
    sizeConstrainer->setMinimumHeight(BOTTOM_BAR_MINIMUM_HEIGHT);
    sizeConstrainer->setMaximumHeight(BOTTOM_BAR_MAXIMUM_HEIGHT);

    resizableEdge = std::make_shared<juce::ResizableEdgeComponent>(this, sizeConstrainer.get(),
                                                                   juce::ResizableEdgeComponent::topEdge);

    tabs.setTabBarDepth(TABS_HEIGHT);
    tabs.addTab("Server", COLOR_BACKGROUND, new EmptyTab(), true);
    tabs.addTab("Display", COLOR_BACKGROUND, new EmptyTab(), true);

    addAndMakeVisible(*resizableEdge);
    addAndMakeVisible(tabs);
}

void BottomPanel::resized()
{
    auto bounds = getLocalBounds();
    resizableEdge->setBounds(bounds.removeFromTop(RESIZE_BAR_HEIGHT));
    tabs.setBounds(bounds);
}

void BottomPanel::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}