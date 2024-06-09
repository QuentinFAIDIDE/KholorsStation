#pragma once

#include "AudioTransport/SyncServer.h"
#include "GUIToolkit/FontsLoader.h"
#include "GUIToolkit/KholorsLookAndFeel.h"
#include "StationApp/Audio/AudioDataWorker.h"
#include "StationApp/GUI/BottomPanel.h"
#include "StationApp/GUI/FreqTimeView.h"
#include "StationApp/GUI/TopMenuModel.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include <juce_gui_extra/juce_gui_extra.h>

#define MENU_BAR_HEIGHT 30
#define TOP_LOGO_WIDTH 200

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
    KholorsLookAndFeel appLookAndFeel;                    /**< Defines the app look and feel */
    TaskingManager taskManager;                           /**< Object that manages task for actions */
    juce::MenuBarComponent menuBar;                       /**< App menu at the top of the app */
    TopMenuModel menuBarModel;                            /**< Model that populates the menuBar */
    BottomPanel bottomPanel;                              /**< pannel at the bottom with all tabs */
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
    FreqTimeView freqTimeView;                            /**< Viewer that display frequencies over time received */
    AudioTransport::SyncServer audioDataServer;           /**< Server that receives audio data */
    AudioDataWorker audioDataWorker; /**< Worker threads to parse audio data from server and emit Tasks accordingly */

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};