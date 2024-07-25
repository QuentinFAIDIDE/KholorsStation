#include "ColorPicker.h"
#include "ColorPickerUpdateTask.h"
#include "GUIToolkit/Consts.h"
#include "GUIToolkit/Widgets/DialogConsts.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

#define COLOR_SQUARE_WIDTH 30
#define COLOR_SQUARE_PADDING 10
#define CROSS_SQUARE_PADDING 4
#define CROSS_SQUARE_LINE_WIDTH 4

ColorPicker::ColorPicker(std::string id, TaskingManager &tm) : identifier(id), taskManager(tm)
{
    colors = getDefaultColours();
    colorSquaresPosition.resize(colors.size() + 1);
    pickedColorId = 0;
    taskListenerId = taskManager.registerTaskListener(this);
}

ColorPicker::~ColorPicker()
{
    taskManager.purgeTaskListener(taskListenerId);
}

void ColorPicker::paint(juce::Graphics &g)
{
    size_t currentlySelectedColor;
    juce::Colour selectedColor;
    {
        std::lock_guard lock(selectedColorMutex);
        currentlySelectedColor = pickedColorId;
        selectedColor = currentColor;
    }

    int baseX = getLocalBounds().getX();
    int squarePixelJump = COLOR_SQUARE_WIDTH + COLOR_SQUARE_PADDING;
    auto rectBounds = juce::Rectangle<int>(COLOR_SQUARE_WIDTH, COLOR_SQUARE_WIDTH);
    rectBounds.setX(baseX);
    rectBounds.setY(getLocalBounds().getY());
    for (size_t i = 0; i <= colors.size(); i++)
    {
        colorSquaresPosition[i] = rectBounds;

        // draw the damn thing
        if (i < colors.size())
        {
            g.setColour(colors[i]);
            g.fillRect(rectBounds);
            if (i == currentlySelectedColor)
            {
                g.setColour(COLOR_TEXT);
                g.drawRect(rectBounds, 2);
            }
            else
            {
                g.setColour(COLOR_TEXT_DARKER);
                g.drawRect(rectBounds, 1);
            }
        }
        else
        {
            if (i == currentlySelectedColor)
            {
                g.setColour(selectedColor);
                g.fillRect(rectBounds);
            }

            g.setColour(COLOR_WHITE);
            auto horizontalLine = rectBounds.withHeight(CROSS_SQUARE_LINE_WIDTH)
                                      .translated(0, (COLOR_SQUARE_WIDTH - CROSS_SQUARE_LINE_WIDTH) / 2);
            auto verticalLine = rectBounds.withWidth(CROSS_SQUARE_LINE_WIDTH)
                                    .translated((COLOR_SQUARE_WIDTH - CROSS_SQUARE_LINE_WIDTH) / 2, 0);
            g.fillRect(horizontalLine.reduced(CROSS_SQUARE_PADDING, 0));
            g.fillRect(verticalLine.reduced(0, CROSS_SQUARE_PADDING));

            if (i == currentlySelectedColor)
            {
                g.setColour(COLOR_TEXT);
                g.drawRect(rectBounds, 2);
            }
            else
            {
                g.setColour(COLOR_TEXT_DARKER);
                g.drawRect(rectBounds, 1);
            }
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
    // if one of the color was clicked we emit a task to update it
    for (size_t i = 0; i < colorSquaresPosition.size(); i++)
    {
        if (colorSquaresPosition[i].contains(me.getMouseDownPosition()))
        {
            // TODO: stop ignoring the custom color button
            if (i != colorSquaresPosition.size() - 1)
            {
                auto colorToSet = colors[i];
                auto colorUpdateTask = std::make_shared<ColorPickerUpdateTask>(
                    identifier, colorToSet.getRed(), colorToSet.getGreen(), colorToSet.getBlue());
                taskManager.broadcastTask(colorUpdateTask);
            }
            else
            {
                juce::DialogWindow::LaunchOptions launchOptions;
                launchOptions.dialogTitle = "Pick a color";
                launchOptions.content.set(new ColorPickerDialog(identifier, taskManager), true);
                launchOptions.escapeKeyTriggersCloseButton = true;
                launchOptions.useNativeTitleBar = true;
                launchOptions.resizable = false;
                launchOptions.useBottomRightCornerResizer = false;
                launchOptions.launchAsync();
            }
            break;
        }
    }
}

std::vector<juce::Colour> ColorPicker::getDefaultColours()
{
    return std::vector<juce::Colour> COLOR_PALETTE;
}

bool ColorPicker::taskHandler(std::shared_ptr<Task> task)
{
    auto colorUpdateTask = std::dynamic_pointer_cast<ColorPickerUpdateTask>(task);
    // either we update based on completed task, or either we process a color update request
    if (colorUpdateTask != nullptr && colorUpdateTask->colorPickerIdentifier == identifier &&
        !colorUpdateTask->hasFailed() && !colorUpdateTask->isCompleted())
    {
        colorUpdateTask->redPrevious = currentColor.getRed();
        colorUpdateTask->greenPrevious = currentColor.getBlue();
        colorUpdateTask->bluePrevious = currentColor.getBlue();

        setColour(colorUpdateTask->red, colorUpdateTask->green, colorUpdateTask->blue);

        colorUpdateTask->setCompleted(true);

        {
            juce::MessageManager::Lock mmLock;
            juce::MessageManager::Lock::ScopedTryLockType tryLock(mmLock);
            while (!tryLock.isLocked())
            {
                if (task->getTaskingManager()->shutdownWasCalled())
                {
                    return true;
                }
                tryLock.retryLock();
            }
            repaint();
        }

        taskManager.broadcastNestedTaskNow(colorUpdateTask);
        return true;
    }

    return false;
}

void ColorPicker::setColour(uint8_t r, uint8_t g, uint8_t b)
{
    int64_t colorId = -1;
    // try to detect if the color is one of ours
    for (size_t i = 0; i < colors.size(); i++)
    {
        if (r == colors[i].getRed() && g == colors[i].getGreen() && b == colors[i].getBlue())
        {
            colorId = (int64_t)i;
            break;
        }
    }
    std::lock_guard lock(selectedColorMutex);
    if (colorId == -1)
    {
        pickedColorId = colors.size();
        currentColor = juce::Colour(r, g, b);
    }
    else
    {
        pickedColorId = (size_t)colorId;
        currentColor = colors[(size_t)colorId];
    }
}

//////////////////////////////////////////////////////////////////////:

ColorPickerDialog::ColorPickerDialog(std::string id, TaskingManager &tm)
    : colorPickerIdentifier(id), colorSelector(juce::ColourSelector::ColourSelectorOptions::showColourspace |
                                               juce::ColourSelector::ColourSelectorOptions::showColourAtTop),
      taskManager(tm)
{
    setSize(500, 350);
    closeButton.setButtonText(TRANS("Close"));
    confirmButton.setButtonText(TRANS("Confirm"));
    closeButton.addListener(this);
    confirmButton.addListener(this);
    addAndMakeVisible(colorSelector);
    addAndMakeVisible(closeButton);
    addAndMakeVisible(confirmButton);
}

void ColorPickerDialog::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void ColorPickerDialog::resized()
{
    auto bounds = getLocalBounds();

    auto bottomBand = bounds.removeFromBottom(DIALOG_FOOTER_AREA_HEIGHT);

    // remove side margins and center vertically for buttons
    bottomBand.reduce(DIALOG_FOOTER_BUTTONS_SPACING, (DIALOG_FOOTER_AREA_HEIGHT - DIALOG_FOOTER_BUTTONS_HEIGHT) / 2);

    confirmButton.setBounds(bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));
    bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_SPACING);
    closeButton.setBounds(bottomBand.removeFromRight(DIALOG_FOOTER_BUTTONS_WIDTH));

    colorSelector.setBounds(bounds);
}

void ColorPickerDialog::closeDialog()
{
    if (juce::DialogWindow *dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        dw->exitModalState(0);
    }
}

void ColorPickerDialog::buttonClicked(juce::Button *clickedButton)
{
    if (clickedButton == &confirmButton)
    {
        auto col = colorSelector.getCurrentColour();
        auto colorUpdateTask =
            std::make_shared<ColorPickerUpdateTask>(colorPickerIdentifier, col.getRed(), col.getGreen(), col.getBlue());
        taskManager.broadcastTask(colorUpdateTask);
    }
    if (clickedButton == &closeButton || clickedButton == &confirmButton)
    {
        closeDialog();
    }
}