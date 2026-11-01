#include "TopBar.h"

#include "GUIToolkit/Consts.h"

constexpr int APP_NAME_FONT_HEIGHT = 40;
constexpr int VERSION_FONT_HEIGHT = 18;
constexpr int TOPBAR_SIDE_PADDING = 25;
constexpr int TITLE_MIDDLE_PADDING = 6;
constexpr int TITLE_VERSION_PADDING = 3;
constexpr int VERSION_TOP_OFFSET = 38;
constexpr int HELP_BUTTON_WIDTH = 75;
constexpr int CLEAR_BUTTON_WIDTH = 90;
constexpr int TOPBAR_BUTTON_HEIGHT = 55;
constexpr int TOPBAR_BUTTONS_PADDING = 6;
constexpr int TOPBAR_BUTTONS_RIGHT_MARGIN = 45;
constexpr int SENSITIVITY_SLIDER_WIDTH = 200;
constexpr int SENSITIVITY_SLIDER_PICTOGRAM_WIDTH = 32;
constexpr int SENSITIVITY_SLIDER_PICTOGRAM_PADDING = 4;

TopBar::TopBar(TaskingManager &tm) : clearButton(tm), volumeSensitivitySlider(tm)
{
    addAndMakeVisible(helpButton);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(volumeSensitivitySlider);
}

void TopBar::setUpdateAvailable(bool isUpdateAvailable)
{
    updateAvailable = isUpdateAvailable;
    repaint();
}

void TopBar::paint(juce::Graphics &g)
{
    g.setColour(KHOLORS_COLOR_TEXT);
    g.setFont(sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT));
    g.drawText("KHOLORS", mainTitleArea, juce::Justification::centredRight, false);

    g.setFont(sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT));
    g.setColour(KHOLORS_COLOR_TEXT_DARKER);
    g.drawText("STATION", subTitleArea, juce::Justification::centredRight, false);

    g.setFont(sharedFonts->roboto.withHeight(VERSION_FONT_HEIGHT));
    g.drawText(GIT_DESCRIBE_VERSION, versionArea, juce::Justification::topRight, false);

    if (updateAvailable)
    {
        g.setFont(sharedFonts->roboto.withHeight(VERSION_FONT_HEIGHT - 4));
        g.setColour(KHOLORS_COLOR_NOTIF_ERROR.withAlpha(0.75f));
        auto const updateTipArea = versionArea.translated(0, -VERSION_FONT_HEIGHT + 6);
        g.drawText("NEW VERSION AVAILABLE", updateTipArea, juce::Justification::topLeft, false);
    }

    if (volumeSensitivitySlider.isVisible())
    {
        sharedSvgs->lowWaveIcon->drawWithin(g, lowWavePictogramArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);
        sharedSvgs->highWaveIcon->drawWithin(g, highWavePictogramArea.toFloat(), juce::RectanglePlacement::centred,
                                             1.0f);
    }
}

void TopBar::resized()
{
    auto const bounds = getLocalBounds();

    // title, subtitle and version on the left
    int const mainTitleWidth =
        juce::GlyphArrangement::getStringWidthInt(sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT), "KHOLORS");
    int const subTitleWidth =
        juce::GlyphArrangement::getStringWidthInt(sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT), "STATION");
    int const versionWidth = juce::GlyphArrangement::getStringWidthInt(
        sharedFonts->roboto.withHeight(VERSION_FONT_HEIGHT), GIT_DESCRIBE_VERSION);

    auto titleSpace = bounds.withTrimmedLeft(TOPBAR_SIDE_PADDING);
    mainTitleArea = titleSpace.removeFromLeft(mainTitleWidth);
    titleSpace.removeFromLeft(TITLE_MIDDLE_PADDING);
    subTitleArea = titleSpace.removeFromLeft(subTitleWidth);
    titleSpace.removeFromLeft(TITLE_VERSION_PADDING);
    versionArea = titleSpace.removeFromLeft(versionWidth);
    versionArea.setY(versionArea.getY() + VERSION_TOP_OFFSET);
    int const totalTitleWidth = versionArea.getRight();

    // buttons on the right, vertically centered
    auto buttonsArea = bounds.withTrimmedRight(TOPBAR_SIDE_PADDING);
    buttonsArea.reduce(0, (buttonsArea.getHeight() - TOPBAR_BUTTON_HEIGHT) / 2);
    helpButton.setBounds(buttonsArea.removeFromRight(HELP_BUTTON_WIDTH));
    clearButton.setBounds(buttonsArea.removeFromRight(CLEAR_BUTTON_WIDTH));

    // sensitivity slider between its two pictograms, hidden if it doesn't fit next to the title
    buttonsArea.setLeft(totalTitleWidth);
    if (buttonsArea.getWidth() < (TOPBAR_BUTTONS_PADDING + TOPBAR_BUTTONS_PADDING + SENSITIVITY_SLIDER_PICTOGRAM_WIDTH +
                                  SENSITIVITY_SLIDER_PICTOGRAM_PADDING + SENSITIVITY_SLIDER_WIDTH +
                                  SENSITIVITY_SLIDER_PICTOGRAM_PADDING + SENSITIVITY_SLIDER_PICTOGRAM_WIDTH))
    {
        volumeSensitivitySlider.setVisible(false);
        return;
    }
    volumeSensitivitySlider.setVisible(true);

    buttonsArea.removeFromRight(SENSITIVITY_SLIDER_PICTOGRAM_PADDING);
    auto const highWaveColumn = buttonsArea.removeFromRight(SENSITIVITY_SLIDER_PICTOGRAM_WIDTH);
    buttonsArea.removeFromRight(SENSITIVITY_SLIDER_PICTOGRAM_PADDING);
    auto const sliderArea = buttonsArea.removeFromRight(SENSITIVITY_SLIDER_WIDTH);
    buttonsArea.removeFromRight(SENSITIVITY_SLIDER_PICTOGRAM_PADDING);
    auto const lowWaveColumn = buttonsArea.removeFromRight(SENSITIVITY_SLIDER_PICTOGRAM_WIDTH);
    volumeSensitivitySlider.setBounds(sliderArea);

    // pictograms use the full bar height, as they are centered vertically when drawn
    highWavePictogramArea = highWaveColumn.withY(bounds.getY()).withHeight(bounds.getHeight());
    lowWavePictogramArea = lowWaveColumn.withY(bounds.getY()).withHeight(bounds.getHeight());
}
