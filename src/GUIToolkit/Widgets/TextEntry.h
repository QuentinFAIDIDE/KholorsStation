#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <nlohmann/json.hpp>

class TextEntry : public juce::Component, juce::TextEditor::Listener, TaskListener
{
  public:
    /**
     * @brief Identifies built in text filtering classes.
     *
     */
    enum TextFilterId
    {
        NoTextFilter = 0,
        NameTextFilter = 1
    };

    TextEntry(TaskingManager &tm, std::string identifier, std::string name, std::string defaultValue,
              TextFilterId filter = NoTextFilter, int minSizeForTask = 3);
    void paint(juce::Graphics &g) override;
    void resized() override;
    void setText(std::string newText);

    void textEditorTextChanged(juce::TextEditor &te) override;

    /**
     * @brief Accepts space, letter and numbers, or eventually - and _
     *
     */
    class NameFilter : public juce::TextEditor::LengthAndCharacterRestriction
    {
      public:
        NameFilter();
    };

    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    std::string identifier; /**< unique identifier for this component */
    std::string name;       /**< text displayed next to the entry */
    std::string lastTextSendToTask;
    juce::TextEditor textEditor;
    juce::SharedResourcePointer<FontsLoader> sharedFonts;
    int minSizeForTask; /**< Minimum number of chars for a TextEntryUpdateTask to be generated */

    TaskingManager &taskManager;
};

/**
 * @brief This task is to update the text entries values. When the text entry
 * broadcast it, it's already marked completed and has the previous value saved for
 * undoing, but when an external component
 * wants to set its value and create the task, the TextEntry will complete
 * it and rebroadcast it.
 * When an external component emits the task, it is by default undo-able but
 * you can prevent it from being by setting undoable as false.
 */
class TextEntryUpdateTask : public Task
{
  public:
    TextEntryUpdateTask(std::string identifier, std::string newTextP)
        : textEntryIdentifier(identifier), newText(newTextP)
    {
        undoable = true;
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "text_entry_update"},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"previous_text", previousText},
                                {"new_text", newText},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    std::vector<std::shared_ptr<Task>> getOppositeTasks() override
    {
        std::vector<std::shared_ptr<Task>> response;
        if (undoable)
        {
            auto rev = std::make_shared<TextEntryUpdateTask>(textEntryIdentifier, previousText);
            rev->previousText = newText;
            response.push_back(rev);
        }
        return response;
    }

    std::string textEntryIdentifier;
    std::string previousText;
    std::string newText;
    bool undoable;
};