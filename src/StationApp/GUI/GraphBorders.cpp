#pragma once

#include "StationApp/GUI/GraphBorders.h"
#include "GUIToolkit/Consts.h"
#include "juce_graphics/juce_graphics.h"

#define FREQVIEW_ROUNDED_CORNERS_WIDTH 7
#define FREQVIEW_BORDER_WIDTH 3

void drawGraphBorders(juce::Graphics &g, juce::Rectangle<int> bounds, bool drawMiddleLine)
{
    // we fill two sub path, one for the borders that are to the left
    // of the middle vertical line, and one for the borders that are to the right
    // of it.
    auto fillPath = juce::Path();
    // we start first subpath to the pixel at the bottom center
    fillPath.startNewSubPath(bounds.getCentreX(), bounds.getBottom());
    fillPath.lineTo(bounds.getBottomLeft().toFloat());
    fillPath.lineTo(bounds.getTopLeft().toFloat());
    fillPath.lineTo(bounds.getCentreX(), bounds.getTopLeft().getY());
    fillPath.lineTo(bounds.getCentreX(), bounds.getTopLeft().getY() + FREQVIEW_BORDER_WIDTH);

    fillPath.lineTo(bounds.getTopLeft().getX() + FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH,
                    bounds.getTopLeft().getY() + FREQVIEW_BORDER_WIDTH);

    fillPath.quadraticTo(bounds.getTopLeft().getX() + FREQVIEW_BORDER_WIDTH,
                         bounds.getTopLeft().getY() + FREQVIEW_BORDER_WIDTH,
                         bounds.getTopLeft().getX() + FREQVIEW_BORDER_WIDTH,
                         bounds.getTopLeft().getY() + FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH);

    fillPath.lineTo(bounds.getBottomLeft().getX() + FREQVIEW_BORDER_WIDTH,
                    bounds.getBottomLeft().getY() - FREQVIEW_BORDER_WIDTH - FREQVIEW_ROUNDED_CORNERS_WIDTH);

    fillPath.quadraticTo(bounds.getBottomLeft().translated(FREQVIEW_BORDER_WIDTH, -FREQVIEW_BORDER_WIDTH).toFloat(),
                         bounds.getBottomLeft()
                             .translated(FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH, -FREQVIEW_BORDER_WIDTH)
                             .toFloat());

    fillPath.lineTo(bounds.getCentreX(), bounds.getBottom() - FREQVIEW_BORDER_WIDTH);
    fillPath.closeSubPath();

    // subpath to the right of the screen
    fillPath.startNewSubPath(bounds.getCentreX(), bounds.getBottom());
    fillPath.lineTo(bounds.getBottomRight().toFloat());
    fillPath.lineTo(bounds.getTopRight().toFloat());
    fillPath.lineTo(bounds.getCentreX(), bounds.getTopRight().getY());
    fillPath.lineTo(bounds.getCentreX(), bounds.getTopRight().getY() + FREQVIEW_BORDER_WIDTH);
    fillPath.lineTo(bounds.getTopRight()
                        .translated(-(FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH), FREQVIEW_BORDER_WIDTH)
                        .toFloat());

    fillPath.quadraticTo(bounds.getTopRight().translated(-FREQVIEW_BORDER_WIDTH, FREQVIEW_BORDER_WIDTH).toFloat(),
                         bounds.getTopRight()
                             .translated(-FREQVIEW_BORDER_WIDTH, FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH)
                             .toFloat());

    fillPath.lineTo(bounds.getBottomRight()
                        .translated(-(FREQVIEW_BORDER_WIDTH), -(FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH))
                        .toFloat());
    fillPath.quadraticTo(
        bounds.getBottomRight().translated(-FREQVIEW_BORDER_WIDTH, -FREQVIEW_BORDER_WIDTH).toFloat(),
        bounds.getBottomRight()
            .translated(-(FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH), -(FREQVIEW_BORDER_WIDTH))
            .toFloat());
    fillPath.lineTo(bounds.getCentreX(), bounds.getBottomRight().getY() - FREQVIEW_BORDER_WIDTH);
    fillPath.closeSubPath();

    g.setColour(KHOLORS_COLOR_FREQVIEW_GRADIENT_BORDERS);
    g.fillPath(fillPath);

    g.setColour(KHOLORS_COLOR_GRIDS_LEVEL_0);
    int borders2Width = 2;
    g.drawRoundedRectangle(bounds.toFloat(), FREQVIEW_ROUNDED_CORNERS_WIDTH, borders2Width);

    if (drawMiddleLine)
    {
        auto middleLine = bounds.withY(bounds.getHeight() / 2).withHeight(1);
        g.setColour(KHOLORS_COLOR_GRIDS_LEVEL_0);
        g.fillRect(middleLine);
    }
}