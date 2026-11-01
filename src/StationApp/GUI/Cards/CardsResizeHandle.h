#pragma once

#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <functional>

/**
 * @brief Horizontal handle shown as three dots in the gap between two stacked cards,
 * that resizes them when dragged up or down. It only reports the drag; the owner
 * decides how the cards are resized.
 */
class CardsResizeHandle final : public juce::Component
{
  public:
    CardsResizeHandle();

    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;

    std::function<void()> onDragStart;      /**< called when the user starts dragging */
    std::function<void(int deltaY)> onDrag; /**< vertical distance dragged since the start, down is positive */

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CardsResizeHandle)
};
