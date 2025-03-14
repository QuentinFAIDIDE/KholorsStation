#include "BottomInfoLine.h"
#include "GUIToolkit/Consts.h"
#include "StationPlugin/Audio/BrokenLicenseCheckTask.h"
#include "StationPlugin/Audio/ProcessingTimer.h"
#include "StationPlugin/Audio/SimpleLicenseCheckTask.h"
#include "StationPlugin/GUI/FftDrawingBackend.h"
#include "StationPlugin/GUI/MouseCursorInfoTask.h"
#include "StationPlugin/Licensing/DummyLicenseManager.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#define BOTTOM_INFO_LINE_SIDE_PADDING 20

BottomInfoLine::BottomInfoLine(TaskingManager &tm)
    : taskingManager(tm), lastFrequency(0), lastSampleTime(0), mouseOverSfftView(false), licensedWordTxt("Licensed")
{
    averageSegmentProcessingTimeMs = 0;
    setOpaque(true);
}

void BottomInfoLine::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(KHOLORS_COLOR_UNITS);
    g.fillRect(getLocalBounds().withHeight(1));

    {
        std::lock_guard lock(licenseOwnerMutex);
        leftText = licensedWordTxt + " to: " + licenseOwnerInfo + " | Average Audio Processing Delay (milliseconds): " +
                   std::to_string((int)averageSegmentProcessingTimeMs);
    }

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
    {
        std::lock_guard lock(licenseOwnerMutex);
        g.drawText(leftText, bounds, juce::Justification::centredLeft, true);
    }
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

    auto simpleLicenseCheckTask = std::dynamic_pointer_cast<SimpleLicenseCheckTask>(task);
    if (simpleLicenseCheckTask != nullptr && !simpleLicenseCheckTask->isCompleted())
    {
        auto licenseData = DummyLicenseManager::getUserDataAndKeyFromDisk();
        std::string licenseTextPart;
        {
            std::lock_guard lock(licenseOwnerMutex);
            licenseTextPart = licenseOwnerInfo;
        }
        auto lineDataPair = DummyLicenseManager::parseOwnerFromBottomLineOrFail(licenseTextPart);
        licenseData->email = lineDataPair.second;
        licenseData->username = lineDataPair.first;
        if (!DummyLicenseManager::isKeyValid(licenseData.value()))
        {
            DummyLicenseManager::writeUserDataAndKeyToDisk(std::nullopt);
            throw std::runtime_error("nice try, R2R!");
        }
        simpleLicenseCheckTask->setCompleted(true);
        return true;
    }

    auto brokenLicenseCheckTask = std::dynamic_pointer_cast<BrokenLicenseCheckTask>(task);
    if (brokenLicenseCheckTask != nullptr && !brokenLicenseCheckTask->isCompleted() &&
        brokenLicenseCheckTask->stage == 0)
    {
        auto licenseData = DummyLicenseManager::getUserDataAndKeyFromDisk();
        std::string licenseTextPart;
        {
            std::lock_guard lock(licenseOwnerMutex);
            licenseTextPart = licenseOwnerInfo;
        }
        auto lineDataPair = DummyLicenseManager::parseOwnerFromBottomLineOrFail(licenseTextPart);
        licenseData->email = lineDataPair.second;
        licenseData->username = lineDataPair.first;

        brokenLicenseCheckTask->username = licenseData->username;
        brokenLicenseCheckTask->email = licenseData->email;
        brokenLicenseCheckTask->key = licenseData->licenseKey;

        brokenLicenseCheckTask->stage = 1;

        taskingManager.broadcastNestedTaskNow(brokenLicenseCheckTask);

        return true;
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

void BottomInfoLine::setLicenseInfo(std::string username, std::string email)
{
    std::lock_guard lock(licenseOwnerMutex);
    licenseOwnerInfo = username + " (" + email + ")";
}