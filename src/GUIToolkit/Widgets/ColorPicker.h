#pragma once

#include "TaskManagement/TaskingManager.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstdint>

// made from https://medialab.github.io/iwanthue/
inline std::vector<juce::Colour> colourPalette = {
    juce::Colour::fromString("ff399898"), juce::Colour::fromString("ff3c7a98"), juce::Colour::fromString("ff4056a5"),
    juce::Colour::fromString("ff663fc6"), juce::Colour::fromString("ff8c2fd3"), juce::Colour::fromString("ffcc34c9"),
    juce::Colour::fromString("ffc2448a"), juce::Colour::fromString("ffc64264"), juce::Colour::fromString("ff9b5f31"),
    juce::Colour::fromString("ffc5a536"), juce::Colour::fromString("ffbec83d"), juce::Colour::fromString("ff97c43e"),
    juce::Colour::fromString("ff49c479")};

class ColorPicker : public juce::Component
{
  public:
    ColorPicker(TaskingManager &tm);
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

  private:
    TaskingManager &taskManager;
    std::vector<juce::Colour> colors;
    size_t pickedColorId; /**< index in the colors array of the color user choosed, or the first number after if he
                             picked his own */
    std::vector<juce::Rectangle<int>> colorSquaresPosition;
};