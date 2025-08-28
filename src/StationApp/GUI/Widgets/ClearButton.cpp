#include "ClearButton.h"
#include "StationApp/GUI/ClearTask.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

ClearButton::ClearButton(TaskingManager &tm)
    : juce::TextButton("Clear", "Clear all FFTs displayed on screen"), taskManager(tm)
{
}

ClearButton::~ClearButton()
{
}

void ClearButton::clicked()
{
    auto clearTask = std::make_shared<ClearTask>();
    taskManager.broadcastTask(clearTask);
}