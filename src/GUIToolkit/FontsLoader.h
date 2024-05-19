#ifndef DEF_CUSTOM_FONTS_HPP
#define DEF_CUSTOM_FONTS_HPP

#include <juce_gui_basics/juce_gui_basics.h>

struct FontsLoader
{
    FontsLoader();

    juce::Font monospaceFont;
    juce::Font roboto;
    juce::Font robotoBold;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FontsLoader)
};

#endif // DEF_CUSTOM_FONTS_HPP