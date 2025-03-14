#include "PluginEditor.h"
#include "GUIToolkit/GUIData.h"
#include "PluginProcessor.h"
#include "StationPlugin/Licensing/DummyLicenseManager.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p, TaskingManager &tm)
    : AudioProcessorEditor(&p), taskManager(tm), trackInfoStore(taskManager), freqTimeView(trackInfoStore, taskManager),
      infoBar(taskManager), clearButton(taskManager), volumeSensitivitySlider(taskManager)
{
    typeface = juce::Typeface::createSystemTypefaceFor(GUIData::RobotoRegular_ttf, GUIData::RobotoRegular_ttfSize);
    appLookAndFeel.setDefaultSansSerifTypeface(typeface);
    juce::LookAndFeel::setDefaultLookAndFeel(&appLookAndFeel);
    setLookAndFeel(&appLookAndFeel);

    setSize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

    addAndMakeVisible(helpButton);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(volumeSensitivitySlider);
    addAndMakeVisible(freqTimeView);
    addAndMakeVisible(infoBar);

    taskListenerIdsToClear.push_back(taskManager.registerTaskListener(&trackInfoStore));
    taskListenerIdsToClear.push_back(taskManager.registerTaskListener(&freqTimeView));
    taskListenerIdsToClear.push_back(taskManager.registerTaskListener(&infoBar));

    auto licenseData = DummyLicenseManager::getUserDataAndKeyFromDisk();
    if (!licenseData.has_value())
    {
        throw std::runtime_error("No license key after license check");
    }
    infoBar.setLicenseInfo(licenseData->username, licenseData->email);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    for (auto it = taskListenerIdsToClear.begin(); it != taskListenerIdsToClear.end(); it++)
    {
        taskManager.purgeTaskListener(*it);
    }
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
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

void AudioPluginAudioProcessorEditor::resized()
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
