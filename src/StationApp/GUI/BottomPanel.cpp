#include "BottomPanel.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/EmptyTab.h"
#include "StationApp/GUI/ServerTab.h"
#include <memory>

BottomPanel::BottomPanel(TaskingManager *tm) : tabs(juce::TabbedButtonBar::Orientation::TabsAtTop)
{
    sizeConstrainer = std::make_shared<juce::ComponentBoundsConstrainer>();
    sizeConstrainer->setMinimumHeight(BOTTOM_BAR_MINIMUM_HEIGHT);
    sizeConstrainer->setMaximumHeight(BOTTOM_BAR_MAXIMUM_HEIGHT);

    resizableEdge = std::make_shared<juce::ResizableEdgeComponent>(this, sizeConstrainer.get(),
                                                                   juce::ResizableEdgeComponent::topEdge);

    tabs.setTabBarDepth(TABS_HEIGHT);
    tabs.addTab(TRANS("Server"), COLOR_BACKGROUND, new ServerTab(tm), true);
    tabs.addTab(TRANS("Display"), COLOR_BACKGROUND, new EmptyTab(), true);

    lastHeight = 0;

    addAndMakeVisible(*resizableEdge);
    addAndMakeVisible(tabs);
}

void BottomPanel::resized()
{
    auto bounds = getLocalBounds();
    resizableEdge->setBounds(bounds.removeFromTop(RESIZE_BAR_HEIGHT));
    tabs.setBounds(bounds);
    // if height change, the parent needs to resize its internal components
    if (lastHeight != getHeight())
    {
        lastHeight = getHeight();
        auto parent = getParentComponent();
        if (parent != nullptr)
        {
            parent->resized();
        }
    }
}

void BottomPanel::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}