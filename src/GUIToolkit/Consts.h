#pragma once

#define COLOR_BACKGROUND juce::Colour(25, 35, 79)
#define COLOR_BACKGROUND_LIGHTER juce::Colour(50, 64, 126)
#define COLOR_APP_BACKGROUND juce::Colour(25, 35, 79)
#define COLOR_BACKGROUND_HIGHLIGHT juce::Colour(50, 64, 126)
#define COLOR_SAMPLE_BORDER juce::Colour(87, 110, 195)
#define COLOR_TEXT COLOR_WHITE
#define COLOR_TEXT_DARKER juce::Colour(179, 179, 179)
#define COLOR_NOTIF_BAR_TOP juce::Colour(62, 49, 49)
#define COLOR_NOTIF_BAR_BOTTOM juce::Colour(10, 10, 10)
#define COLOR_LABELS_BORDER juce::Colour(210, 210, 220)
#define COLOR_SPLIT_PLACEHOLDER juce::Colour(210, 30, 30)
#define COLOR_SELECT_AREA juce::Colour(54, 210, 210)
#define COLOR_OPAQUE_BICOLOR_LIST_1 COLOR_APP_BACKGROUND
#define COLOR_OPAQUE_BICOLOR_LIST_2 COLOR_APP_BACKGROUND
#define COLOR_DIALOG_BACKGROUND juce::Colour(30, 30, 30)
#define COLOR_SEPARATOR_LINE juce::Colour(80, 80, 80)
#define COLOR_HIGHLIGHT juce::Colour(87, 110, 195)
#define COLOR_SELECTED_BACKGROUND juce::Colour(33, 45, 90)
#define COLOR_GRIDS_LEVEL_0 juce::Colour(juce::uint8(31), 38, 62, 1.0f)
#define COLOR_GRIDS_LEVEL_1 juce::Colour(juce::uint8(31), 38, 62, 1.0f)
#define COLOR_GRIDS_LEVEL_2 juce::Colour(juce::uint8(31), 38, 62, 0.5f)
#define COLOR_FREQVIEW_BORDER juce::Colour(129, 137, 166)
#define COLOR_INPUT_BACKGROUND juce::Colour(211, 222, 222)
#define COLOR_INPUT_TEXT juce::Colour(51, 51, 51)

#define COLOR_FREQVIEW_GRADIENT_BORDERS juce::Colour(12, 17, 40)
#define COLOR_FREQVIEW_GRADIENT_CENTER juce::Colour(12, 17, 40)

#define COLOR_NOTIF_INFO juce::Colour(64, 86, 165)
#define COLOR_NOTIF_WARN juce::Colour(155, 95, 49)
#define COLOR_NOTIF_ERROR juce::Colour(198, 66, 100)

// these ones are lighter and more appropriate for text
#define COLOR_TEXT_STATUS_OK juce::Colour(94, 219, 103)
#define COLOR_TEXT_WARN juce::Colour(219, 157, 94)
#define COLOR_TEXT_STATUS_ERROR juce::Colour(219, 94, 94)

#define COLOR_UNITS COLOR_TEXT_DARKER.withAlpha(0.5f)

#define COLOR_BLACK juce::Colour(10, 10, 10)
#define COLOR_WHITE juce::Colour(243, 243, 243)

#define DEFAULT_FONT_SIZE 17
#define SMALLER_FONT_SIZE 13

#define TAB_BUTTONS_INNER_PADDING 42

#define TAB_PADDING 4
#define TAB_SECTIONS_MARGINS 5
#define TABS_HEIGHT 34

#define POPUP_MENU_IDEAL_HEIGHT 18
#define POPUP_MENU_SEPARATOR_IDEAL_HEIGHT 4
#define TAB_HIGHLIGHT_LINE_WIDTH 2

#define COLOR_PALETTE                                                                                                  \
    {juce::Colour::fromString("ff399898"), juce::Colour::fromString("ff3c7a98"), juce::Colour::fromString("ff4056a5"), \
     juce::Colour::fromString("ff663fc6"), juce::Colour::fromString("ff8c2fd3"), juce::Colour::fromString("ffcc34c9"), \
     juce::Colour::fromString("ffc2448a"), juce::Colour::fromString("ffc64264"), juce::Colour::fromString("ff9b5f31"), \
     juce::Colour::fromString("ffc5a536"), juce::Colour::fromString("ffbec83d"), juce::Colour::fromString("ff97c43e"), \
     juce::Colour::fromString("ff49c479")}
