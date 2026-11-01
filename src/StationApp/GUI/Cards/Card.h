#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <functional>
#include <memory>

inline constexpr int CARD_PADDING = 10;
inline constexpr float CARD_CORNER_RADIUS = 10.0f;
inline constexpr int CARD_FOOTER_HEIGHT = 16; /**< footer text area, at the bottom of the padded content area */
inline constexpr int CARD_HEADER_HEIGHT =
    25; /**< from the card top to the line below the title, which is centred in it */

/**
 * @brief A rounded panel with a lighter background and a thin border that
 * groups content in the dashboard, with a dimmed title header separated from
 * the content by a line. The content fills the padded area below the header,
 * with an optional fixed-width side component on its left (eg: a graph scale).
 * Collapsible cards can be folded to their header by clicking it.
 *
 * The content and side components are owned by the caller, which must keep
 * them alive as long as the card.
 */
class Card final : public juce::Component
{
  public:
    /**
     * @param cardTitle title shown in the header, in uppercase.
     * @param content component filling the card below the header.
     * @param sideComponent optional component shown on the left of the content.
     * @param sideComponentWidth width of the side component, ignored if there is none.
     */
    Card(const juce::String &cardTitle, juce::Component &content, juce::Component *sideComponent = nullptr,
         int sideComponentWidth = 0);

    void paint(juce::Graphics &g) override;
    void resized() override;

    /**
     * @brief Height of a card whose content area is contentHeight pixels high.
     */
    static int getHeightForContent(int contentHeight);

    /**
     * @brief Height of a collapsed card, which only shows its header.
     */
    static int getCollapsedHeight();

    /**
     * @brief Allow collapsing the card by clicking its header.
     */
    void setCollapsible(bool shouldBeCollapsible);

    /**
     * @brief Fold the card to its header (hiding its content) or unfold it.
     * The owner is told through onCollapsedChanged so that it can resize the card.
     */
    void setCollapsed(bool shouldBeCollapsed);
    bool isCollapsed() const;

    std::function<void()> onCollapsedChanged; /**< called after the card was collapsed or expanded */

    /**
     * @brief Show a dimmed text at the bottom of the card, below the content (eg: a usage hint).
     * An empty text removes the footer.
     */
    void setFooterText(const juce::String &text);

  private:
    /**
     * @brief Invisible component over the header that toggles the collapsed state on click,
     * as the card itself lets mouse events go through to its parent.
     */
    class HeaderClickArea final : public juce::Component
    {
      public:
        explicit HeaderClickArea(Card &c);
        void mouseUp(const juce::MouseEvent &e) override;

      private:
        Card &card;
    };

    void drawCollapseChevron(juce::Graphics &g) const;

    /**
     * @brief Whole-pixel baseline that vertically centres the drawn letters of a text in an area of the given
     * height. If left fractional, rounding at render time depends on the card position and texts end up 1px apart
     * from one card to another.
     */
    static int getCentredBaseline(const juce::Font &font, const juce::String &text, int areaHeight);

    juce::Component &contentComponent;
    juce::Component *sideComponent;
    int sideComponentWidth;
    juce::String title;
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
    int titleBaseline; /**< whole-pixel baseline that centres the title letters in the header */
    juce::String footerText;
    int footerBaseline{0}; /**< baseline of the footer text, from the top of the footer area */
    bool collapsible{false};
    bool collapsed{false};
    std::unique_ptr<HeaderClickArea> headerClickArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Card)
};
