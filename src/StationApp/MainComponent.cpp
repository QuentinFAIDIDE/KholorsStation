#include "MainComponent.h"

#include "GUIToolkit/Consts.h"
#include "GUIToolkit/GUIData.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/GUI/BottomPanel.h"
#include "StationApp/GUI/FreqTimeView.h"
#include "TaskManagement/TaskingManager.h"
#include <spdlog/spdlog.h>

#define DEFAULT_SERVER_PORT 7849

MainComponent::MainComponent()
    : menuBarModel(taskManager), bottomPanel(taskManager), freqTimeView(trackInfoStore),
      audioDataWorker(audioDataServer, taskManager)
{

    menuBar.setModel(&menuBarModel);
    addAndMakeVisible(menuBar);

    bottomPanel.setSize(getWidth(), 210);
    addAndMakeVisible(bottomPanel);

    addAndMakeVisible(freqTimeView);

    setSize(1440, 900);

    taskManager.registerTaskListener(&trackInfoStore);
    taskManager.registerTaskListener(&freqTimeView);

    audioDataServer.setTaskManager(&taskManager);

    audioDataServer.setServerToListenOnPort(DEFAULT_SERVER_PORT);

    taskManager.registerTaskListener(this);
    taskManager.startTaskBroadcast();
}

MainComponent::~MainComponent()
{
    audioDataServer.stopServer();
    taskManager.stopTaskBroadcast();

    menuBar.setModel(nullptr);
    // appLookAndFeel.setDefaultSansSerifTypeface(nullptr);
    // setLookAndFeel(nullptr);
    // juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
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