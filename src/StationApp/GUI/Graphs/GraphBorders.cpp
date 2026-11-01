#include "StationApp/GUI/Graphs/GraphBorders.h"
#include "GUIToolkit/Consts.h"
#include "juce_graphics/juce_graphics.h"

GraphBorders::~GraphBorders()
{
    borderCache.clear();
}

uint64_t GraphBorders::getBorderCacheKey(juce::Rectangle<int> bounds, bool drawMiddleLine)
{
    return (static_cast<uint64_t>(bounds.getWidth()) << 32) | (static_cast<uint64_t>(bounds.getHeight()) << 16) |
           (drawMiddleLine ? 1 : 0);
}

void GraphBorders::draw(juce::Graphics &g, juce::Rectangle<int> bounds, bool drawMiddleLine)
{
    uint64_t const cacheKey = getBorderCacheKey(bounds, drawMiddleLine);

    auto const cached = borderCache.find(cacheKey);
    if (cached != borderCache.end())
    {
        g.drawImageAt(cached->second, bounds.getX(), bounds.getY());
        return;
    }

    // Clear cache if it gets too large
    if (borderCache.size() > 50)
    {
        borderCache.clear();
    }

    // Render to cached image
    juce::Image const cachedBorder(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
    juce::Graphics cacheGraphics(cachedBorder);

    // Adjust bounds for local coordinate system
    auto const localBounds = juce::Rectangle<int>(0, 0, bounds.getWidth(), bounds.getHeight());
    // we fill two sub path, one for the borders that are to the left
    // of the middle vertical line, and one for the borders that are to the right
    // of it.
    auto fillPath = juce::Path();
    // we start first subpath to the pixel at the bottom center
    fillPath.startNewSubPath(localBounds.getCentreX(), localBounds.getBottom());
    fillPath.lineTo(localBounds.getBottomLeft().toFloat());
    fillPath.lineTo(localBounds.getTopLeft().toFloat());
    fillPath.lineTo(localBounds.getCentreX(), localBounds.getTopLeft().getY());
    fillPath.lineTo(localBounds.getCentreX(), localBounds.getTopLeft().getY() + FREQVIEW_BORDER_WIDTH);

    fillPath.lineTo(localBounds.getTopLeft().getX() + FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH,
                    localBounds.getTopLeft().getY() + FREQVIEW_BORDER_WIDTH);

    fillPath.quadraticTo(localBounds.getTopLeft().getX() + FREQVIEW_BORDER_WIDTH,
                         localBounds.getTopLeft().getY() + FREQVIEW_BORDER_WIDTH,
                         localBounds.getTopLeft().getX() + FREQVIEW_BORDER_WIDTH,
                         localBounds.getTopLeft().getY() + FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH);

    fillPath.lineTo(localBounds.getBottomLeft().getX() + FREQVIEW_BORDER_WIDTH,
                    localBounds.getBottomLeft().getY() - FREQVIEW_BORDER_WIDTH - FREQVIEW_ROUNDED_CORNERS_WIDTH);

    fillPath.quadraticTo(
        localBounds.getBottomLeft().translated(FREQVIEW_BORDER_WIDTH, -FREQVIEW_BORDER_WIDTH).toFloat(),
        localBounds.getBottomLeft()
            .translated(FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH, -FREQVIEW_BORDER_WIDTH)
            .toFloat());

    fillPath.lineTo(localBounds.getCentreX(), localBounds.getBottom() - FREQVIEW_BORDER_WIDTH);
    fillPath.closeSubPath();

    // subpath to the right of the screen
    fillPath.startNewSubPath(localBounds.getCentreX(), localBounds.getBottom());
    fillPath.lineTo(localBounds.getBottomRight().toFloat());
    fillPath.lineTo(localBounds.getTopRight().toFloat());
    fillPath.lineTo(localBounds.getCentreX(), localBounds.getTopRight().getY());
    fillPath.lineTo(localBounds.getCentreX(), localBounds.getTopRight().getY() + FREQVIEW_BORDER_WIDTH);
    fillPath.lineTo(localBounds.getTopRight()
                        .translated(-(FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH), FREQVIEW_BORDER_WIDTH)
                        .toFloat());

    fillPath.quadraticTo(localBounds.getTopRight().translated(-FREQVIEW_BORDER_WIDTH, FREQVIEW_BORDER_WIDTH).toFloat(),
                         localBounds.getTopRight()
                             .translated(-FREQVIEW_BORDER_WIDTH, FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH)
                             .toFloat());

    fillPath.lineTo(localBounds.getBottomRight()
                        .translated(-(FREQVIEW_BORDER_WIDTH), -(FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH))
                        .toFloat());
    fillPath.quadraticTo(
        localBounds.getBottomRight().translated(-FREQVIEW_BORDER_WIDTH, -FREQVIEW_BORDER_WIDTH).toFloat(),
        localBounds.getBottomRight()
            .translated(-(FREQVIEW_BORDER_WIDTH + FREQVIEW_ROUNDED_CORNERS_WIDTH), -(FREQVIEW_BORDER_WIDTH))
            .toFloat());
    fillPath.lineTo(localBounds.getCentreX(), localBounds.getBottomRight().getY() - FREQVIEW_BORDER_WIDTH);
    fillPath.closeSubPath();

    cacheGraphics.setColour(KHOLORS_COLOR_FREQVIEW_GRADIENT_BORDERS);
    cacheGraphics.fillPath(fillPath);

    if (drawMiddleLine)
    {
        auto const middleLine = localBounds.withY(localBounds.getHeight() / 2).withHeight(1);
        cacheGraphics.setColour(KHOLORS_COLOR_GRIDS_LEVEL_0);
        cacheGraphics.fillRect(middleLine);
    }

    // Cache the rendered image
    borderCache[cacheKey] = cachedBorder;

    // Draw the cached image
    g.drawImageAt(cachedBorder, bounds.getX(), bounds.getY());
}