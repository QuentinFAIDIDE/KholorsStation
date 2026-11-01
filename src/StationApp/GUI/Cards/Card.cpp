#include "Card.h"

#include "GUIToolkit/Consts.h"

constexpr float CHEVRON_SIZE = 8.0f;
constexpr float CHEVRON_THICKNESS = 1.5f;

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

    titleBaseline = getCentredBaseline(sharedFonts->dimmedLabelFont, title, CARD_HEADER_HEIGHT);
}

int Card::getCentredBaseline(const juce::Font &font, const juce::String &text, int areaHeight)
{
    // centre the drawn letters, not the font ascent and descent
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText(font, text, 0.0f, 0.0f);
    juce::Path lettersOutline;
    glyphs.createPath(lettersOutline);
    auto const lettersBounds = lettersOutline.getBounds(); // relative to a baseline at y = 0
    return juce::roundToInt(((static_cast<float>(areaHeight) - lettersBounds.getHeight()) / 2.0f) -
                            lettersBounds.getY());
}

void Card::setFooterText(const juce::String &text)
{
    footerText = text.toUpperCase();
    footerBaseline =
        footerText.isEmpty() ? 0 : getCentredBaseline(sharedFonts->dimmedLabelFont, footerText, CARD_FOOTER_HEIGHT);
    resized();
    repaint();
}

int Card::getHeightForContent(int contentHeight)
{
    // header, its 1px line, then the content with padding below the line
    return CARD_HEADER_HEIGHT + 1 + (2 * CARD_PADDING) + contentHeight;
}

int Card::getCollapsedHeight()
{
    return CARD_HEADER_HEIGHT;
}

void Card::setCollapsible(bool shouldBeCollapsible)
{
    collapsible = shouldBeCollapsible;
    if (collapsible && headerClickArea == nullptr)
    {
        headerClickArea = std::make_unique<HeaderClickArea>(*this);
        addAndMakeVisible(headerClickArea.get());
        resized();
    }
    if (headerClickArea != nullptr)
    {
        headerClickArea->setVisible(collapsible);
    }
    if (!collapsible)
    {
        setCollapsed(false);
    }
    repaint();
}

void Card::setCollapsed(bool shouldBeCollapsed)
{
    if (collapsed == shouldBeCollapsed)
    {
        return;
    }
    collapsed = shouldBeCollapsed;

    // Note that hiding an OpenGL graph closes its OpenGL context, which is recreated when it's shown again.
    contentComponent.setVisible(!collapsed);
    if (sideComponent != nullptr)
    {
        sideComponent->setVisible(!collapsed);
    }
    repaint();

    if (onCollapsedChanged)
    {
        onCollapsedChanged();
    }
}

bool Card::isCollapsed() const
{
    return collapsed;
}

Card::HeaderClickArea::HeaderClickArea(Card &c) : card(c)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void Card::HeaderClickArea::mouseUp(const juce::MouseEvent &e)
{
    // only a click released over the header toggles, not a drag that ends elsewhere
    if (e.mouseWasClicked() && isMouseOver())
    {
        card.setCollapsed(!card.isCollapsed());
    }
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

    if (collapsible)
    {
        drawCollapseChevron(g);
    }

    if (footerText.isNotEmpty() && !collapsed)
    {
        int const footerTop = getHeight() - CARD_PADDING - CARD_FOOTER_HEIGHT;
        g.setColour(KHOLORS_COLOR_DIMMED_LABEL);
        g.setFont(sharedFonts->dimmedLabelFont);
        g.drawSingleLineText(footerText, CARD_PADDING, footerTop + footerBaseline);
    }

    // line below the header, with the same side padding as the content
    if (!collapsed)
    {
        g.setColour(KHOLORS_COLOR_HEADER_LINE);
        g.fillRect(CARD_PADDING, CARD_HEADER_HEIGHT, getWidth() - (2 * CARD_PADDING), 1);
    }
}

void Card::drawCollapseChevron(juce::Graphics &g) const
{
    // pointing down when the card is expanded, right when it's collapsed
    auto const area = juce::Rectangle<float>(CHEVRON_SIZE, CHEVRON_SIZE)
                          .withCentre({static_cast<float>(getWidth() - CARD_PADDING) - (CHEVRON_SIZE / 2.0f),
                                       static_cast<float>(CARD_HEADER_HEIGHT) / 2.0f});
    juce::Path chevron;
    if (collapsed)
    {
        chevron.startNewSubPath(area.getX() + (CHEVRON_SIZE / 4.0f), area.getY());
        chevron.lineTo(area.getRight() - (CHEVRON_SIZE / 4.0f), area.getCentreY());
        chevron.lineTo(area.getX() + (CHEVRON_SIZE / 4.0f), area.getBottom());
    }
    else
    {
        chevron.startNewSubPath(area.getX(), area.getY() + (CHEVRON_SIZE / 4.0f));
        chevron.lineTo(area.getCentreX(), area.getBottom() - (CHEVRON_SIZE / 4.0f));
        chevron.lineTo(area.getRight(), area.getY() + (CHEVRON_SIZE / 4.0f));
    }
    g.setColour(KHOLORS_COLOR_DIMMED_LABEL);
    g.strokePath(chevron,
                 juce::PathStrokeType(CHEVRON_THICKNESS, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void Card::resized()
{
    if (headerClickArea != nullptr)
    {
        headerClickArea->setBounds(getLocalBounds().withHeight(CARD_HEADER_HEIGHT));
    }

    auto contentBounds = getLocalBounds().withTrimmedTop(CARD_HEADER_HEIGHT + 1).reduced(CARD_PADDING);
    if (footerText.isNotEmpty())
    {
        contentBounds.removeFromBottom(CARD_FOOTER_HEIGHT);
    }
    if (collapsed)
    {
        // The hidden content keeps its last height, but its width follows the card as
        // the dashboard relies on the graphs width (eg: for autoscrolling).
        contentBounds = contentBounds.withHeight(contentComponent.getHeight());
    }
    if (sideComponent != nullptr)
    {
        sideComponent->setBounds(contentBounds.removeFromLeft(sideComponentWidth));
    }
    contentComponent.setBounds(contentBounds);
}
