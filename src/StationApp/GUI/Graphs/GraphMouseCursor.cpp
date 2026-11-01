#include "GraphMouseCursor.h"
#include "GUIToolkit/Consts.h"

GraphMouseCursor::GraphMouseCursor() = default;

void GraphMouseCursor::paint(juce::Graphics &g, const juce::Rectangle<int> &bounds) const
{
    if (mouseOnComponent)
    {
        auto const horizontalLine = bounds.withHeight(1).withY(lastMouseY);
        auto const verticalLine = bounds.withWidth(1).withX(lastMouseX);
        g.setColour(KHOLORS_COLOR_WHITE);
        g.fillRect(horizontalLine);
        g.fillRect(verticalLine);
    }
}

void GraphMouseCursor::setMouseCursor(bool onComponent, int x, int y)
{
    mouseOnComponent = onComponent;
    lastMouseX = x;
    lastMouseY = y;
}