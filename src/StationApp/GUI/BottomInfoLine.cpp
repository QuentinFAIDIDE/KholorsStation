#include "BottomInfoLine.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/ProcessingTimer.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/MouseCursorInfoTask.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include <memory>
#include <string>

#define BOTTOM_INFO_LINE_SIDE_PADDING 20

BottomInfoLine::BottomInfoLine(TaskingManager &tm)
    : taskingManager(tm), lastFrequency(0), lastSampleTime(0), mouseOverSfftView(false)
{
    averageSegmentProcessingTimeMs = 0;
}

void BottomInfoLine::paint(juce::Graphics &g)
{
    g.setColour(COLOR_UNITS);
    g.fillRect(getLocalBounds().withHeight(1));

    std::string leftText =
        "Average Audio Processing Delay (milliseconds): " + std::to_string((int)averageSegmentProcessingTimeMs);
    std::string rightText = TRANS("No tip to display because mouse is not on grid...").toStdString();

    if (mouseOverSfftView)
    {
        std::string posTip;
        float secs = lastSampleTime / float(VISUAL_SAMPLE_RATE);
        float ms = (secs - std::floor(secs)) * 1000;
        posTip += "     |    " + std::to_string(int(secs)) +
                  " s"
                  "   " +
                  std::to_string(int(ms + 0.5f)) + " ms";

        rightText = std::to_string(int(lastFrequency + 0.5f)) + " Hz" + posTip;
    }

    auto bounds = getLocalBounds();
    bounds.reduce(BOTTOM_INFO_LINE_SIDE_PADDING, 0);
    g.setFont(juce::Font(DEFAULT_FONT_SIZE));
    g.setColour(COLOR_WHITE);
    g.drawText(leftText, bounds, juce::Justification::centredLeft, true);
    g.drawText(rightText, bounds, juce::Justification::centredRight, true);
}

bool BottomInfoLine::taskHandler(std::shared_ptr<Task> task)
{
    auto cursorInfo = std::dynamic_pointer_cast<MouseCursorInfoTask>(task);
    if (cursorInfo != nullptr)
    {
        mouseOverSfftView = cursorInfo->cursorOnSfftView;
        lastFrequency = cursorInfo->frequencyUnderMouse;
        lastSampleTime = cursorInfo->timeUnderMouse;
        {
            juce::MessageManager::Lock mmLock;
            juce::MessageManager::Lock::ScopedTryLockType tryLock(mmLock);
            while (!tryLock.isLocked())
            {
                if (task->getTaskingManager()->shutdownWasCalled())
                {
                    return true;
                }
                tryLock.retryLock();
            }
            repaint();
        }
    }

    auto processingRateTask = std::dynamic_pointer_cast<ProcessingTimeUpdateTask>(task);
    if (processingRateTask != nullptr)
    {
        averageSegmentProcessingTimeMs = processingRateTask->averageProcesingTimeMs;
        {
            juce::MessageManager::Lock mmLock;
            juce::MessageManager::Lock::ScopedTryLockType tryLock(mmLock);
            while (!tryLock.isLocked())
            {
                if (task->getTaskingManager()->shutdownWasCalled())
                {
                    return true;
                }
                tryLock.retryLock();
            }
            repaint();
        }
    }

    return false;
}