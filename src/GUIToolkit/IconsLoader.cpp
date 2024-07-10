#include "IconsLoader.h"
#include "GUIData.h"

#include "Consts.h"

IconsLoader::IconsLoader()
{
    playIcon = juce::Drawable::createFromImageData(GUIData::play_svg, GUIData::play_svgSize);
    playIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    pauseIcon = juce::Drawable::createFromImageData(GUIData::pause_svg, GUIData::pause_svgSize);
    pauseIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    startIcon = juce::Drawable::createFromImageData(GUIData::start_svg, GUIData::start_svgSize);
    startIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    loopIcon = juce::Drawable::createFromImageData(GUIData::loop_svg, GUIData::loop_svgSize);
    loopIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    unloopIcon = juce::Drawable::createFromImageData(GUIData::unloop_svg, GUIData::unloop_svgSize);
    unloopIcon->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    moveIcon = juce::Drawable::createFromImageData(GUIData::move_svg, GUIData::move_svgSize);
    moveIcon->replaceColour(juce::Colours::white, COLOR_BACKGROUND);

    searchIcon = juce::Drawable::createFromImageData(GUIData::search_svg, GUIData::search_svgSize);
    searchIcon->replaceColour(juce::Colours::white, COLOR_SEPARATOR_LINE);

    resizeHorizontalIcon =
        juce::Drawable::createFromImageData(GUIData::resize_horizontal_svg, GUIData::resize_horizontal_svgSize);
    resizeHorizontalIcon->replaceColour(juce::Colours::white, COLOR_BACKGROUND);

    closedCaret =
        juce::Drawable::createFromImageData(GUIData::closed_folder_caret_svg, GUIData::closed_folder_caret_svgSize);
    closedCaret->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    openedCaret =
        juce::Drawable::createFromImageData(GUIData::opened_folder_caret_svg, GUIData::opened_folder_caret_svgSize);
    openedCaret->replaceColour(juce::Colours::white, COLOR_TEXT_DARKER);

    fileIcon = juce::Drawable::createFromImageData(GUIData::file_svg, GUIData::file_svgSize);
    folderIcon = juce::Drawable::createFromImageData(GUIData::folder_svg, GUIData::folder_svgSize);
    audioIcon = juce::Drawable::createFromImageData(GUIData::song_svg, GUIData::song_svgSize);

    artifaktNdLogo =
        juce::Drawable::createFromImageData(GUIData::artifaktnd_logo_svg, GUIData::artifaktnd_logo_svgSize);
}