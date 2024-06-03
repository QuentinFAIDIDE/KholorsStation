#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "GUIToolkit/KholorsLookAndFeel.h"
#include "StationApp/GUI/BottomPanel.h"
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
    KholorsLookAndFeel appLookAndFeel;
    TaskingManager taskManager;

    juce::MenuBarComponent menuBar;
    TopMenuModel menuBarModel;

    BottomPanel bottomPanel;

    juce::SharedResourcePointer<FontsLoader> sharedFonts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};