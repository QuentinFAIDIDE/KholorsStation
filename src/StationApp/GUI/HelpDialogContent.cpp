#include "HelpDialogContent.h"
#include "GUIToolkit/Consts.h"

#define TIP_LINE_HEIGHT 40
#define TIP_LEFT_PADDING 22
#define TOP_PADDING 22

#define HELP_TIP "Click the help button to view this list of tips."
#define ADD_VST_TIP "Add the 'KholorsSink' VST plugin to your DAW tracks to draw their spectrum and volumes here."
#define MOUSE_MOVE_TIP "Middle-click and drag to zoom and pan the view."
#define AUTOMATION_TIP "The view automatically follows your DAW's playhead, beat signature and BPM."
#define FOCUS_TRACK_TIP "Hover over a track in the list on the right to highlight its spectrum."
#define PRECISION_TIP                                                                                                  \
    "Use the top slider to adjust spectrum detail. Note that lower frequencies are inherently less precise."
#define FREQ_TIP "Hover over the spectrum to see frequency and time info in the bottom-right corner."
#define VOLUME_TIP "For consistent color intensity, adjust track gain before the KholorsSink VST plugin."
#define EXPLAIN_TIP "Graphs show frequencies (upper) and stacked volumes (lower) over time."
#define EXPLAIN_TIP_2 "Each graph has right channel on top half and left channel on bottom half."

HelpDialogContent::HelpDialogContent()
{
    tipsAndText.push_back(std::make_pair(&helpTipPos, HELP_TIP));
    tipsAndText.push_back(std::make_pair(&addVstTipPos, ADD_VST_TIP));
    tipsAndText.push_back(std::make_pair(&mouseMoveTipPos, MOUSE_MOVE_TIP));
    tipsAndText.push_back(std::make_pair(&automationTipPos, AUTOMATION_TIP));
    tipsAndText.push_back(std::make_pair(&explainTipPos, EXPLAIN_TIP));
    tipsAndText.push_back(std::make_pair(&explainTipPos2, EXPLAIN_TIP_2));
    tipsAndText.push_back(std::make_pair(&focusTrackTipPos, FOCUS_TRACK_TIP));
    tipsAndText.push_back(std::make_pair(&precisionTipPos, PRECISION_TIP));
    tipsAndText.push_back(std::make_pair(&freqTipPos, FREQ_TIP));
    tipsAndText.push_back(std::make_pair(&volumeTipPos, VOLUME_TIP));
    font = sharedFonts->roboto.withHeight(KHOLORS_DEFAULT_FONT_SIZE + 1);
    int maxWidth = getMaxWidthFromTexts();
    int maxHeight = (2 * TOP_PADDING) + (tipsAndText.size() * TIP_LINE_HEIGHT);
    // another line height is added here to compensate for the width of the numbers boxes
    setSize(maxWidth + TIP_LINE_HEIGHT, maxHeight);
}

HelpDialogContent::~HelpDialogContent()
{
}

int HelpDialogContent::getMaxWidthFromTexts()
{
    int maxWidth = 0;
    for (size_t i = 0; i < tipsAndText.size(); i++)
    {
        int width = juce::GlyphArrangement::getStringWidthInt(font, tipsAndText[i].second);
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
    g.setFont(font);
    for (size_t i = 0; i < tipsAndText.size(); i++)
    {
        g.setColour(KHOLORS_COLOR_TEXT_DARKER);
        auto lineArea = *tipsAndText[i].first;
        auto numberArea = lineArea.removeFromLeft(TIP_LINE_HEIGHT);
        g.drawText(std::to_string(i + 1), numberArea, juce::Justification::centred);
        g.setColour(KHOLORS_COLOR_TEXT);
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
