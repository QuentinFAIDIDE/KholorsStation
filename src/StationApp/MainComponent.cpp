#include "MainComponent.h"

#include "GUIToolkit/Consts.h"
#include "GUIToolkit/GUIData.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "StationApp/GUI/BottomInfoLine.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/FreqTimeView.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include <spdlog/spdlog.h>

#define DEFAULT_SERVER_PORT 7849

MainComponent::MainComponent()
    : trackInfoStore(taskManager), freqTimeView(trackInfoStore, taskManager),
      audioDataWorker(audioDataServer, taskManager), infoBar(taskManager)
{
    addAndMakeVisible(freqTimeView);
    addAndMakeVisible(infoBar);

    setSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

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
    localBounds.removeFromTop(MENU_BAR_HEIGHT);
    infoBar.setBounds(localBounds.removeFromBottom(BOTTOM_INFO_LINE_HEIGHT));
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

    // draw the shadow over the freqview
    juce::Path shadowPath = getShadowPath();
    g.setColour(juce::Colour(3, 6, 31).withAlpha(0.3f));
    g.fillPath(shadowPath);

    auto bordersGradient = juce::ColourGradient(
        juce::Colour(juce::uint8(234), 239, 255, 0.45f),
        getLocalBounds().removeFromRight(getWidth() / 4).removeFromTop(getHeight() / 8).getTopLeft().toFloat(),
        juce::Colour(juce::uint8(215), 224, 255, 0.f),
        getLocalBounds().getBottomLeft().translated(getWidth() / 3, -getHeight() / 5).toFloat(), false);

    auto freqViewBounds = getLocalBounds()
                              .withTrimmedTop(MENU_BAR_HEIGHT)
                              .withTrimmedBottom(TIME_GRID_HEIGHT + BOTTOM_INFO_LINE_HEIGHT)
                              .withTrimmedLeft(FREQUENCY_GRID_WIDTH)
                              .withTrimmedRight(TRACK_LIST_WIDTH)
                              .expanded(FREQVIEW_OUTER_BORDER_WIDTH, FREQVIEW_OUTER_BORDER_WIDTH);

    g.setGradientFill(bordersGradient);
    g.fillRoundedRectangle(freqViewBounds.toFloat(), FREQVIEW_ROUNDED_CORNERS_WIDTH);
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