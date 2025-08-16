#pragma once

#include "AudioTransport/SyncServer.h"
#include "GUI/HelpButton.h"
#include "GUIToolkit/FontsLoader.h"
#include "GUIToolkit/IconsLoader.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/GUI/BottomInfoLine.h"
#include "StationApp/GUI/ClearButton.h"
#include "StationApp/GUI/FreqTimeView.h"
#include "StationApp/GUI/SensitivitySlider.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_core/juce_core.h"
#include <juce_gui_extra/juce_gui_extra.h>

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

class MainComponent final : public juce::Component, public TaskListener
{
  public:
    MainComponent();
    ~MainComponent();

    void paint(juce::Graphics &) override;
    void resized() override;
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    juce::Path getShadowPath();
    TaskingManager taskManager;                           /**< Object that manages task for actions */
    juce::MenuBarComponent menuBar;                       /**< App menu at the top of the app */
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
    TrackInfoStore trackInfoStore;                        /**< storing all track info (name, color) */
    FreqTimeView freqTimeView;                            /**< Viewer that display frequencies over time received */
    AudioTransport::SyncServer audioDataServer;           /**< Server that receives audio data */
    AudioDataWorker audioDataWorker; /**< Worker threads to parse audio data from server and emit Tasks accordingly */
    BottomInfoLine infoBar;          /**< bottom tip bar */
    HelpButton helpButton;
    ClearButton clearButton;
    SensitivitySlider volumeSensitivitySlider;
    bool showTipsAtStartup;

    juce::SharedResourcePointer<IconsLoader> sharedSvgs; /**< singleton that loads svg files */

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};