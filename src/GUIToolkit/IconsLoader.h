#ifndef DEF_ICONS_LOADER_HPP
#define DEF_ICONS_LOADER_HPP

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

struct IconsLoader
{
    IconsLoader();

    std::unique_ptr<juce::Drawable> playIcon;
    std::unique_ptr<juce::Drawable> pauseIcon;
    std::unique_ptr<juce::Drawable> startIcon;
    std::unique_ptr<juce::Drawable> loopIcon;
    std::unique_ptr<juce::Drawable> unloopIcon;
    std::unique_ptr<juce::Drawable> moveIcon;
    std::unique_ptr<juce::Drawable> resizeHorizontalIcon;
    std::unique_ptr<juce::Drawable> searchIcon;
    std::unique_ptr<juce::Drawable> folderIcon;
    std::unique_ptr<juce::Drawable> fileIcon;
    std::unique_ptr<juce::Drawable> audioIcon;
    std::unique_ptr<juce::Drawable> closedCaret;
    std::unique_ptr<juce::Drawable> openedCaret;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IconsLoader)
};

#endif // DEF_ICONS_LOADER_HPP