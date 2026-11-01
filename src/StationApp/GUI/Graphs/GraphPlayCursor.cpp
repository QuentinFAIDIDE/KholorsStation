#include "GraphPlayCursor.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/AudioConstants.h"
#include <cmath>

constexpr int MIN_SAMPLE_PLAY_CURSOR_BACKWARD_MOVEMENT = 24000;
constexpr int PLAY_CURSOR_WIDTH = 2;

GraphPlayCursor::GraphPlayCursor()
{
}

GraphPlayCursor::~GraphPlayCursor() = default;

void GraphPlayCursor::paint(juce::Graphics &g, const juce::Rectangle<int> &bounds, int64_t viewPosition,
                            int64_t viewScale)
{
    int playCursorStartPixel = 0;
    {
        std::scoped_lock const lock(playCursorMutex);
        playCursorStartPixel = static_cast<float>(playCursorPosition - viewPosition) / static_cast<float>(viewScale);
    }

    // we won't draw the playhead cursor when it's to the 0 position, it's just ugly
    if (playCursorStartPixel > 2)
    {
        auto const areaRightToCursor = bounds.withTrimmedLeft(playCursorStartPixel);
        auto const playCursorBounds = areaRightToCursor.withWidth(PLAY_CURSOR_WIDTH);

        g.setColour(KHOLORS_COLOR_WHITE);
        g.fillRect(playCursorBounds);
    }
}

int64_t GraphPlayCursor::getPlayCursorPosition()
{
    std::scoped_lock const lock(playCursorMutex);
    return playCursorPosition;
}

void GraphPlayCursor::submitNewPlayCursorPosition(int64_t samplePosition, uint32_t sampleRate)
{
    std::scoped_lock const lock(playCursorMutex);
    int64_t newPlayCursorPos = samplePosition;
    if (sampleRate != VISUAL_SAMPLE_RATE)
    {
        newPlayCursorPos =
            static_cast<int64_t>(static_cast<float>(samplePosition) *
                                 (static_cast<float>(VISUAL_SAMPLE_RATE) / static_cast<float>(sampleRate)));
    }

    // only update the play cursor pos if it's bigger than previous one,
    // or if it's smaller and beyond a certain distance
    if (newPlayCursorPos > playCursorPosition ||
        std::abs(playCursorPosition - newPlayCursorPos) > MIN_SAMPLE_PLAY_CURSOR_BACKWARD_MOVEMENT)
    {
        playCursorPosition = newPlayCursorPos;
    }
}