#pragma once

#include "GUIToolkit/Widgets/LabeledLineStack.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

class ServerTab : public juce::Component
{
  public:
    ServerTab(TaskingManager &taskManager);
    void resized() override;

  private:
    LabeledLineStack serverStatusStack;
};