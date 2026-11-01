#pragma once

#include "GUIToolkit/Consts.h"
#include "juce_graphics/juce_graphics.h"
#include <initializer_list>
#include <utility>

inline constexpr int CHANNEL_BADGE_WIDTH = 22;
inline constexpr int CHANNEL_BADGE_HEIGHT = 18;
inline constexpr float CHANNEL_BADGE_CORNER_RADIUS = 4.0f;
inline constexpr int CHANNEL_BADGE_FONT_HEIGHT = 12;
inline constexpr int CHANNEL_BADGE_MARGIN = 4; /**< space between the badge and the top or bottom of the scale */

/**
 * @brief Draw the "L" and "R" channel badges of a graph scale: left channel
 * at the top (as graphs draw it in their top half), right channel at the bottom.
 *
 * @param g graphics context of the scale component.
 * @param scaleBounds local bounds of the scale component.
 * @param font font to draw the letters with, resized to the badge font height.
 */
inline void drawChannelBadges(juce::Graphics &g, juce::Rectangle<int> scaleBounds, const juce::Font &font)
{
    auto const leftBadge = juce::Rectangle<int>(scaleBounds.getX(), scaleBounds.getY() + CHANNEL_BADGE_MARGIN,
                                                CHANNEL_BADGE_WIDTH, CHANNEL_BADGE_HEIGHT);
    auto const rightBadge = leftBadge.withY(scaleBounds.getBottom() - CHANNEL_BADGE_MARGIN - CHANNEL_BADGE_HEIGHT);

    g.setFont(font.withHeight(CHANNEL_BADGE_FONT_HEIGHT));
    for (auto const &[badge, letter] : {std::pair{leftBadge, "L"}, std::pair{rightBadge, "R"}})
    {
        g.setColour(KHOLORS_COLOR_BADGE_BACKGROUND);
        g.fillRoundedRectangle(badge.toFloat(), CHANNEL_BADGE_CORNER_RADIUS);
        g.setColour(KHOLORS_COLOR_BADGE_TEXT);
        g.drawText(letter, badge, juce::Justification::centred, false);
    }
}
