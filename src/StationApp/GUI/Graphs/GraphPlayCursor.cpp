#include "GraphPlayCursor.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/AudioConstants.h"
#include <cmath>

#define MIN_SAMPLE_PLAY_CURSOR_BACKWARD_MOVEMENT 24000
#define PLAY_CURSOR_WIDTH 2

GraphPlayCursor::GraphPlayCursor() : playCursorPosition(0)
{
}

GraphPlayCursor::~GraphPlayCursor()
{
}

void GraphPlayCursor::paint(juce::Graphics &g, const juce::Rectangle<int> &bounds, int64_t viewPosition, int64_t viewScale)
{
    int playCursorStartPixel = 0;
    {
        std::lock_guard lock(playCursorMutex);
        playCursorStartPixel = (float)(playCursorPosition - viewPosition) / (float)viewScale;
    }

    // we won't draw the playhead cursor when it's to the 0 position, it's just ugly
    if (playCursorStartPixel > 2)
    {
        auto areaRightToCursor = bounds.withTrimmedLeft(playCursorStartPixel);
        auto playCursorBounds = areaRightToCursor.withWidth(PLAY_CURSOR_WIDTH);

        g.setColour(KHOLORS_COLOR_WHITE);
        g.fillRect(playCursorBounds);
    }
}

int64_t GraphPlayCursor::getPlayCursorPosition()
{
    std::lock_guard lock(playCursorMutex);
    return playCursorPosition;
}

void GraphPlayCursor::submitNewPlayCursorPosition(int64_t samplePosition, uint32_t sampleRate)
{
    std::lock_guard lock(playCursorMutex);
    int64_t newPlayCursorPos = samplePosition;
    if (sampleRate != VISUAL_SAMPLE_RATE)
    {
        newPlayCursorPos = (int64_t)(float(samplePosition) * ((float)VISUAL_SAMPLE_RATE / (float)sampleRate));
    }

    // only update the play cursor pos if it's bigger than previous one,
    // or if it's smaller and beyond a certain distance
    if (newPlayCursorPos > playCursorPosition ||
        std::abs(playCursorPosition - newPlayCursorPos) > MIN_SAMPLE_PLAY_CURSOR_BACKWARD_MOVEMENT)
    {
        playCursorPosition = newPlayCursorPos;
    }
}