#include "ServerPortText.h"
#include "AudioTransport/ServerStatusTask.h"
#include "GUIToolkit/Consts.h"
#include "TaskManagement/TaskingManager.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

ServerPortText::ServerPortText(TaskingManager &tm)
{
    tm.registerTaskListener(this);
}

void ServerPortText::paint(juce::Graphics &g)
{
    g.setFont(sharedFonts->monospaceFont.withHeight(KHOLORS_DEFAULT_FONT_SIZE));

    std::optional<uint32_t> portToDisplay;
    {
        std::lock_guard lock(currentPortMutex);
        portToDisplay = currentPort;
    }

    if (portToDisplay.has_value())
    {
        g.setColour(KHOLORS_COLOR_TEXT);
        g.drawText(std::to_string(*portToDisplay), getLocalBounds(), juce::Justification::centredRight, true);
    }
    else
    {
        g.setColour(KHOLORS_COLOR_TEXT_WARN);
        g.drawText(TRANS("UNKNOWN"), getLocalBounds(), juce::Justification::centredRight, true);
    }
}

bool ServerPortText::taskHandler(std::shared_ptr<Task> task)
{
    auto taskToParse = std::dynamic_pointer_cast<AudioTransport::ServerStatusTask>(task);
    if (taskToParse != nullptr)
    {
        std::lock_guard lock(currentPortMutex);
        currentPort = taskToParse->runningAndPort.second;
    }

    return false;
}