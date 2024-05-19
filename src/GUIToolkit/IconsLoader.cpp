#include "IconsLoader.h"
#include "Icons.h"

#include "Consts.h"

IconsLoader::IconsLoader()
{
    playIcon = juce::Drawable::createFromImageData(Icons::play_svg, Icons::play_svgSize);
    playIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    pauseIcon = juce::Drawable::createFromImageData(Icons::pause_svg, Icons::pause_svgSize);
    pauseIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    startIcon = juce::Drawable::createFromImageData(Icons::start_svg, Icons::start_svgSize);
    startIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    loopIcon = juce::Drawable::createFromImageData(Icons::loop_svg, Icons::loop_svgSize);
    loopIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    unloopIcon = juce::Drawable::createFromImageData(Icons::unloop_svg, Icons::unloop_svgSize);
    unloopIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    moveIcon = juce::Drawable::createFromImageData(Icons::move_svg, Icons::move_svgSize);
    moveIcon->replaceColour(juce::Colours::white, COLOR_BACKGROUND);

    searchIcon = juce::Drawable::createFromImageData(Icons::search_svg, Icons::search_svgSize);
    searchIcon->replaceColour(juce::Colours::white, COLOR_SEPARATOR_LINE);

    resizeHorizontalIcon =
        juce::Drawable::createFromImageData(Icons::resize_horizontal_svg, Icons::resize_horizontal_svgSize);
    resizeHorizontalIcon->replaceColour(juce::Colours::white, COLOR_BACKGROUND);

    closedCaret =
        juce::Drawable::createFromImageData(Icons::closed_folder_caret_svg, Icons::closed_folder_caret_svgSize);
    closedCaret->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    openedCaret =
        juce::Drawable::createFromImageData(Icons::opened_folder_caret_svg, Icons::opened_folder_caret_svgSize);
    openedCaret->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    fileIcon = juce::Drawable::createFromImageData(Icons::file_svg, Icons::file_svgSize);
    folderIcon = juce::Drawable::createFromImageData(Icons::folder_svg, Icons::folder_svgSize);
    audioIcon = juce::Drawable::createFromImageData(Icons::song_svg, Icons::song_svgSize);
}