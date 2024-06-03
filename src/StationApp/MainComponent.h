#pragma once

#include "GUIToolkit/KholorsLookAndFeel.h"
#include "StationApp/GUI/TopMenuModel.h"
#include "TaskManagement/TaskingManager.h"
#include <juce_gui_extra/juce_gui_extra.h>

#define MENU_BAR_HEIGHT 28

class MainComponent final : public juce::Component
{
  public:
    MainComponent();
    ~MainComponent();

    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    KholorsLookAndFeel appLookAndFeel;
    TaskingManager taskManager;

    juce::MenuBarComponent menuBar;
    TopMenuModel menuBarModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};