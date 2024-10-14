#pragma once

#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

#define BOTTOM_INFO_LINE_HEIGHT 35

/**
 * @brief A component that is meant to be at the bottom of the
 * station window, that showcase info about cursor position
 * and static software information.
 */
class BottomInfoLine : public juce::Component, public TaskListener
{
  public:
    BottomInfoLine(TaskingManager &tm);
    void paint(juce::Graphics &g) override;
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    std::string noteFromFreq(float freq);

    TaskingManager &taskingManager;
    std::atomic<float> lastFrequency;
    std::atomic<float> lastSampleTime;
    std::atomic<bool> mouseOverSfftView;
    std::atomic<float> averageSegmentProcessingTimeMs;
};