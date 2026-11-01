#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "juce_graphics/juce_graphics.h"
#include <juce_gui_extra/juce_gui_extra.h>

inline constexpr int KHOLORS_BUTTON_CORNERSIZE = 8;
inline constexpr int KHOLORS_BUTTON_BORDER_THICKNESS = 2;
inline constexpr int KHOLORS_BUTTON_DOWN_DISPLACEMENT = 2;
inline constexpr int BUTTON_TEXT_HEIGHT = 14;
inline constexpr int LINEAR_SLIDER_HEIGHT = 6;
inline constexpr float LINEAR_SLIDER_BAR_CORNER_RADIUS = 4.0f;
inline constexpr int LINEAR_SLIDER_HANDLE_RADIUS = 18;

class KholorsLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    KholorsLookAndFeel();
    void drawTabButton(juce::TabBarButton & /*tb*/, juce::Graphics & /*g*/, bool isMouseOver,
                       bool isMouseDown) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getMenuBarFont(juce::MenuBarComponent & /*unused*/, int /*itemIndex*/,
                              const juce::String & /*itemText*/) override;
    int getTabButtonBestWidth(juce::TabBarButton &tbb, int depth) override;
    void drawTabbedButtonBarBackground(juce::TabbedButtonBar & /*unused*/, juce::Graphics & /*g*/) override;
    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar &tb, juce::Graphics &g, int w, int h) override;
    void getIdealPopupMenuItemSize(const juce::String &text, bool isSeparator, int standardMenuItemHeight,
                                   int &idealWidth, int &idealHeight) override;
    void drawResizableFrame(juce::Graphics &g, int w, int h, const juce::BorderSize<int> &border) override;
    int getPopupMenuBorderSize() override;
    int getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options & /*unused*/) override;
    void drawMenuBarBackground(juce::Graphics &g, int width, int height, bool isMouseOverBar,
                               juce::MenuBarComponent & /*unused*/) override;
    void drawButtonBackground(juce::Graphics &g, juce::Button &b, const juce::Colour &backgroundColor, bool highlighted,
                              bool down) override;
    void drawButtonText(juce::Graphics & /*g*/, juce::TextButton & /*b*/, bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawLinearSlider(juce::Graphics & /*g*/, int x, int y, int width, int height, float sliderPos,
                          float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle /*unused*/,
                          juce::Slider & /*sl*/) override;

  private:
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
};
