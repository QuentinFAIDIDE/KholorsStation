#include "MainComponent.h"

#include "GUIToolkit/Consts.h"
#include "GUIToolkit/GUIData.h"
#include "StationApp/GUI/BottomPanel.h"
#include <spdlog/spdlog.h>

#define DEFAULT_SERVER_PORT 7849

MainComponent::MainComponent() : menuBarModel(taskManager), bottomPanel(taskManager)
{
    // Various GUI related initializations
    juce::Typeface::Ptr tface =
        juce::Typeface::createSystemTypefaceFor(GUIData::RobotoRegular_ttf, GUIData::RobotoRegular_ttfSize);
    appLookAndFeel.setDefaultSansSerifTypeface(tface);
    setLookAndFeel(&appLookAndFeel);
    juce::LookAndFeel::setDefaultLookAndFeel(&appLookAndFeel);

    menuBar.setModel(&menuBarModel);
    addAndMakeVisible(menuBar);

    bottomPanel.setSize(getWidth(), 300);
    addAndMakeVisible(bottomPanel);

    addAndMakeVisible(freqTimeView);

    setSize(1440, 900);

    audioDataReceiver.setTaskManager(&taskManager);

    audioDataReceiver.setServerToListenOnPort(DEFAULT_SERVER_PORT);

    taskManager.registerTaskListener(this);
    taskManager.startTaskBroadcast();
}

MainComponent::~MainComponent()
{
    audioDataReceiver.stopServer();
    taskManager.stopTaskBroadcast();

    menuBar.setModel(nullptr);
    appLookAndFeel.setDefaultSansSerifTypeface(nullptr);
    setLookAndFeel(nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    juce::Rectangle<int> localBounds = getLocalBounds();
    menuBar.setBounds(localBounds.removeFromTop(MENU_BAR_HEIGHT));
    bottomPanel.setBounds(localBounds.removeFromBottom(bottomPanel.getHeight()));
    freqTimeView.setBounds(localBounds);
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
    int middlePadding = 6;
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