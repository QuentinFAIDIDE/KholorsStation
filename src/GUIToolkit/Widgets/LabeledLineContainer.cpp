#include "LabeledLineContainer.h"

#include "../Consts.h"

LabeledLineContainer::LabeledLineContainer(std::string lab, std::shared_ptr<juce::Component> widget, int maxLabWidth,
                                           int contentWidthDesired)
    : content(widget), label(lab), maxLabelWidth(maxLabWidth), contentWidth(contentWidthDesired)
{
    addAndMakeVisible(*content);
}

std::shared_ptr<juce::Component> LabeledLineContainer::getContent()
{
    return content;
}

void LabeledLineContainer::paint(juce::Graphics &g)
{
    auto bounds = g.getClipBounds();
    g.setColour(COLOR_BACKGROUND);
    g.fillRect(bounds);

    if (isMouseOverOrDragging(true))
    {
        g.setColour(COLOR_HIGHLIGHT);
    }
    else
    {
        g.setColour(COLOR_SEPARATOR_LINE);
    }

    g.drawRect(bounds);

    g.setFont(juce::Font(DEFAULT_FONT_SIZE));

    if (!isMouseOverOrDragging(true))
    {
        g.setColour(COLOR_UNITS);
    }

    g.drawText(label, labelLocalBounds, juce::Justification::centredLeft, true);
}

void LabeledLineContainer::resized()
{
    auto bounds = getLocalBounds().reduced(LABELED_LINE_CONTAINER_PADDING, 0);

    contentLocalBounds = bounds.removeFromRight(contentWidth);
    labelLocalBounds = bounds;

    // maximum value for the
    if (labelLocalBounds.getWidth() > maxLabelWidth)
    {
        labelLocalBounds = labelLocalBounds.removeFromLeft(maxLabelWidth);
    }

    content->setBounds(contentLocalBounds);
}