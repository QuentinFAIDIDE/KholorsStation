#include "Card.h"

#include "GUIToolkit/Consts.h"

Card::Card(const juce::String &cardTitle, juce::Component &content, juce::Component *side, int sideWidth)
    : contentComponent(content), sideComponent(side), sideComponentWidth(sideWidth), title(cardTitle.toUpperCase())
{
    // let mouse events the content doesn't take go through to the parent (the dashboard
    // handles panning, zooming and the graph cursor)
    setInterceptsMouseClicks(false, true);

    addAndMakeVisible(contentComponent);
    if (sideComponent != nullptr)
    {
        addAndMakeVisible(sideComponent);
    }

    // Centre the drawn letters (not the font ascent and descent) in the header, and round
    // the baseline here: if it's left fractional, rounding at render time depends on the
    // card position and titles end up 1px apart from one card to another.
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(sharedFonts->dimmedLabelFont, title, 0.0f, 0.0f);
    juce::Path lettersOutline;
    glyphs.createPath(lettersOutline);
    auto const lettersBounds = lettersOutline.getBounds(); // relative to a baseline at y = 0
    titleBaseline = juce::roundToInt(((static_cast<float>(CARD_HEADER_HEIGHT) - lettersBounds.getHeight()) / 2.0f) -
                                     lettersBounds.getY());
}

int Card::getHeightForContent(int contentHeight)
{
    // header, its 1px line, then the content with padding below the line
    return CARD_HEADER_HEIGHT + 1 + (2 * CARD_PADDING) + contentHeight;
}

void Card::paint(juce::Graphics &g)
{
    // inset by half a pixel so that the 1px border is drawn on whole pixels
    auto const cardBounds = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(KHOLORS_COLOR_CARD_BACKGROUND);
    g.fillRoundedRectangle(cardBounds, CARD_CORNER_RADIUS);
    g.setColour(KHOLORS_COLOR_CARD_BORDER);
    g.drawRoundedRectangle(cardBounds, CARD_CORNER_RADIUS, 1.0f);

    // dimmed title, aligned with the content and vertically centred in the header
    g.setColour(KHOLORS_COLOR_DIMMED_LABEL);
    g.setFont(sharedFonts->dimmedLabelFont);
    g.drawSingleLineText(title, CARD_PADDING, titleBaseline);

    // line below the header, with the same side padding as the content
    g.setColour(KHOLORS_COLOR_HEADER_LINE);
    g.fillRect(CARD_PADDING, CARD_HEADER_HEIGHT, getWidth() - (2 * CARD_PADDING), 1);
}

void Card::resized()
{
    auto contentBounds = getLocalBounds().withTrimmedTop(CARD_HEADER_HEIGHT + 1).reduced(CARD_PADDING);
    if (sideComponent != nullptr)
    {
        sideComponent->setBounds(contentBounds.removeFromLeft(sideComponentWidth));
    }
    contentComponent.setBounds(contentBounds);
}
