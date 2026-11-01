#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

inline constexpr int CARD_PADDING = 12;
inline constexpr float CARD_CORNER_RADIUS = 10.0f;
inline constexpr int CARD_HEADER_HEIGHT =
    29; /**< from the card top to the line below the title, which is centred in it */

/**
 * @brief A rounded panel with a lighter background and a thin border that
 * groups content in the dashboard, with a dimmed title header separated from
 * the content by a line. The content fills the padded area below the header,
 * with an optional fixed-width side component on its left (eg: a graph scale).
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

  private:
    juce::Component &contentComponent;
    juce::Component *sideComponent;
    int sideComponentWidth;
    juce::String title;
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
    int titleBaseline; /**< whole-pixel baseline that centres the title letters in the header */

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Card)
};
