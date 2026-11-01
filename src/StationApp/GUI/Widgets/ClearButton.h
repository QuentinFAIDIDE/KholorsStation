#pragma once

#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

class ClearButton : public juce::TextButton
{
  public:
    explicit ClearButton(TaskingManager &tm);
    ~ClearButton() override;

    void clicked() override;

  private:
    TaskingManager &taskManager;
};