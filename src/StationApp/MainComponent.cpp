#include "MainComponent.h"

#include "GUIToolkit/GUIToolkit.h"

MainComponent::MainComponent() : menuBarModel(taskManager)
{
    // Various GUI related initializations
    juce::Typeface::Ptr tface =
        juce::Typeface::createSystemTypefaceFor(GUIData::RobotoRegular_ttf, GUIData::RobotoRegular_ttfSize);
    appLookAndFeel.setDefaultSansSerifTypeface(tface);
    setLookAndFeel(&appLookAndFeel);
    juce::LookAndFeel::setDefaultLookAndFeel(&appLookAndFeel);

    menuBar.setModel(&menuBarModel);
    addAndMakeVisible(menuBar);

    setSize(600, 400);

    taskManager.startTaskBroadcast();
}

MainComponent::~MainComponent()
{
    taskManager.stopTaskBroadcast();

    menuBar.setModel(nullptr);
    appLookAndFeel.setDefaultSansSerifTypeface(nullptr);
    setLookAndFeel(nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setFont(juce::Font(16.0f));
    g.setColour(juce::Colours::white);
    g.drawText("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    juce::Rectangle<int> localBounds = getLocalBounds();
    menuBar.setBounds(localBounds.removeFromTop(MENU_BAR_HEIGHT));
}