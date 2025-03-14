#pragma once

#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

class ClearButton : public juce::TextButton
{
  public:
    ClearButton(TaskingManager &tm);
    ~ClearButton();

    void clicked() override;

  private:
    TaskingManager &taskManager;
};