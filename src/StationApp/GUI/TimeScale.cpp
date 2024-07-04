#include "TimeScale.h"
#include "GUIToolkit/Consts.h"
#include "StationApp/GUI/FftDrawingBackend.h"
#include <mutex>
#include <string>

TimeScale::TimeScale()
{
}

void TimeScale::setViewPosition(int64_t nviewPosition)
{
    std::lock_guard lock(mutex);
    viewPosition = nviewPosition;
}

void TimeScale::setViewScale(int64_t nviewScale)
{
    std::lock_guard lock(mutex);
    viewScale = nviewScale;
}

void TimeScale::setBpm(float nbpm)
{
    std::lock_guard lock(mutex);
    bpm = nbpm;
}

void TimeScale::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    int64_t currentViewPos, currentViewScale;
    float currentBpm;
    {
        std::lock_guard lock(mutex);
        currentBpm = bpm;
        currentViewPos = viewPosition;
        currentViewScale = viewScale;
    }

    drawTicks(g, currentViewPos, currentViewScale, currentBpm);

    g.setFont(juce::Font(TITLE_PIXELS_HEIGHT));
    g.setColour(COLOR_WHITE);
    g.drawText(TRANS("Track Time").toUpperCase(), getLocalBounds().withTrimmedBottom(TITLE_PIXELS_FROM_BOTTOM),
               juce::Justification::centredBottom, false);
}

void TimeScale::drawTicks(juce::Graphics &g, int64_t currentViewPosition, int64_t currentViewScale, float currentBpm)
{
    float framesPerMinutes = VISUAL_SAMPLE_RATE * 60;

    float grid0FrameWidth = (framesPerMinutes / currentBpm);
    float grid1FrameWidth = (framesPerMinutes / (currentBpm * 4));
    float grid2FrameWidth = (framesPerMinutes / (currentBpm * 16));

    float grid2PixelWidth = grid2FrameWidth / float(currentViewScale);
    float grid1PixelWidth = grid2PixelWidth * 4;
    float grid0PixelWidth = grid1PixelWidth * 4;

    int grid0PixelShift =
        (int(grid0FrameWidth + 0.5f) - (currentViewPosition % int(grid0FrameWidth + 0.5f))) / currentViewScale;
    int grid1PixelShift = int(grid0PixelShift + 0.5f) % int(grid1PixelWidth + 0.5f);
    int grid2PixelShift = int(grid1PixelShift + 0.5f) % int(grid2PixelWidth + 0.5f);

    g.setColour(COLOR_WHITE.withAlpha(0.2f));
    g.fillRect(getLocalBounds().removeFromTop(1));

    int currentBarIndex = int((float(currentViewPosition) / grid0FrameWidth)) + 1;

    g.setFont(DEFAULT_FONT_SIZE);

    drawTickLevel(g, 4, 4, COLOR_WHITE.withAlpha(0.0f), grid0PixelWidth, grid0PixelShift, currentBarIndex);
    drawTickLevel(g, 4, 2, COLOR_WHITE.withAlpha(0.0f), grid1PixelWidth, grid1PixelShift, -1);
}

void TimeScale::drawTickLevel(juce::Graphics &g, int height, int width, juce::Colour color, float pixelStepWidth,
                              int pixelStepShift, int firstBarIndex)
{
    auto tickShape =
        getLocalBounds().removeFromTop(height + TICK_TOP_PADDING).withWidth(width).withTrimmedTop(TICK_TOP_PADDING);
    auto textBox = tickShape.translated(0, tickShape.getHeight() + TICK_LABEL_MARGIN);
    textBox.setWidth(TICK_LABEL_WIDTH);

    int currentTickPos = pixelStepShift;
    int i = 0;
    while (currentTickPos < getLocalBounds().getWidth())
    {
        if (firstBarIndex >= 0 && (pixelStepWidth > 50 || ((firstBarIndex + i) % 2) == 0))
        {
            int quarterIndex = firstBarIndex + i;
            int barIndex = (quarterIndex / 4) + 1;
            int quarterInBar = 1 + (quarterIndex % 4);
            std::string barIndexText = std::to_string(barIndex) + "." + std::to_string(quarterInBar);
            g.setColour(COLOR_WHITE.withAlpha(0.75f));
            textBox.setX(currentTickPos - (TICK_LABEL_WIDTH >> 1));
            g.drawText(barIndexText, textBox, juce::Justification::centred, false);
        }
        g.setColour(color);
        g.fillRect(tickShape.withX(currentTickPos - (width >> 1)));
        i++;
        currentTickPos = pixelStepShift + (int)(float(i) * pixelStepWidth);
    }
}