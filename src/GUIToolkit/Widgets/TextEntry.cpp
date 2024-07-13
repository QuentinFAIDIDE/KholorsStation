#include "TextEntry.h"
#include "GUIToolkit/Consts.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>
#include <mutex>

#define TEXT_INPUT_INNER_PADDING 8
#define INPUT_LABEL_WIDTH 100

TextEntry::NameFilter::NameFilter()
    : juce::TextEditor::LengthAndCharacterRestriction(
          25, " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")
{
}

TextEntry::TextEntry(TaskingManager &tm, std::string id, std::string shownName, std::string def, TextFilterId filter,
                     int minCharacters)
    : identifier(id), name(shownName), lastTextSendToTask(def), minSizeForTask(minCharacters), taskManager(tm)
{
    if (filter == NameTextFilter)
    {
        textEditor.setInputFilter(new NameFilter(), true);
    }
    textEditor.setMultiLine(false);
    textEditor.setFont(sharedFonts->monospaceFont);
    textEditor.applyColourToAllText(COLOR_INPUT_TEXT);
    textEditor.setText(def);
    textEditor.addListener(this);
    textEditor.setJustification(juce::Justification::centredRight);
    addAndMakeVisible(textEditor);
}

void TextEntry::paint(juce::Graphics &g)
{
    g.fillAll(COLOR_INPUT_BACKGROUND);
    auto labelBounds = getLocalBounds().reduced(TEXT_INPUT_INNER_PADDING);
    labelBounds = labelBounds.removeFromLeft(INPUT_LABEL_WIDTH);
    g.drawText(name, labelBounds, juce::Justification::centredLeft, true);
}

void TextEntry::resized()
{
    auto textBounds = getLocalBounds().reduced(TEXT_INPUT_INNER_PADDING);
    textBounds.removeFromLeft(INPUT_LABEL_WIDTH);
    textEditor.setBounds(textBounds);
}

void TextEntry::setText(std::string newText)
{
    juce::MessageManagerLock mmlock;
    lastTextSendToTask = newText;
    textEditor.setText(newText);
}

void TextEntry::textEditorTextChanged(juce::TextEditor &te)
{
    auto txt = textEditor.getText();
    if (txt.toStdString() != lastTextSendToTask)
    {
        auto newTask = std::make_shared<TextEntryUpdateTask>(identifier, txt.toStdString());
        newTask->previousText = lastTextSendToTask;
        newTask->setCompleted(true);
        lastTextSendToTask = txt.toStdString();
        taskManager.broadcastTask(newTask);
    }
}

bool TextEntry::taskHandler(std::shared_ptr<Task> task)
{
    auto textUpdateTask = std::dynamic_pointer_cast<TextEntryUpdateTask>(task);
    if (textUpdateTask != nullptr && !textUpdateTask->isCompleted() &&
        textUpdateTask->textEntryIdentifier == identifier)
    {
        {
            juce::MessageManagerLock mmlock;
            textUpdateTask->previousText = textEditor.getText().toStdString();
            setText(textUpdateTask->newText);
        }
        textUpdateTask->setCompleted(true);
        taskManager.broadcastNestedTaskNow(textUpdateTask);
        return true;
    }
    return false;
}