#pragma once

#define KHOLORS_COLOR_BACKGROUND juce::Colour(0, 17, 43)
#define KHOLORS_COLOR_TEXT KHOLORS_COLOR_WHITE
#define KHOLORS_COLOR_TEXT_DARKER juce::Colour(179, 179, 179)
#define KHOLORS_COLOR_SEPARATOR_LINE juce::Colour(80, 80, 80)
#define KHOLORS_COLOR_HIGHLIGHT juce::Colour(87, 110, 195)
#define KHOLORS_COLOR_SELECTED_BACKGROUND juce::Colour(33, 45, 90)
#define KHOLORS_COLOR_GRIDS_LEVEL_0 juce::Colour(juce::uint8(31), 38, 62, 1.0f)
#define KHOLORS_COLOR_GRIDS_LEVEL_1 juce::Colour(juce::uint8(31), 38, 62, 1.0f)
#define KHOLORS_COLOR_GRIDS_LEVEL_2 juce::Colour(juce::uint8(31), 38, 62, 0.5f)
#define KHOLORS_COLOR_INPUT_BACKGROUND juce::Colour(211, 222, 222)
#define KHOLORS_COLOR_INPUT_TEXT juce::Colour(51, 51, 51)

#define KHOLORS_COLOR_FREQVIEW_GRADIENT_BORDERS juce::Colour(0, 17, 43)
#define KHOLORS_COLOR_FREQVIEW_GRADIENT_CENTER juce::Colour(0, 17, 43)

#define KHOLORS_COLOR_NOTIF_INFO juce::Colour(64, 86, 165)
#define KHOLORS_COLOR_NOTIF_WARN juce::Colour(155, 95, 49)
#define KHOLORS_COLOR_NOTIF_ERROR juce::Colour(198, 66, 100)

// these ones are lighter and more appropriate for text
#define KHOLORS_COLOR_TEXT_STATUS_OK juce::Colour(94, 219, 103)
#define KHOLORS_COLOR_TEXT_WARN juce::Colour(219, 157, 94)
#define KHOLORS_COLOR_TEXT_STATUS_ERROR juce::Colour(219, 94, 94)

#define KHOLORS_COLOR_UNITS KHOLORS_COLOR_TEXT_DARKER.withAlpha(0.5f)

#define KHOLORS_COLOR_BLACK juce::Colour(10, 10, 10)
#define KHOLORS_COLOR_WHITE juce::Colour(243, 243, 243)

#define KHOLORS_DEFAULT_FONT_SIZE 17
#define KHOLORS_SMALLER_FONT_SIZE 13

#define KHOLORS_TAB_BUTTONS_INNER_PADDING 42

#define KHOLORS_TAB_PADDING 4
#define KHOLORS_TAB_SECTIONS_MARGINS 5
#define KHOLORS_TABS_HEIGHT 34

#define KHOLORS_TAB_HIGHLIGHT_LINE_WIDTH 2

#define KHOLORS_COLOR_PALETTE                                                                                                  \
    {juce::Colour::fromString("ff399898"), juce::Colour::fromString("ff3c7a98"), juce::Colour::fromString("ff4056a5"), \
     juce::Colour::fromString("ff663fc6"), juce::Colour::fromString("ff8c2fd3"), juce::Colour::fromString("ffcc34c9"), \
     juce::Colour::fromString("ffc2448a"), juce::Colour::fromString("ffc64264"), juce::Colour::fromString("ff9b5f31"), \
     juce::Colour::fromString("ffc5a536"), juce::Colour::fromString("ffbec83d"), juce::Colour::fromString("ff97c43e"), \
     juce::Colour::fromString("ff49c479")}
