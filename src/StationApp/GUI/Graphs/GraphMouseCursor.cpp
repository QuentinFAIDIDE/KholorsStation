#include "GraphMouseCursor.h"
#include "GUIToolkit/Consts.h"

GraphMouseCursor::GraphMouseCursor() : lastMouseX(0), lastMouseY(0), mouseOnComponent(false)
{
}

GraphMouseCursor::~GraphMouseCursor()
{
}

void GraphMouseCursor::paint(juce::Graphics &g, const juce::Rectangle<int> &bounds)
{
    if (mouseOnComponent)
    {
        auto horizontalLine = bounds.withHeight(1).withY(lastMouseY);
        auto verticalLine = bounds.withWidth(1).withX(lastMouseX);
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