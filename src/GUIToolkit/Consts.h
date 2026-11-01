#pragma once

#include "juce_graphics/juce_graphics.h"
#include <vector>

inline const juce::Colour KHOLORS_COLOR_BLACK = juce::Colour(10, 10, 10);
inline const juce::Colour KHOLORS_COLOR_WHITE = juce::Colour(243, 243, 243);

inline const juce::Colour KHOLORS_COLOR_BACKGROUND = juce::Colour(7, 9, 14);
inline const juce::Colour KHOLORS_COLOR_CARD_BACKGROUND = juce::Colour(13, 16, 23);
inline const juce::Colour KHOLORS_COLOR_CARD_BORDER = juce::Colour(27, 33, 48);
inline const juce::Colour KHOLORS_COLOR_HEADER_LINE = juce::Colour(22, 27, 38);
inline const juce::Colour KHOLORS_COLOR_BUTTON_BORDER = juce::Colour(42, 50, 69);
inline const juce::Colour KHOLORS_COLOR_DIMMED_LABEL = juce::Colour(94, 102, 121);
inline const juce::Colour KHOLORS_COLOR_BADGE_BACKGROUND = juce::Colour(22, 27, 38);
inline const juce::Colour KHOLORS_COLOR_BADGE_TEXT = juce::Colour(140, 148, 167);
inline const juce::Colour KHOLORS_COLOR_TEXT = KHOLORS_COLOR_WHITE;
inline const juce::Colour KHOLORS_COLOR_TEXT_DARKER = juce::Colour(179, 179, 179);
inline const juce::Colour KHOLORS_COLOR_SEPARATOR_LINE = juce::Colour(80, 80, 80);
inline const juce::Colour KHOLORS_COLOR_HIGHLIGHT = juce::Colour(87, 110, 195);
inline const juce::Colour KHOLORS_COLOR_BUTTON_HIGHLIGHT = juce::Colour(194, 219, 255);
inline const juce::Colour KHOLORS_COLOR_SELECTED_BACKGROUND = juce::Colour(33, 45, 90);
inline const juce::Colour KHOLORS_COLOR_GRIDS_LEVEL_0 = juce::Colour(static_cast<juce::uint8>(31), 38, 62, 1.0f);
inline const juce::Colour KHOLORS_COLOR_GRIDS_LEVEL_1 = juce::Colour(static_cast<juce::uint8>(31), 38, 62, 1.0f);
inline const juce::Colour KHOLORS_COLOR_GRIDS_LEVEL_2 = juce::Colour(static_cast<juce::uint8>(31), 38, 62, 0.5f);
inline const juce::Colour KHOLORS_COLOR_INPUT_BACKGROUND = juce::Colour(211, 222, 222);
inline const juce::Colour KHOLORS_COLOR_INPUT_TEXT = juce::Colour(51, 51, 51);

inline const juce::Colour KHOLORS_COLOR_FREQVIEW_GRADIENT_BORDERS = KHOLORS_COLOR_CARD_BACKGROUND;
inline const juce::Colour KHOLORS_COLOR_FREQVIEW_GRADIENT_CENTER = juce::Colour(0, 17, 43);

inline const juce::Colour KHOLORS_COLOR_NOTIF_INFO = juce::Colour(64, 86, 165);
inline const juce::Colour KHOLORS_COLOR_NOTIF_WARN = juce::Colour(155, 95, 49);
inline const juce::Colour KHOLORS_COLOR_NOTIF_ERROR = juce::Colour(198, 66, 100);

// these ones are lighter and more appropriate for text
inline const juce::Colour KHOLORS_COLOR_TEXT_STATUS_OK = juce::Colour(94, 219, 103);
inline const juce::Colour KHOLORS_COLOR_TEXT_WARN = juce::Colour(219, 157, 94);
inline const juce::Colour KHOLORS_COLOR_TEXT_STATUS_ERROR = juce::Colour(219, 94, 94);

inline const juce::Colour KHOLORS_COLOR_UNITS = KHOLORS_COLOR_TEXT_DARKER.withAlpha(0.5f);

inline constexpr int KHOLORS_DEFAULT_FONT_SIZE = 17;
inline constexpr int KHOLORS_SMALLER_FONT_SIZE = 13;


inline constexpr int KHOLORS_TAB_BUTTONS_INNER_PADDING = 42;

inline constexpr int KHOLORS_TAB_PADDING = 4;
inline constexpr int KHOLORS_TAB_SECTIONS_MARGINS = 5;
inline constexpr int KHOLORS_TABS_HEIGHT = 34;

inline constexpr int KHOLORS_TAB_HIGHLIGHT_LINE_WIDTH = 2;

inline const std::vector<juce::Colour> KHOLORS_COLOR_PALETTE = {
    juce::Colour::fromString("ff399898"), juce::Colour::fromString("ff3c7a98"), juce::Colour::fromString("ff4056a5"),
    juce::Colour::fromString("ff663fc6"), juce::Colour::fromString("ff8c2fd3"), juce::Colour::fromString("ffcc34c9"),
    juce::Colour::fromString("ffc2448a"), juce::Colour::fromString("ffc64264"), juce::Colour::fromString("ff9b5f31"),
    juce::Colour::fromString("ffc5a536"), juce::Colour::fromString("ffbec83d"), juce::Colour::fromString("ff97c43e"),
    juce::Colour::fromString("ff49c479"),
};
