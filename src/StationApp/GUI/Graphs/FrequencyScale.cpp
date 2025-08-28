#include "FrequencyScale.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include <limits>
#include <optional>
#include <vector>

FrequencyScale::FrequencyScale(NormalizedUnitTransformer &ut, float maxDrawableFreqArg)
    : unitTransformer(ut), maxDrawableFreq(maxDrawableFreqArg)
{
    setOpaque(true);
}

FrequencyScale::~FrequencyScale()
{
}

void FrequencyScale::paint(juce::Graphics &g)
{
    // fill background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // draw the "FREQUENCIES" axis title
    g.setColour(KHOLORS_COLOR_WHITE);
    drawRotatedTitle(g);

    g.setColour(KHOLORS_COLOR_UNITS);
    drawRotatedChannelNames(g);

    // generate necessary labels
    std::vector<LabelBox> labelsToDraw;
    auto labs0Hz = generateLabelBox(0, "0", "Hz");
    for (size_t i = 0; i < labs0Hz.size(); i++)
    {
        labelsToDraw.push_back(labs0Hz[i]);
    }
    auto labs100Hz = generateLabelBox(100, "100", "Hz");
    for (size_t i = 0; i < labs100Hz.size(); i++)
    {
        labelsToDraw.push_back(labs100Hz[i]);
    }
    auto labs350Hz = generateLabelBox(350, "350", "Hz");
    for (size_t i = 0; i < labs350Hz.size(); i++)
    {
        labelsToDraw.push_back(labs350Hz[i]);
    }
    auto labs1kHz = generateLabelBox(1000, "1", "kHz");
    for (size_t i = 0; i < labs1kHz.size(); i++)
    {
        labelsToDraw.push_back(labs1kHz[i]);
    }
    auto labs35kHz = generateLabelBox(3500, "3.5", "kHz");
    for (size_t i = 0; i < labs35kHz.size(); i++)
    {
        labelsToDraw.push_back(labs35kHz[i]);
    }
    auto labs10kHz = generateLabelBox(10000, "10", "kHz");
    for (size_t i = 0; i < labs10kHz.size(); i++)
    {
        labelsToDraw.push_back(labs10kHz[i]);
    }

    drawLabels(g, labelsToDraw);
}

void FrequencyScale::drawRotatedTitle(juce::Graphics &g)
{
    // nice trick from https://forum.juce.com/t/draw-rotated-text/14695/11

    juce::GlyphArrangement ga;
    ga.addLineOfText(juce::Font(TITLE_PIXELS_HEIGHT), TRANS("Frequency").toUpperCase(), 0, 0);
    juce::Path p;
    ga.createPath(p);

    auto pathBounds = p.getBounds();

    p.applyTransform(
        juce::AffineTransform()
            .rotated(3.0f * juce::MathConstants<float>::halfPi, pathBounds.getCentreX(), pathBounds.getCentreY())
            .translated(0, getHeight() * 0.5f));

    p.applyTransform(juce::AffineTransform().translated(TITLE_PIXELS_FROM_LEFT - p.getBounds().getX(), 0));

    g.fillPath(p);
}

void FrequencyScale::drawRotatedChannelNames(juce::Graphics &g)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText(juce::Font(TITLE_PIXELS_HEIGHT), TRANS("Left Channel").toUpperCase(), 0, 0);
    juce::Path p;
    ga.createPath(p);
    auto pathBounds = p.getBounds();
    p.applyTransform(
        juce::AffineTransform()
            .rotated(3.0f * juce::MathConstants<float>::halfPi, pathBounds.getCentreX(), pathBounds.getCentreY())
            .translated(0, getHeight() * 0.25f));
    p.applyTransform(juce::AffineTransform().translated(TITLE_PIXELS_FROM_LEFT - p.getBounds().getX(), 0));
    g.fillPath(p);

    ga.clear();
    ga.addLineOfText(juce::Font(TITLE_PIXELS_HEIGHT), TRANS("Right Channel").toUpperCase(), 0, 0);
    p.clear();
    ga.createPath(p);
    pathBounds = p.getBounds();
    p.applyTransform(
        juce::AffineTransform()
            .rotated(3.0f * juce::MathConstants<float>::halfPi, pathBounds.getCentreX(), pathBounds.getCentreY())
            .translated(0, getHeight() * 0.75f));
    p.applyTransform(juce::AffineTransform().translated(TITLE_PIXELS_FROM_LEFT - p.getBounds().getX(), 0));
    g.fillPath(p);
}

std::vector<FrequencyScale::LabelBox> FrequencyScale::generateLabelBox(float frequencyHz, std::string text,
                                                                       std::string unit)
{

    // convert the frequency to a ratio of position from center
    float positionRatioLinearRange = float(frequencyHz) / maxDrawableFreq;
    float positionRatioOnScreenRange = positionRatioLinearRange;
    if (positionRatioLinearRange > std::numeric_limits<float>::epsilon() && positionRatioLinearRange < 1.0f)
    {
        positionRatioOnScreenRange = unitTransformer.transform(positionRatioLinearRange);
    }
    // ignore data that is too close to borders
    if (positionRatioOnScreenRange > MAXIMUM_DRAWABLE_LABEL_POSITION_RATIO)
    {
        return std::vector<LabelBox>();
    }

    float halfScreen = (float)getHeight() / 2.0f;
    auto upperTextBox = getLocalBounds()
                            .withY(halfScreen - (positionRatioOnScreenRange * halfScreen) - (LABEL_HEIGHT >> 1))
                            .withHeight(LABEL_HEIGHT)
                            .withTrimmedRight(LABEL_RIGHT_PADDING);
    auto lowerTextBox = getLocalBounds()
                            .withY(halfScreen + (positionRatioOnScreenRange * halfScreen) - (LABEL_HEIGHT >> 1))
                            .withHeight(LABEL_HEIGHT)
                            .withTrimmedRight(LABEL_RIGHT_PADDING);

    std::vector<LabelBox> result;

    result.emplace_back(upperTextBox, text, unit);

    if (!upperTextBox.intersects(lowerTextBox))
    {
        result.emplace_back(lowerTextBox, text, unit);
    }

    return result;
}

void FrequencyScale::drawLabels(juce::Graphics &g, std::vector<LabelBox> &labs)
{
    std::vector<juce::Rectangle<int>> drawnAreas;

    for (size_t i = 0; i < labs.size(); i++)
    {
        bool goodToDraw = true;
        // abort if it intersect already drawn Areas
        for (size_t j = 0; j < drawnAreas.size(); j++)
        {
            if (drawnAreas[j].intersects(labs[i].bounds))
            {
                goodToDraw = false;
            }
        }
        if (!goodToDraw)
        {
            continue;
        }

        juce::Font unitFont(LABEL_FONT_HEIGHT);
        juce::Font valueFont(LABEL_FONT_HEIGHT);

        // extract area to draw units
        std::string fullUnitText = " " + labs[i].unit;
        int unitAreaWidth = unitFont.getStringWidth(fullUnitText);
        auto unitDrawArea = labs[i].bounds.withLeft(unitAreaWidth);
        auto valueDrawArea = labs[i].bounds.withTrimmedRight(unitAreaWidth);

        int valueTextWidth = valueFont.getStringWidth(labs[i].text);
        // ignore drawing if there is no horizontal space left for the text
        if (valueTextWidth >= valueDrawArea.getWidth())
        {
            continue;
        }

        g.setFont(unitFont);
        g.setColour(KHOLORS_COLOR_UNITS);
        g.drawText(labs[i].unit, unitDrawArea, juce::Justification::centredRight, false);

        g.setFont(valueFont);
        g.setColour(KHOLORS_COLOR_WHITE);
        g.drawText(labs[i].text, valueDrawArea, juce::Justification::centredRight, false);

        drawnAreas.push_back(labs[i].bounds);
    }
}