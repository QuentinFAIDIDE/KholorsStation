#include "MainComponent.h"

#include "GUIToolkit/Consts.h"
#include "GUIToolkit/GUIToolkit.h"
#include <spdlog/spdlog.h>

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

    setSize(1440, 900);

    taskManager.registerTaskListener(this);
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

bool MainComponent::taskHandler(std::shared_ptr<Task> task)
{
    auto quitTask = std::dynamic_pointer_cast<QuittingTask>(task);
    if (quitTask != nullptr)
    {
        juce::JUCEApplicationBase::quit();
        quitTask->setCompleted(true);
        return true;
    }

    return false;
}

void MainComponent::paintOverChildren(juce::Graphics &g)
{
    int middlePadding = 8;
    int leftPadding = 8;

    int mainTitleWidth = sharedFonts->robotoBlack.withHeight(MENU_BAR_HEIGHT).getStringWidth("KHOLORS II");
    int subTitleWidth = sharedFonts->roboto.withHeight(MENU_BAR_HEIGHT).getStringWidth("STATION");
    int totalWidth = mainTitleWidth + middlePadding + subTitleWidth;

    auto bounds = getLocalBounds();
    g.setFont(sharedFonts->robotoBlack.withHeight(MENU_BAR_HEIGHT));
    g.setColour(COLOR_TEXT);

    auto topspace = bounds.removeFromTop(MENU_BAR_HEIGHT);
    topspace.removeFromRight(leftPadding);
    auto titleSpace = topspace.removeFromRight(totalWidth);
    auto leftTitleSpace = titleSpace.removeFromLeft(mainTitleWidth);
    titleSpace.removeFromLeft(middlePadding);

    g.drawText("KHOLORS II", leftTitleSpace, juce::Justification::centredLeft, false);

    g.setFont(sharedFonts->roboto.withHeight(MENU_BAR_HEIGHT));
    g.setColour(COLOR_TEXT_DARKER);
    g.drawText("STATION", titleSpace, juce::Justification::centredLeft, false);
}