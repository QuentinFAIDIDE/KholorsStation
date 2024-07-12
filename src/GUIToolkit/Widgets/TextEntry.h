#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <nlohmann/json.hpp>

class TextEntry : public juce::Component, juce::TextEditor::Listener
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

  private:
    std::string identifier; /**< unique identifier for this component */
    std::string name;       /**< text displayed next to the entry */
    std::string lastTextSendToTask;
    juce::TextEditor textEditor;
    juce::SharedResourcePointer<FontsLoader> sharedFonts;
    int minSizeForTask; /**< Minimum number of chars for a TextEntryUpdateTask to be generated */

    TaskingManager &taskManager;
};

class TextEntryUpdateTask : public Task
{
  public:
    TextEntryUpdateTask(std::string identifier, std::string newTextP, std::string previousTextP)
        : textEntryIdentifier(identifier), previousText(previousTextP), newText(newTextP)
    {
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
        auto rev = std::make_shared<TextEntryUpdateTask>(textEntryIdentifier, previousText, newText);
        std::vector<std::shared_ptr<Task>> response;
        response.push_back(rev);
        return response;
    }

    std::string textEntryIdentifier;
    std::string previousText;
    std::string newText;
};