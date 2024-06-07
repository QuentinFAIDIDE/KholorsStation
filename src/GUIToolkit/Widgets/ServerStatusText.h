#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

/**
 * @brief A simple text widget that reads the server status (UNKNOWN, LISTENING, OFFLINE)
 * based on the update received through the taskManager taskHandler callback.
 */
class ServerStatusText : public juce::Component, public TaskListener
{
  public:
    /**
     * @brief Construct a new Server Status Text object.
     * Registers the server taskHandler against the taskingManager.
     *
     * @param tm tasking manager to register handler to.
     */
    ServerStatusText(TaskingManager &tm);

    /**
     * @brief Paint the text reading the server status.
     *
     * @param g
     */
    void paint(juce::Graphics &g) override;

    /**
     * @brief Called by taskingManager when a task is received, will parse server state.
     *
     * @param task eventual task, not necessarely the right one for this task.
     * @return true Will never be used here, stop the task from propagating.
     * @return false Will always be used here, let the task propagate more.
     */
    bool taskHandler(std::shared_ptr<Task> task) override;

    // enumeration of all possible displayed states
    enum DisplayedServerState
    {
        UNKNOWN,
        LISTENING,
        OFFLINE,
    };

  private:
    DisplayedServerState currentStatus;                   /**< The state of server displayed on screen */
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads fonts */
    std::mutex currentStatusMutex;                        /**< Used to protect currentStatus variable */
    juce::Colour unknownStatusColor;
    juce::Colour listeningStatusColor;
    juce::Colour offlineStatusColor;
};