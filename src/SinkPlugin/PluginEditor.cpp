#include "PluginEditor.h"
#include "GUIToolkit/Consts.h"
#include "GUIToolkit/GUIData.h"
#include "GUIToolkit/Widgets/ColorPicker.h"
#include "PluginProcessor.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"

#define HEADER_HEIGHT 65
#define HEADER_INNER_PADDING 16
#define LOGO_WIDTH 120
#define APP_NAME_FONT_HEIGHT 42
#define APP_WIDTH 600
#define APP_HEIGHT 240
#define COLOR_AREA_WIDTH 250
#define SECTIONS_HEADER_HEIGHT 40
#define SECTIONS_HEADER_TITLE_FONT_SIZE 21

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p, TaskingManager &tm,
                                                                 juce::Colour currentlySelectedColor)
    : AudioProcessorEditor(&p), processorRef(p), taskManager(tm), colorPicker("track-color-picker", taskManager)
{
    typeface = juce::Typeface::createSystemTypefaceFor(GUIData::RobotoRegular_ttf, GUIData::RobotoRegular_ttfSize);
    appLookAndFeel.setDefaultSansSerifTypeface(typeface);
    juce::LookAndFeel::setDefaultLookAndFeel(&appLookAndFeel);
    setLookAndFeel(&appLookAndFeel);

    setSize(APP_WIDTH, APP_HEIGHT);

    colorPicker.setColour(currentlySelectedColor.getRed(), currentlySelectedColor.getGreen(),
                          currentlySelectedColor.getBlue());

    addAndMakeVisible(colorPicker);
    addAndMakeVisible(textEntry);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{

    // (Our component is opaque, so we must completely fill the background with a
    // solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    drawHeader(g);

    auto boundsWithoutHeaders = getLocalBounds().withTrimmedTop(HEADER_HEIGHT).withTrimmedBottom(HEADER_INNER_PADDING);
    auto leftArea = boundsWithoutHeaders.removeFromLeft(COLOR_AREA_WIDTH);
    auto rightArea = boundsWithoutHeaders;
    leftArea.reduce(HEADER_INNER_PADDING, 0);
    rightArea.removeFromRight(HEADER_INNER_PADDING);

    auto leftAreaHeader = leftArea.removeFromTop(SECTIONS_HEADER_HEIGHT);
    auto rightAreaHeader = rightArea.removeFromTop(SECTIONS_HEADER_HEIGHT);

    g.setColour(COLOR_TEXT);
    g.setFont(juce::Font(SECTIONS_HEADER_TITLE_FONT_SIZE));
    g.drawText(TRANS("Track Color"), leftAreaHeader, juce::Justification::centredLeft, false);
    g.drawText(TRANS("Track Infos"), rightAreaHeader, juce::Justification::centredLeft, false);
}

void AudioPluginAudioProcessorEditor::drawHeader(juce::Graphics &g)
{
    int middlePadding = 5;
    auto topHeaderArea = getLocalBounds().removeFromTop(HEADER_HEIGHT).reduced(HEADER_INNER_PADDING, 0);
    auto logoArea = topHeaderArea.removeFromRight(LOGO_WIDTH);
    sharedSvgs->artifaktNdLogo->drawWithin(g, logoArea.toFloat(), juce::RectanglePlacement::centred, 1.0f);

    int mainTitleWidth = sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT).getStringWidth("KHOLORS II");
    int subTitleWidth = sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT).getStringWidth("SINK");
    int totalWidth = mainTitleWidth + middlePadding + subTitleWidth;

    auto titleArea = topHeaderArea.removeFromLeft(totalWidth);
    auto boldTitleArea = titleArea.removeFromLeft(mainTitleWidth);
    titleArea.removeFromLeft(middlePadding);

    g.setColour(COLOR_TEXT);
    g.setFont(sharedFonts->robotoBlack.withHeight(APP_NAME_FONT_HEIGHT));
    g.drawText("KHOLORS II", boldTitleArea, juce::Justification::centred, false);

    g.setFont(sharedFonts->roboto.withHeight(APP_NAME_FONT_HEIGHT));
    g.setColour(COLOR_TEXT_DARKER);
    g.drawText("SINK", titleArea, juce::Justification::centredRight, false);
}

void AudioPluginAudioProcessorEditor::mouseDrag(const juce::MouseEvent &)
{
}

void AudioPluginAudioProcessorEditor::mouseDown(const juce::MouseEvent &)
{
}

void AudioPluginAudioProcessorEditor::mouseUp(const juce::MouseEvent &)
{
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto boundsWithoutHeaders = getLocalBounds().withTrimmedTop(HEADER_HEIGHT).withTrimmedBottom(HEADER_INNER_PADDING);
    auto leftArea = boundsWithoutHeaders.removeFromLeft(COLOR_AREA_WIDTH);
    auto rightArea = boundsWithoutHeaders;
    leftArea.reduce(HEADER_INNER_PADDING, 0);
    rightArea.removeFromRight(HEADER_INNER_PADDING);

    leftArea.removeFromTop(SECTIONS_HEADER_HEIGHT);
    rightArea.removeFromTop(SECTIONS_HEADER_HEIGHT);

    colorPicker.setBounds(leftArea);
    textEntry.setBounds(rightArea.removeFromTop(TABS_HEIGHT));
}
