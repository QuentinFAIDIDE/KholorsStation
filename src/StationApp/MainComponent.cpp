#include "MainComponent.h"

#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/BottomInfoLine.h"
#include "StationApp/GUI/ClearButton.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/FreqTimeView.h"
#include "StationApp/GUI/SensitivitySlider.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <spdlog/spdlog.h>

#define DEFAULT_SERVER_PORT 7849

MainComponent::MainComponent()
    : trackInfoStore(taskManager), freqTimeView(trackInfoStore, taskManager),
      audioDataWorker(audioDataServer, taskManager), infoBar(taskManager), clearButton(taskManager),
      volumeSensitivitySlider(taskManager)
{
    addAndMakeVisible(freqTimeView);
    addAndMakeVisible(infoBar);

    setSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

    addAndMakeVisible(helpButton);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(volumeSensitivitySlider);

    taskManager.registerTaskListener(&trackInfoStore);
    taskManager.registerTaskListener(&freqTimeView);

    audioDataServer.setTaskManager(&taskManager);

    audioDataServer.setServerToListenOnPort(DEFAULT_SERVER_PORT);

    taskManager.registerTaskListener(this);
    taskManager.registerTaskListener(&infoBar);
    taskManager.startTaskBroadcast();
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

    int middlePadding = 6;
    int versionPadding = 3;

    int mainTitleWidth = sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT).getStringWidth("KHOLORS");
    int subTitleWidth = sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT).getStringWidth("STATION");
    int versionWidth = sharedFonts->roboto.withHeight(VERSION_FONT_HEIGHT).getStringWidth(GIT_DESCRIBE_VERSION);

    int totalWidth = mainTitleWidth + middlePadding + subTitleWidth + versionPadding + versionWidth;

    auto bounds = getLocalBounds();
    g.setFont(sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT));
    g.setColour(KHOLORS_COLOR_TEXT);

    auto topspace = bounds.removeFromTop(MENU_BAR_HEIGHT);
    topspace.removeFromLeft(TOPBAR_RIGHT_PADDING);
    auto titleSpace = topspace.removeFromLeft(totalWidth);
    auto leftTitleSpace = titleSpace.removeFromLeft(mainTitleWidth);

    titleSpace.removeFromLeft(middlePadding);
    auto subTitleSpace = titleSpace.removeFromLeft(subTitleWidth);

    titleSpace.removeFromLeft(versionPadding);
    auto versionArea = titleSpace.removeFromLeft(versionWidth);
    versionArea.setY(versionArea.getY() + 38);

    g.drawText("KHOLORS", leftTitleSpace, juce::Justification::centredRight, false);

    g.setFont(sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT));
    g.setColour(KHOLORS_COLOR_TEXT_DARKER);
    g.drawText("STATION", subTitleSpace, juce::Justification::centredRight, false);

    g.setFont(sharedFonts->roboto.withHeight(VERSION_FONT_HEIGHT));
    g.drawText(GIT_DESCRIBE_VERSION, versionArea, juce::Justification::topRight, false);

    auto artifaktLogoBounds = topspace.removeFromRight(TOPBAR_RIGHT_PADDING + TOP_LOGO_WIDTH);
    artifaktLogoBounds.removeFromRight(TOPBAR_RIGHT_PADDING);
    sharedSvgs->artifaktNdLogo->drawWithin(g, artifaktLogoBounds.toFloat(), juce::RectanglePlacement::centred, 1.0f);

    topspace.removeFromRight(TOPBAR_BUTTONS_RIGHT_MARGIN + HELP_BUTTON_WIDTH + CLEAR_BUTTON_WIDTH +
                             TOPBAR_BUTTONS_PADDING + TOPBAR_BUTTONS_PADDING);

    auto sliderWithPictogramsArea = topspace.removeFromRight(
        SENSITIVITY_SLIDER_PICTOGRAM_WIDTH + TOPBAR_BUTTONS_PADDING + SENSITIVITY_SLIDER_WIDTH +
        TOPBAR_BUTTONS_PADDING + SENSITIVITY_SLIDER_PICTOGRAM_WIDTH);

    if (sliderWithPictogramsArea.getWidth() >= SENSITIVITY_SLIDER_PICTOGRAM_WIDTH + TOPBAR_BUTTONS_PADDING +
                                                   SENSITIVITY_SLIDER_WIDTH + TOPBAR_BUTTONS_PADDING +
                                                   SENSITIVITY_SLIDER_PICTOGRAM_WIDTH)
    {

        auto leftSliderPicto = sliderWithPictogramsArea.removeFromLeft(SENSITIVITY_SLIDER_PICTOGRAM_WIDTH);
        auto rightSliderPicto = sliderWithPictogramsArea.removeFromRight(SENSITIVITY_SLIDER_PICTOGRAM_WIDTH);

        sharedSvgs->lowWaveIcon->drawWithin(g, leftSliderPicto.toFloat(), juce::RectanglePlacement::centred, 1.0f);
        sharedSvgs->highWaveIcon->drawWithin(g, rightSliderPicto.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    }
}

void MainComponent::resized()
{
    juce::Rectangle<int> localBounds = getLocalBounds();
    auto topBar = localBounds.removeFromTop(MENU_BAR_HEIGHT);
    infoBar.setBounds(localBounds.removeFromBottom(BOTTOM_INFO_LINE_HEIGHT));
    freqTimeView.setBounds(localBounds);

    auto buttonsArea = topBar.withTrimmedRight(TOPBAR_RIGHT_PADDING + TOP_LOGO_WIDTH + TOPBAR_BUTTONS_RIGHT_MARGIN);
    buttonsArea.reduce(0, (buttonsArea.getHeight() - TOPBAR_BUTTON_HEIGHT) / 2);

    auto helpButtonArea = buttonsArea.removeFromRight(HELP_BUTTON_WIDTH);
    helpButton.setBounds(helpButtonArea);

    auto clearButtonArea = buttonsArea.removeFromRight(CLEAR_BUTTON_WIDTH);
    clearButton.setBounds(clearButtonArea);

    int middlePadding = 6;
    int versionPadding = 3;
    int mainTitleWidth = sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT).getStringWidth("KHOLORS");
    int subTitleWidth = sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT).getStringWidth("STATION");
    int versionWidth = sharedFonts->roboto.withHeight(VERSION_FONT_HEIGHT).getStringWidth(GIT_DESCRIBE_VERSION);
    int totalTextWidth =
        TOPBAR_RIGHT_PADDING + mainTitleWidth + middlePadding + subTitleWidth + versionPadding + versionWidth;
    buttonsArea.removeFromLeft(totalTextWidth);

    if (buttonsArea.getWidth() <
        (TOPBAR_BUTTONS_PADDING + TOPBAR_BUTTONS_PADDING + SENSITIVITY_SLIDER_PICTOGRAM_WIDTH + TOPBAR_BUTTONS_PADDING +
         SENSITIVITY_SLIDER_WIDTH + TOPBAR_BUTTONS_PADDING + SENSITIVITY_SLIDER_PICTOGRAM_WIDTH))
    {
        volumeSensitivitySlider.setVisible(false);
    }
    else
    {
        volumeSensitivitySlider.setVisible(true);

        buttonsArea.removeFromRight(TOPBAR_BUTTONS_PADDING * 2);
        buttonsArea.removeFromRight(SENSITIVITY_SLIDER_PICTOGRAM_WIDTH);
        buttonsArea.removeFromRight(TOPBAR_BUTTONS_PADDING);
        auto sensitivitySliderArea = buttonsArea.removeFromRight(SENSITIVITY_SLIDER_WIDTH);
        volumeSensitivitySlider.setBounds(sensitivitySliderArea);
    }
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

juce::Path MainComponent::getShadowPath()
{
    // coordinates of the freqview top left angle bottom
    auto topLeftAngleBottom = getLocalBounds()
                                  .withTrimmedTop(MENU_BAR_HEIGHT)
                                  .withTrimmedLeft(FREQUENCY_GRID_WIDTH)
                                  .getTopLeft()
                                  .translated(FREQVIEW_BORDER_WIDTH, FREQVIEW_ROUNDED_CORNERS_WIDTH / 2);
    // next point is 45 degree angle to left side of the screen (1:1 h/v pixel ratio)
    auto point1 = topLeftAngleBottom.withX(getLocalBounds().getX()).translated(0, topLeftAngleBottom.getX());
    auto point2 = getLocalBounds().getBottomLeft();
    auto point3 = getLocalBounds()
                      .withTrimmedRight(TRACK_LIST_WIDTH)
                      .getBottomRight()
                      .translated(-(BOTTOM_INFO_LINE_HEIGHT + TIME_GRID_HEIGHT), 0);
    auto point4 = getLocalBounds()
                      .withTrimmedRight(TRACK_LIST_WIDTH)
                      .withTrimmedBottom((BOTTOM_INFO_LINE_HEIGHT + TIME_GRID_HEIGHT))
                      .getBottomRight()
                      .translated(-FREQVIEW_ROUNDED_CORNERS_WIDTH / 2, 0);

    juce::Path shadowPath;
    shadowPath.startNewSubPath(topLeftAngleBottom.toFloat());
    shadowPath.lineTo(point1.toFloat());
    shadowPath.lineTo(point2.toFloat());
    shadowPath.lineTo(point3.toFloat());
    shadowPath.lineTo(point4.toFloat());
    shadowPath.closeSubPath();

    return shadowPath;
}