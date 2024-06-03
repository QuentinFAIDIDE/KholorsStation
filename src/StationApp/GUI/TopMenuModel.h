#pragma once

#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

class TopMenuModel : public juce::MenuBarModel
{
  public:
    TopMenuModel(TaskingManager &taskingManager);
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String &menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    void menuBarActivated(bool isActive) override;

  private:
    TaskingManager &taskManager;
    juce::StringArray topLevelMenus;
    std::vector<juce::PopupMenu> menuItems;
};