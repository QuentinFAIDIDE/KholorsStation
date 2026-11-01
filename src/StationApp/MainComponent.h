#pragma once

#include "AudioTransport/SyncServer.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/GUI/BottomInfoLine.h"
#include "StationApp/GUI/DashboardView.h"
#include "StationApp/GUI/TopBar.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_core/juce_core.h"
#include <juce_gui_extra/juce_gui_extra.h>

inline constexpr int DEFAULT_WINDOW_WIDTH = 1440;
inline constexpr int DEFAULT_WINDOW_HEIGHT = 800;
inline constexpr int FREQVIEW_OUTER_BORDER_WIDTH = 2;

class MainComponent final : public juce::Component, public TaskListener
{
  public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics & /*g*/) override;
    void resized() override;
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    juce::Path getShadowPath();
    TaskingManager taskManager;                           /**< Object that manages task for actions */
    juce::MenuBarComponent menuBar;                       /**< App menu at the top of the app */
    TrackInfoStore trackInfoStore;                        /**< storing all track info (name, color) */
    DashboardView dashboardView;                          /**< Viewer that display frequencies over time received */
    AudioTransport::SyncServer audioDataServer;           /**< Server that receives audio data */
    AudioDataWorker audioDataWorker; /**< Worker threads to parse audio data from server and emit Tasks accordingly */
    BottomInfoLine infoBar;          /**< bottom tip bar */
    TopBar topBar;                   /**< top bar with title, sensitivity slider and buttons */
    bool showTipsAtStartup{true};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
