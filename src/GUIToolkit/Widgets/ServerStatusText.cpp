#include "ServerStatusText.h"
#include "../Consts.h"
#include "AudioTransport/ServerStatusTask.h"
#include "TaskManagement/TaskingManager.h"
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

ServerStatusText::ServerStatusText(TaskingManager &tm)
    : currentStatus(UNKNOWN), unknownStatusColor(COLOR_TEXT_WARN), listeningStatusColor(COLOR_TEXT_STATUS_OK),
      offlineStatusColor(COLOR_TEXT_STATUS_ERROR)
{
    tm.registerTaskListener(this);
}

void ServerStatusText::paint(juce::Graphics &g)
{
    g.setFont(sharedFonts->monospaceFont.withHeight(DEFAULT_FONT_SIZE));
    DisplayedServerState fetchedStatus;
    {
        std::lock_guard lock(currentStatusMutex);
        fetchedStatus = currentStatus;
    }
    switch (fetchedStatus)
    {
    case UNKNOWN:
        g.setColour(unknownStatusColor);
        g.drawText(TRANS("UNKNOWN"), getLocalBounds(), juce::Justification::centredRight, true);
        break;

    case OFFLINE:
        g.setColour(offlineStatusColor);
        g.drawText(TRANS("OFFLINE"), getLocalBounds(), juce::Justification::centredRight, true);
        break;

    case LISTENING:
        g.setColour(listeningStatusColor);
        g.drawText(TRANS("LISTENING"), getLocalBounds(), juce::Justification::centredRight, true);
        break;
    }
}

bool ServerStatusText::taskHandler(std::shared_ptr<Task> task)
{
    auto serverUpdate = std::dynamic_pointer_cast<AudioTransport::ServerStatusTask>(task);
    if (serverUpdate != nullptr)
    {
        {
            std::lock_guard lock(currentStatusMutex);
            if (serverUpdate->runningAndPort.first)
            {
                currentStatus = LISTENING;
            }
            else
            {
                currentStatus = OFFLINE;
            }
        }
    }
    return false;
}