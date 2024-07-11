#include "ColorPicker.h"
#include "GUIToolkit/Consts.h"
#include "TaskManagement/TaskingManager.h"

#define COLOR_SQUARE_WIDTH 30
#define COLOR_SQUARE_PADDING 10
#define CROSS_SQUARE_PADDING 4
#define CROSS_SQUARE_LINE_WIDTH 4

ColorPicker::ColorPicker(TaskingManager &tm) : taskManager(tm)
{
    colors = getDefaultColours();
    colorSquaresPosition.resize(colors.size() + 1);
}

void ColorPicker::paint(juce::Graphics &g)
{
    int baseX = getLocalBounds().getX();
    int squarePixelJump = COLOR_SQUARE_WIDTH + COLOR_SQUARE_PADDING;
    auto rectBounds = juce::Rectangle<int>(COLOR_SQUARE_WIDTH, COLOR_SQUARE_WIDTH);
    rectBounds.setX(baseX);
    rectBounds.setY(getLocalBounds().getY());
    for (size_t i = 0; i <= colourPalette.size(); i++)
    {
        colorSquaresPosition[i] = rectBounds;

        // draw the damn thing
        if (i < colourPalette.size())
        {
            g.setColour(colors[i]);
            g.fillRect(rectBounds);
            g.setColour(COLOR_WHITE);
            g.drawRect(rectBounds);
        }
        else
        {
            g.setColour(COLOR_WHITE);
            auto horizontalLine = rectBounds.withHeight(CROSS_SQUARE_LINE_WIDTH)
                                      .translated(0, (COLOR_SQUARE_WIDTH - CROSS_SQUARE_LINE_WIDTH) / 2);
            auto verticalLine = rectBounds.withWidth(CROSS_SQUARE_LINE_WIDTH)
                                    .translated((COLOR_SQUARE_WIDTH - CROSS_SQUARE_LINE_WIDTH) / 2, 0);
            g.fillRect(horizontalLine.reduced(CROSS_SQUARE_PADDING, 0));
            g.fillRect(verticalLine.reduced(0, CROSS_SQUARE_PADDING));

            g.setColour(COLOR_WHITE);
            g.drawRect(rectBounds);
        }

        // shift the color square for the next iteration
        if (getLocalBounds().contains(rectBounds.translated(squarePixelJump, 0)))
        {
            rectBounds.translate(squarePixelJump, 0);
        }
        else if (getLocalBounds().contains(rectBounds.translated(0, squarePixelJump).withX(baseX)))
        {
            rectBounds.translate(0, squarePixelJump);
            rectBounds.setX(baseX);
        }
        else
        {
            // no more space to draw...
            break;
        }
    }
}

void ColorPicker::resized()
{
}

void ColorPicker::mouseDown(const juce::MouseEvent &me)
{
}

std::vector<juce::Colour> ColorPicker::getDefaultColours()
{
    return colourPalette;
}