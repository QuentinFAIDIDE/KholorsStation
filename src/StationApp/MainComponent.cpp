#include "MainComponent.h"

#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/CheckUpdates.h"
#include "StationApp/GUI/BottomInfoLine.h"
#include "StationApp/GUI/DashboardView.h"
#include "StationApp/GUI/Graphs/GraphBorders.h"
#include "StationApp/GUI/HelpDialogContent.h"
#include "StationApp/GUI/TopBar.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <spdlog/spdlog.h>

constexpr int DEFAULT_SERVER_PORT = 7849;

MainComponent::MainComponent()
    : trackInfoStore(taskManager), dashboardView(trackInfoStore, taskManager),
      audioDataWorker(audioDataServer, taskManager), infoBar(taskManager), topBar(taskManager)
{
    addAndMakeVisible(dashboardView);
    addAndMakeVisible(infoBar);
    addAndMakeVisible(topBar);

    setSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

    taskManager.registerTaskListener(&trackInfoStore);
    taskManager.registerTaskListener(&dashboardView);

    audioDataServer.setTaskManager(&taskManager);

    audioDataServer.setServerToListenOnPort(DEFAULT_SERVER_PORT);

    taskManager.registerTaskListener(this);
    taskManager.registerTaskListener(&infoBar);
    taskManager.startTaskBroadcast();

    topBar.setUpdateAvailable(checkIfUpdateAvailable());

    if (showTipsAtStartup)
    {
        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = "Usage Tips";
        juce::OptionalScopedPointer<HelpDialogContent> const contentContainer;
        options.resizable = false;
        options.useNativeTitleBar = false;
        options.content.set(new HelpDialogContent(), true);
        options.launchAsync();
    }
}

MainComponent::~MainComponent()
{
    audioDataServer.stopServer();

    taskManager.shutdownBackgroundThreadAsync();
    while (taskManager.isBackgroundThreadRunning())
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
    }

    menuBar.setModel(nullptr);
}

void MainComponent::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    juce::Rectangle<int> localBounds = getLocalBounds();
    topBar.setBounds(localBounds.removeFromTop(TOP_BAR_HEIGHT));
    infoBar.setBounds(localBounds.removeFromBottom(BOTTOM_INFO_LINE_HEIGHT));
    dashboardView.setBounds(localBounds);
}

bool MainComponent::taskHandler(std::shared_ptr<Task> task)
{
    auto const quitTask = std::dynamic_pointer_cast<QuittingTask>(task);
    if (quitTask != nullptr)
    {
        juce::JUCEApplicationBase::quit();
        quitTask->setCompleted(true);
        return true;
    }

    return false;
}
