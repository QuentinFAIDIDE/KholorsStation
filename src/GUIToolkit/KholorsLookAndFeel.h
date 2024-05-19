#ifndef DEF_LOOKANDFEEL_HPP
#define DEF_LOOKANDFEEL_HPP

#include <juce_gui_extra/juce_gui_extra.h>

class KholorsLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    KholorsLookAndFeel();
    void drawTabButton(juce::TabBarButton &, juce::Graphics &, bool isMouseOver, bool isMouseDown) override;
    juce::Font getPopupMenuFont() override;
    int getTabButtonBestWidth(juce::TabBarButton &tbb, int depth) override;
    void drawTabbedButtonBarBackground(juce::TabbedButtonBar &, juce::Graphics &) override;
    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar &tb, juce::Graphics &g, int w, int h) override;
    void getIdealPopupMenuItemSize(const juce::String &text, bool isSeparator, int standardMenuItemHeight,
                                   int &idealWidth, int &idealHeight) override;
    void drawResizableFrame(juce::Graphics &g, int w, int h, const juce::BorderSize<int> &border) override;
    int getPopupMenuBorderSize() override;
    int getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options &) override;
};

#endif // DEF_LOOKANDFEEL_HPP