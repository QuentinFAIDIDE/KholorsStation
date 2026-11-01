#include "TimeScale.h"

#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/AudioConstants.h"
#include "juce_graphics/juce_graphics.h"
#include <math.h>
#include <mutex>
#include <string>
#include <unordered_map>

TimeScale::TimeScale()
{
    setOpaque(true);
}

void TimeScale::setViewPosition(int64_t nviewPosition)
{
    std::scoped_lock const lock(mutex);
    viewPosition = nviewPosition;
}

void TimeScale::setViewScale(int64_t nviewScale)
{
    std::scoped_lock const lock(mutex);
    viewScale = nviewScale;
}

void TimeScale::setBpm(float nbpm)
{
    std::scoped_lock const lock(mutex);
    bpm = nbpm;
}

void TimeScale::resized()
{
    width = getLocalBounds().getWidth();
}

void TimeScale::paint(juce::Graphics &g)
{
    int64_t currentViewPos;
    int64_t currentViewScale;
    float currentBpm = NAN;
    {
        std::scoped_lock const lock(mutex);
        currentBpm = bpm;
        currentViewPos = viewPosition;
        currentViewScale = viewScale;
    }

    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    drawTicks(g, currentViewPos, currentViewScale, currentBpm);
}

void TimeScale::drawTicks(juce::Graphics &g, int64_t currentViewPosition, int64_t currentViewScale, float currentBpm)
{
    float const framesPerMinutes = VISUAL_SAMPLE_RATE * 60;

    float const grid0FrameWidth = (framesPerMinutes / currentBpm);
    float const grid1FrameWidth = (framesPerMinutes / (currentBpm * 4));
    float const grid2FrameWidth = (framesPerMinutes / (currentBpm * 16));

    float const grid2PixelWidth = grid2FrameWidth / static_cast<float>(currentViewScale);
    float const grid1PixelWidth = grid2PixelWidth * 4;
    float const grid0PixelWidth = grid1PixelWidth * 4;

    int const grid0PixelShift =
        (static_cast<int>(grid0FrameWidth + 0.5f) - (currentViewPosition % static_cast<int>(grid0FrameWidth + 0.5f))) /
        currentViewScale;
    int const grid1PixelShift = static_cast<int>(grid0PixelShift + 0.5f) % static_cast<int>(grid1PixelWidth + 0.5f);
    int const grid2PixelShift = static_cast<int>(grid1PixelShift + 0.5f) % static_cast<int>(grid2PixelWidth + 0.5f);

    int const currentBarIndex = static_cast<int>((static_cast<float>(currentViewPosition) / grid0FrameWidth)) + 1;

    g.setFont(KHOLORS_DEFAULT_FONT_SIZE);

    drawTickLevel(g, 4, 4, KHOLORS_COLOR_WHITE.withAlpha(0.0f), grid0PixelWidth, grid0PixelShift, currentBarIndex);
    drawTickLevel(g, 4, 2, KHOLORS_COLOR_WHITE.withAlpha(0.0f), grid1PixelWidth, grid1PixelShift, -1);
}

juce::Rectangle<int> TimeScale::getTickTextArea(int height) const
{
    auto const tickShape = getLocalBounds().removeFromTop(height + TICK_TOP_PADDING).withTrimmedTop(TICK_TOP_PADDING);
    return tickShape.translated(0, tickShape.getHeight() + TICK_LABEL_MARGIN);
}

void TimeScale::drawTickLevel(juce::Graphics &g, int height, int width, juce::Colour /*color*/, float pixelStepWidth,
                              int pixelStepShift, int firstBarIndex)
{
    auto textBox = getTickTextArea(height).withWidth(width);
    textBox.setWidth(TICK_LABEL_WIDTH);

    int currentTickPos = pixelStepShift;
    int i = 0;
    while (currentTickPos < getLocalBounds().getWidth())
    {
        if (firstBarIndex >= 0 && (pixelStepWidth > 50 || ((firstBarIndex + i) % 2) == 0))
        {
            int const quarterIndex = firstBarIndex + i;
            int const barIndex = (quarterIndex / 4) + 1;
            int const quarterInBar = 1 + (quarterIndex % 4);
            const std::string barIndexText = std::to_string(barIndex) + "." + std::to_string(quarterInBar);
            g.setColour(KHOLORS_COLOR_WHITE.withAlpha(0.75f));
            textBox.setX(currentTickPos - (TICK_LABEL_WIDTH >> 1));

            // Use cached glyph arrangement for maximum performance

            // we create the glyph if its not in the cache
            auto const desiredGlypth = glyphCache.find(barIndexText);
            if (desiredGlypth == glyphCache.end())
            {
                // clear entire glyph cache is its full (cheap to repopulate)
                if (glyphCache.size() > MAXIMUM_GLYPH_CACHE_SIZE)
                {
                    glyphCache.clear();
                }
                else
                {
                    glyphCache[barIndexText].addLineOfText(g.getCurrentFont(), barIndexText, 0, 0);
                }
            }
            int const xTranslation = textBox.getX() + textBox.getY() / 2;
            glyphCache[barIndexText].draw(g, juce::AffineTransform::translation(xTranslation, textBox.getCentreY()));
        }

        i++;
        currentTickPos = pixelStepShift + static_cast<int>(static_cast<float>(i) * pixelStepWidth);
    }
}