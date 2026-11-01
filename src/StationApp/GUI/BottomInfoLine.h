#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

inline constexpr int BOTTOM_INFO_LINE_HEIGHT = 35;

/**
 * @brief A component that is meant to be at the bottom of the
 * station window, that showcase info about cursor position
 * and static software information.
 */
class BottomInfoLine : public juce::Component, public TaskListener
{
  public:
    explicit BottomInfoLine(TaskingManager &tm);
    void paint(juce::Graphics &g) override;
    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    static std::string noteFromFreq(float freq);

    TaskingManager &taskingManager;
    std::atomic<float> lastFrequency;
    std::atomic<float> lastSampleTime;
    std::atomic<bool> mouseOverSfftView;
    std::atomic<float> averageSegmentProcessingTimeMs;
    std::string leftText;                                 /**< left aligned bottom bar text */
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads all fonts */
};