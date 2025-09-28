#include "VolumeScale.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/NormalizedUnitTransformer.h"
#include <limits>
#include <optional>
#include <vector>

VolumeScale::VolumeScale()
{
    setOpaque(true);
}

VolumeScale::~VolumeScale()
{
}

void VolumeScale::paint(juce::Graphics &g)
{
    // fill background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // draw the "VOLUME" axis title
    g.setColour(KHOLORS_COLOR_WHITE);
    drawRotatedTitle(g);

    g.setColour(KHOLORS_COLOR_UNITS);
    drawRotatedChannelNames(g);
}

void VolumeScale::drawRotatedTitle(juce::Graphics &g)
{
    int distanceFromLeft = getWidth() - (AXIS_TITLE_PIXELS_HEIGHT * 2) - 12;

    juce::GlyphArrangement ga;
    ga.addLineOfText(juce::Font(AXIS_TITLE_PIXELS_HEIGHT), TRANS("Stacked Volumes").toUpperCase(), 0, 0);
    juce::Path p;
    ga.createPath(p);

    auto pathBounds = p.getBounds();

    p.applyTransform(
        juce::AffineTransform()
            .rotated(3.0f * juce::MathConstants<float>::halfPi, pathBounds.getCentreX(), pathBounds.getCentreY())
            .translated(0, getHeight() * 0.5f));

    p.applyTransform(juce::AffineTransform().translated(distanceFromLeft - p.getBounds().getX(), 0));

    g.fillPath(p);
}

void VolumeScale::drawRotatedChannelNames(juce::Graphics &g)
{
    int distanceFromLeft = getWidth() - AXIS_TITLE_PIXELS_HEIGHT - 6;

    juce::GlyphArrangement ga;
    ga.addLineOfText(juce::Font(AXIS_TITLE_PIXELS_HEIGHT), TRANS("Left").toUpperCase(), 0, 0);
    juce::Path p;
    ga.createPath(p);
    auto pathBounds = p.getBounds();
    p.applyTransform(
        juce::AffineTransform()
            .rotated(3.0f * juce::MathConstants<float>::halfPi, pathBounds.getCentreX(), pathBounds.getCentreY())
            .translated(0, getHeight() * 0.25f));
    p.applyTransform(juce::AffineTransform().translated(distanceFromLeft - p.getBounds().getX(), 0));
    g.fillPath(p);

    ga.clear();
    ga.addLineOfText(juce::Font(AXIS_TITLE_PIXELS_HEIGHT), TRANS("Right").toUpperCase(), 0, 0);
    p.clear();
    ga.createPath(p);
    pathBounds = p.getBounds();
    p.applyTransform(
        juce::AffineTransform()
            .rotated(3.0f * juce::MathConstants<float>::halfPi, pathBounds.getCentreX(), pathBounds.getCentreY())
            .translated(0, getHeight() * 0.75f));
    p.applyTransform(juce::AffineTransform().translated(distanceFromLeft - p.getBounds().getX(), 0));
    g.fillPath(p);
}