#pragma once

#include "AudioTransport/SyncServer.h"
#include "GUIToolkit/FontsLoader.h"
#include "GUIToolkit/IconsLoader.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/GUI/FreqTimeView.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_core/juce_core.h"
#include <juce_gui_extra/juce_gui_extra.h>

#define MENU_BAR_HEIGHT 80
#define TOP_LOGO_WIDTH 150
#define DEFAULT_WINDOW_WIDTH 1440
#define DEFAULT_WINDOW_HEIGHT 800
#define APP_NAME_FONT_HEIGHT 40

class MainComponent final : public juce::Component, public TaskListener
{
  public:
    MainComponent();
    ~MainComponent();

    void paint(juce::Graphics &) override;
    void paintOverChildren(juce::Graphics &) override;
    void resized() override;
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    TaskingManager taskManager;                           /**< Object that manages task for actions */
    juce::MenuBarComponent menuBar;                       /**< App menu at the top of the app */
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
    TrackInfoStore trackInfoStore;                        /**< storing all track info (name, color) */
    FreqTimeView freqTimeView;                            /**< Viewer that display frequencies over time received */
    AudioTransport::SyncServer audioDataServer;           /**< Server that receives audio data */
    AudioDataWorker audioDataWorker; /**< Worker threads to parse audio data from server and emit Tasks accordingly */

    juce::SharedResourcePointer<IconsLoader> sharedSvgs; /**< singleton that loads svg files */

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};