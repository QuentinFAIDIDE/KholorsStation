#include "BottomInfoLine.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/Audio/ProcessingTimer.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include "StationApp/GUI/MouseCursorInfoTask.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#define BOTTOM_INFO_LINE_SIDE_PADDING 20

BottomInfoLine::BottomInfoLine(TaskingManager &tm)
    : taskingManager(tm), lastFrequency(0), lastSampleTime(0), mouseOverSfftView(false)
{
    averageSegmentProcessingTimeMs = 0;
    setOpaque(true);
}

void BottomInfoLine::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(KHOLORS_COLOR_UNITS);
    g.fillRect(getLocalBounds().withHeight(1));

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

        rightText = std::to_string(int(lastFrequency + 0.5f)) + " Hz (" + noteFromFreq(lastFrequency) + ")" + posTip;
    }

    auto bounds = getLocalBounds();
    bounds.reduce(BOTTOM_INFO_LINE_SIDE_PADDING, 0);
    g.setFont(juce::Font(KHOLORS_DEFAULT_FONT_SIZE));
    g.setColour(KHOLORS_COLOR_WHITE);
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

std::string BottomInfoLine::noteFromFreq(float freq)
{
    float semiToneShiftFromA4 = 12.0f * std::log2(freq / 440);
    int centDistanceToNearestNote = ((100.0f * (semiToneShiftFromA4 - std::round(semiToneShiftFromA4))) + 0.5f);
    int noteId = (int)(semiToneShiftFromA4 + 0.5f);
    int octaveInvariantNoteId = noteId % 12;
    int octave = 4 + (noteId / 12);
    std::string noteTxt;
    switch (octaveInvariantNoteId)
    {
    case 0:
        noteTxt = "A";
        break;
    case 1:
        noteTxt = "A#";
        break;
    case 2:
        noteTxt = "B";
        break;
    case 3:
        noteTxt = "C";
        break;
    case 4:
        noteTxt = "C#";
        break;
    case 5:
        noteTxt = "D";
        break;
    case 6:
        noteTxt = "D#";
        break;
    case 7:
        noteTxt = "E";
        break;
    case 8:
        noteTxt = "F";
        break;
    case 9:
        noteTxt = "F#";
        break;
    case 10:
        noteTxt = "G";
        break;
    case 11:
        noteTxt = "G#";
        break;
    default:
        noteTxt = "A";
        break;
    }
    noteTxt += std::to_string(octave);
    if (centDistanceToNearestNote < 0)
    {
        noteTxt += " ";
    }
    else
    {
        noteTxt += " +";
    }
    noteTxt += std::to_string(centDistanceToNearestNote) + "c";
    return noteTxt;
}