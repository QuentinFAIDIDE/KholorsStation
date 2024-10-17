#pragma once

#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include <cstdint>
#include <memory>

class ColorPicker : public juce::Component, public TaskListener
{
  public:
    ColorPicker(std::string id, TaskingManager &tm);
    ~ColorPicker();
    void paint(juce::Graphics &g) override;
    void resized() override;

    /**
     * @brief Set the Colour picked.
     * Should automatically detect of the color is one
     * of the chosen one or a custom one.
     *
     * @param r red level from 0 to 255
     * @param g green level from 0 to 255
     * @param b blue level from 0 to 255
     */
    void setColour(uint8_t r, uint8_t g, uint8_t b);

    void mouseDown(const juce::MouseEvent &me) override;

    static std::vector<juce::Colour> getDefaultColours();

    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    std::string identifier; /**< uniquely identifies this colorPicker */
    TaskingManager &taskManager;
    std::vector<juce::Colour> colors;
    size_t pickedColorId; /**< index in the colors array of the color user choosed, or the first number after if he
                             picked his own */
    std::vector<juce::Rectangle<int>> colorSquaresPosition;
    juce::Colour currentColor; /**< used when custom colors is chosen to display it */
    std::mutex selectedColorMutex;

    int64_t taskListenerId; /**< Id to delete itself from task listeners when destructor is called */

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorPicker)
};

class ColorPickerDialog : public juce::Component, juce::Button::Listener
{
  public:
    ColorPickerDialog(std::string colorPickerId, TaskingManager &tm);
    void paint(juce::Graphics &g) override;
    void resized() override;
    void buttonClicked(juce::Button *button) override;
    void closeDialog();

  private:
    std::string colorPickerIdentifier;
    juce::TextButton closeButton;
    juce::TextButton confirmButton;
    juce::ColourSelector colorSelector;
    TaskingManager &taskManager;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorPickerDialog)
};