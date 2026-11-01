#include "CardsResizeHandle.h"

#include "GUIToolkit/Consts.h"

constexpr float HANDLE_DOT_DIAMETER = 3.0f;
constexpr float HANDLE_DOTS_SPACING = 7.0f; /**< between the centers of two dots */

CardsResizeHandle::CardsResizeHandle()
{
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    setRepaintsOnMouseActivity(true);
}

void CardsResizeHandle::paint(juce::Graphics &g)
{
    g.setColour(isMouseOverOrDragging() ? KHOLORS_COLOR_BADGE_TEXT : KHOLORS_COLOR_DIMMED_LABEL);
    auto const centre = getLocalBounds().toFloat().getCentre();
    for (int i = -1; i <= 1; i++)
    {
        auto const dotCentre = centre.translated(static_cast<float>(i) * HANDLE_DOTS_SPACING, 0.0f);
        g.fillEllipse(juce::Rectangle<float>(HANDLE_DOT_DIAMETER, HANDLE_DOT_DIAMETER).withCentre(dotCentre));
    }
}

void CardsResizeHandle::mouseDown(const juce::MouseEvent & /*e*/)
{
    if (onDragStart)
    {
        onDragStart();
    }
}

void CardsResizeHandle::mouseDrag(const juce::MouseEvent &e)
{
    if (onDrag)
    {
        onDrag(e.getDistanceFromDragStartY());
    }
}
