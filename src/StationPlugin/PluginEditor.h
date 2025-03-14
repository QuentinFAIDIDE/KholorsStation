#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "GUIToolkit/IconsLoader.h"
#include "GUIToolkit/KholorsLookAndFeel.h"
#include "PluginProcessor.h"
#include "StationPlugin/Audio/TrackInfoStore.h"
#include "StationPlugin/GUI/BottomInfoLine.h"
#include "StationPlugin/GUI/ClearButton.h"
#include "StationPlugin/GUI/FreqTimeView.h"
#include "StationPlugin/GUI/HelpButton.h"
#include "StationPlugin/GUI/SensitivitySlider.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"

#define MENU_BAR_HEIGHT 80
#define TOP_LOGO_WIDTH 150
#define DEFAULT_WINDOW_WIDTH 1440
#define DEFAULT_WINDOW_HEIGHT 800
#define APP_NAME_FONT_HEIGHT 40
#define VERSION_FONT_HEIGHT 18
#define FREQVIEW_OUTER_BORDER_WIDTH 2
#define TOPBAR_RIGHT_PADDING 25
#define HELP_BUTTON_WIDTH 85
#define CLEAR_BUTTON_WIDTH 100
#define TOPBAR_BUTTON_HEIGHT 55
#define TOPBAR_BUTTONS_PADDING 8
#define TOPBAR_BUTTONS_RIGHT_MARGIN 45
#define SENSITIVITY_SLIDER_WIDTH 200
#define SENSITIVITY_SLIDER_PICTOGRAM_WIDTH 36

/**
 * @brief Class that describes the GUI of the plugin.
 */
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &, TaskingManager &);
    ~AudioPluginAudioProcessorEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    TaskingManager &taskManager;

    juce::Typeface::Ptr typeface;      /**< Default font of the app */
    KholorsLookAndFeel appLookAndFeel; /**< Defines the app look and feel */

    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
    juce::SharedResourcePointer<IconsLoader> sharedSvgs;  /**< singleton that loads svg files */
    TrackInfoStore trackInfoStore;                        /**< storing all track info (name, color) */
    FreqTimeView freqTimeView;                            /**< Viewer that display frequencies over time received */
    BottomInfoLine infoBar;                               /**< bottom tip bar */
    HelpButton helpButton;
    ClearButton clearButton;
    SensitivitySlider volumeSensitivitySlider;

    std::vector<int64_t> taskListenerIdsToClear; /**< These are ids of task listeners items that needs to be cleared
                                                    open closing of the editor window */

    // destroy copy constructors
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
