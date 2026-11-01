#include "FrequencyScale.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/Graphs/ChannelBadge.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include <limits>
#include <optional>
#include <vector>

constexpr double MAXIMUM_DRAWABLE_LABEL_POSITION_RATIO = 0.93;
constexpr int LABEL_HEIGHT = 16;
constexpr auto LABEL_FONT_HEIGHT = (KHOLORS_DEFAULT_FONT_SIZE - 2);
constexpr int LABEL_RIGHT_PADDING = 10;

FrequencyScale::FrequencyScale(NormalizedUnitTransformer &ut, float maxDrawableFreqArg)
    : unitTransformer(ut), maxDrawableFreq(maxDrawableFreqArg)
{
    setOpaque(true);
}

FrequencyScale::~FrequencyScale() = default;

void FrequencyScale::paint(juce::Graphics &g)
{
    // fill background
    g.fillAll(KHOLORS_COLOR_CARD_BACKGROUND);

    drawChannelBadges(g, getLocalBounds(), sharedFonts->monospaceFont);

    // generate necessary labels
    std::vector<LabelBox> labelsToDraw;
    auto const labs0Hz = generateLabelBox(0, "0", "Hz");
    labelsToDraw.reserve(labs0Hz.size());
    for (const auto &i : labs0Hz)
    {
        labelsToDraw.push_back(i);
    }
    auto const labs100Hz = generateLabelBox(100, "100", "Hz");
    for (const auto &i : labs100Hz)
    {
        labelsToDraw.push_back(i);
    }
    auto const labs350Hz = generateLabelBox(350, "350", "Hz");
    for (const auto &i : labs350Hz)
    {
        labelsToDraw.push_back(i);
    }
    auto const labs1kHz = generateLabelBox(1000, "1", "kHz");
    for (const auto &i : labs1kHz)
    {
        labelsToDraw.push_back(i);
    }
    auto const labs35kHz = generateLabelBox(3500, "3.5", "kHz");
    for (const auto &i : labs35kHz)
    {
        labelsToDraw.push_back(i);
    }
    auto const labs10kHz = generateLabelBox(10000, "10", "kHz");
    for (const auto &i : labs10kHz)
    {
        labelsToDraw.push_back(i);
    }

    drawLabels(g, labelsToDraw);
}

std::vector<FrequencyScale::LabelBox> FrequencyScale::generateLabelBox(float frequencyHz, std::string text,
                                                                       std::string unit)
{

    // convert the frequency to a ratio of position from center
    float const positionRatioLinearRange = frequencyHz / maxDrawableFreq;
    float positionRatioOnScreenRange = positionRatioLinearRange;
    if (positionRatioLinearRange > std::numeric_limits<float>::epsilon() && positionRatioLinearRange < 1.0f)
    {
        positionRatioOnScreenRange = unitTransformer.transform(positionRatioLinearRange);
    }
    // ignore data that is too close to borders
    if (positionRatioOnScreenRange > MAXIMUM_DRAWABLE_LABEL_POSITION_RATIO)
    {
        return {};
    }

    float const halfScreen = static_cast<float>(getHeight()) / 2.0f;
    auto const upperTextBox = getLocalBounds()
                                  .withY(halfScreen - (positionRatioOnScreenRange * halfScreen) - (LABEL_HEIGHT >> 1))
                                  .withHeight(LABEL_HEIGHT)
                                  .withTrimmedRight(LABEL_RIGHT_PADDING);
    auto const lowerTextBox = getLocalBounds()
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

    for (auto const &lab : labs)
    {
        bool goodToDraw = true;
        // abort if it intersect already drawn Areas
        for (auto const drawnArea : drawnAreas)
        {
            if (drawnArea.intersects(lab.bounds))
            {
                goodToDraw = false;
            }
        }
        if (!goodToDraw)
        {
            continue;
        }

        juce::Font const unitFont(LABEL_FONT_HEIGHT);
        juce::Font const valueFont(LABEL_FONT_HEIGHT);

        // extract area to draw units
        std::string const fullUnitText = " " + lab.unit;
        int const unitAreaWidth = juce::GlyphArrangement::getStringWidthInt(unitFont, fullUnitText);
        auto const unitDrawArea = lab.bounds.withLeft(unitAreaWidth);
        auto const valueDrawArea = lab.bounds.withTrimmedRight(unitAreaWidth);

        int const valueTextWidth = juce::GlyphArrangement::getStringWidthInt(valueFont, lab.text);
        // ignore drawing if there is no horizontal space left for the text
        if (valueTextWidth >= valueDrawArea.getWidth())
        {
            continue;
        }

        g.setFont(unitFont);
        g.setColour(KHOLORS_COLOR_UNITS);
        g.drawText(lab.unit, unitDrawArea, juce::Justification::centredRight, false);

        g.setFont(valueFont);
        g.setColour(KHOLORS_COLOR_WHITE);
        g.drawText(lab.text, valueDrawArea, juce::Justification::centredRight, false);

        drawnAreas.push_back(lab.bounds);
    }
}
