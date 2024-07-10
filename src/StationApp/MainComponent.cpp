#include "MainComponent.h"

#include "GUIToolkit/Consts.h"
#include "GUIToolkit/GUIData.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/FreqTimeView.h"
#include "TaskManagement/TaskingManager.h"
#include <spdlog/spdlog.h>

#define DEFAULT_SERVER_PORT 7849

MainComponent::MainComponent()
    : trackInfoStore(taskManager), freqTimeView(trackInfoStore), audioDataWorker(audioDataServer, taskManager)
{
    addAndMakeVisible(freqTimeView);

    setSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

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
}

void MainComponent::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    juce::Rectangle<int> localBounds = getLocalBounds();
    localBounds.removeFromTop(MENU_BAR_HEIGHT); // note that we don't show top bar anymore
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
    int rightPadding = 25;

    int mainTitleWidth = sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT).getStringWidth("KHOLORS II");
    int subTitleWidth = sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT).getStringWidth("STATION");
    int totalWidth = mainTitleWidth + middlePadding + subTitleWidth;

    auto bounds = getLocalBounds();
    g.setFont(sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT));
    g.setColour(COLOR_TEXT);

    auto topspace = bounds.removeFromTop(MENU_BAR_HEIGHT);
    topspace.removeFromLeft(rightPadding);
    auto titleSpace = topspace.removeFromLeft(totalWidth);
    auto leftTitleSpace = titleSpace.removeFromLeft(mainTitleWidth);
    titleSpace.removeFromLeft(middlePadding);

    g.drawText("KHOLORS II", leftTitleSpace, juce::Justification::centredRight, false);

    g.setFont(sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT));
    g.setColour(COLOR_TEXT_DARKER);
    g.drawText("STATION", titleSpace, juce::Justification::centredRight, false);

    auto artifaktLogoBounds = topspace.removeFromRight(rightPadding + TOP_LOGO_WIDTH);
    artifaktLogoBounds.removeFromRight(rightPadding);
    sharedSvgs->artifaktNdLogo->drawWithin(g, artifaktLogoBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);
}