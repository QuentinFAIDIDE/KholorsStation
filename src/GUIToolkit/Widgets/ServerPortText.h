#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"

class ServerPortText : public juce::Component, public TaskListener
{
  public:
    /**
     * @brief Construct a new object.
     * Registers the server taskHandler against the taskingManager.
     *
     * @param tm tasking manager to register handler to.
     */
    ServerPortText(TaskingManager &tm);

    /**
     * @brief Paint the text reading the port.
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

  private:
    std::optional<uint32_t> currentPort;                  /**< The port of server displayed on screen */
    juce::SharedResourcePointer<FontsLoader> sharedFonts; /**< Singleton that loads fonts */
    std::mutex currentPortMutex;                          /**< Used to protect currentPort variable */
};