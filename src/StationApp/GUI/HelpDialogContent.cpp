#include "HelpDialogContent.h"
#include "GUIToolkit/Consts.h"

#define TIP_LINE_HEIGHT 40
#define TIP_LEFT_PADDING 16
#define TOP_PADDING 22

#define HELP_TIP "Click the help button to view this list of tips."
#define ADD_VST_TIP "Add the 'KholorsSink' VST plugin to your DAW tracks to visualize them here."
#define MOUSE_MOVE_TIP "Middle-click and drag up/down to zoom, or left/right to pan the view."
#define AUTOMATION_TIP "The view automatically follows your DAW's playhead, beat signature and BPM."
#define FOCUS_TRACK_TIP "Hover over a track in the list on the right to highlight its spectrum."
#define PRECISION_TIP                                                                                                  \
    "Use the top slider to adjust spectrum detail. Note that lower frequencies are inherently less precise."
#define FREQ_TIP "Hover over the spectrum to see frequency and time info in the bottom-right corner."
#define VOLUME_TIP "For consistent color intensity, adjust track gain before the KholorsSink VST plugin."

HelpDialogContent::HelpDialogContent()
{
    tipsAndText.push_back(std::make_pair(&helpTipPos, juce::String(HELP_TIP).toUpperCase().toStdString()));
    tipsAndText.push_back(std::make_pair(&addVstTipPos, juce::String(ADD_VST_TIP).toUpperCase().toStdString()));
    tipsAndText.push_back(std::make_pair(&mouseMoveTipPos, juce::String(MOUSE_MOVE_TIP).toUpperCase().toStdString()));
    tipsAndText.push_back(std::make_pair(&automationTipPos, juce::String(AUTOMATION_TIP).toUpperCase().toStdString()));
    tipsAndText.push_back(std::make_pair(&focusTrackTipPos, juce::String(FOCUS_TRACK_TIP).toUpperCase().toStdString()));
    tipsAndText.push_back(std::make_pair(&precisionTipPos, juce::String(PRECISION_TIP).toUpperCase().toStdString()));
    tipsAndText.push_back(std::make_pair(&freqTipPos, juce::String(FREQ_TIP).toUpperCase().toStdString()));
    tipsAndText.push_back(std::make_pair(&volumeTipPos, juce::String(VOLUME_TIP).toUpperCase().toStdString()));
    font = sharedFonts->robotoBold.withHeight(KHOLORS_DEFAULT_FONT_SIZE);
    int maxWidth = getMaxWidthFromTexts();
    int maxHeight = (2 * TOP_PADDING) + (tipsAndText.size() * TIP_LINE_HEIGHT);
    setSize(maxWidth, maxHeight);
}

HelpDialogContent::~HelpDialogContent()
{
}

int HelpDialogContent::getMaxWidthFromTexts()
{
    int maxWidth = 0;
    for (size_t i = 0; i < tipsAndText.size(); i++)
    {
        int width = font.getStringWidth(tipsAndText[i].second);
        if (width > maxWidth)
        {
            maxWidth = width;
        }
    }
    // the padding applies to the left and right, and the numbered box for tips
    // is TIP-LINE-HEIGHT pixels wide.
    return maxWidth + (2 * TIP_LEFT_PADDING) + TIP_LINE_HEIGHT;
}

void HelpDialogContent::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(KHOLORS_COLOR_TEXT_DARKER);
    g.setFont(font);
    for (size_t i = 0; i < tipsAndText.size(); i++)
    {
        auto lineArea = *tipsAndText[i].first;
        auto numberArea = lineArea.removeFromLeft(TIP_LINE_HEIGHT);
        g.drawText(std::to_string(i + 1), numberArea, juce::Justification::centred);
        g.drawText(tipsAndText[i].second, lineArea, juce::Justification::centredLeft);
    }
}

void HelpDialogContent::resized()
{
    auto area = getLocalBounds();
    area.removeFromLeft(TIP_LEFT_PADDING);
    area.removeFromTop(TOP_PADDING);
    for (size_t i = 0; i < tipsAndText.size(); i++)
    {
        *tipsAndText[i].first = area.removeFromTop(TIP_LINE_HEIGHT);
    }
}
