#include "KholorsLookAndFeel.h"

#include "Consts.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_extra/juce_gui_extra.h"

#define MENU_SEPARATOR_HEIGHT 6
#define MENU_ITEM_HEIGHT 28
#define MENU_ITEM_SIDE_PADDING 60 /**< note that it will be divided by two on each side*/

KholorsLookAndFeel::KholorsLookAndFeel()
{
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, KHOLORS_COLOR_BACKGROUND);
    setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, KHOLORS_COLOR_SELECTED_BACKGROUND);

    setColour(juce::TabbedButtonBar::ColourIds::tabOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TabbedButtonBar::ColourIds::frontOutlineColourId, juce::Colours::transparentBlack);

    setColour(juce::TabbedComponent::ColourIds::outlineColourId, juce::Colours::transparentBlack);

    setColour(juce::ScrollBar::ColourIds::thumbColourId, KHOLORS_COLOR_HIGHLIGHT);

    setColour(juce::CaretComponent::ColourIds::caretColourId, KHOLORS_COLOR_HIGHLIGHT);

    setColour(juce::TreeView::ColourIds::linesColourId, KHOLORS_COLOR_TEXT.withAlpha(0.8f));
    setColour(juce::TreeView::ColourIds::selectedItemBackgroundColourId, KHOLORS_COLOR_SELECTED_BACKGROUND);

    setColour(juce::ListBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ListBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);

    setColour(juce::TextEditor::ColourIds::textColourId, KHOLORS_COLOR_INPUT_TEXT);
    setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::highlightColourId, KHOLORS_COLOR_HIGHLIGHT);

    setColour(juce::TextButton::ColourIds::buttonColourId, KHOLORS_COLOR_BACKGROUND);
    setColour(juce::TextButton::ColourIds::buttonOnColourId, KHOLORS_COLOR_BACKGROUND.darker());
    setColour(juce::TextButton::ColourIds::textColourOnId, KHOLORS_COLOR_TEXT);
    setColour(juce::TextButton::ColourIds::textColourOffId, KHOLORS_COLOR_TEXT_DARKER);

    setColour(juce::ColourSelector::ColourIds::backgroundColourId, KHOLORS_COLOR_BACKGROUND);

    setColour(juce::ResizableWindow::backgroundColourId, KHOLORS_COLOR_BACKGROUND);
}

void KholorsLookAndFeel::drawTabButton(juce::TabBarButton &tb, juce::Graphics &g, bool isMouseOver, bool)
{
    auto bounds = g.getClipBounds();

    g.setColour(KHOLORS_COLOR_TEXT);
    g.setFont(juce::Font(KHOLORS_DEFAULT_FONT_SIZE));

    if (tb.isFrontTab())
    {
        g.setColour(KHOLORS_COLOR_HIGHLIGHT);
    }

    if (isMouseOver)
    {
        g.setColour(KHOLORS_COLOR_HIGHLIGHT);
    }

    g.drawText(tb.getButtonText().toUpperCase(), bounds.reduced(4, 0), juce::Justification::centred, true);
}

juce::Font KholorsLookAndFeel::getPopupMenuFont()
{
    return juce::Font(KHOLORS_DEFAULT_FONT_SIZE + 2);
}

juce::Font KholorsLookAndFeel::getMenuBarFont(juce::MenuBarComponent &, int, const juce::String &)
{
    return juce::Font(KHOLORS_DEFAULT_FONT_SIZE + 3);
}

void KholorsLookAndFeel::getIdealPopupMenuItemSize(const juce::String &text, bool isSeparator, int, int &idealWidth,
                                                   int &idealHeight)
{
    if (isSeparator)
    {
        idealHeight = MENU_SEPARATOR_HEIGHT;
        return;
    }

    auto font = getPopupMenuFont();
    int textWidth = font.getStringWidth(text);
    idealWidth = textWidth + MENU_ITEM_SIDE_PADDING;
    idealHeight = MENU_ITEM_HEIGHT;
}

int KholorsLookAndFeel::getTabButtonBestWidth(juce::TabBarButton &tbb, int)
{
    auto font = juce::Font(KHOLORS_DEFAULT_FONT_SIZE);
    int i = tbb.getIndex();
    return font.getStringWidth(tbb.getTabbedButtonBar().getTabNames()[i]) + KHOLORS_TAB_BUTTONS_INNER_PADDING;
}

void KholorsLookAndFeel::drawTabbedButtonBarBackground(juce::TabbedButtonBar &, juce::Graphics &g)
{
    g.setColour(KHOLORS_COLOR_BACKGROUND);
    g.fillAll();

    g.setColour(KHOLORS_COLOR_SEPARATOR_LINE);
    auto line = g.getClipBounds().withHeight(1);
    g.drawRect(line);
}

void KholorsLookAndFeel::drawTabAreaBehindFrontButton(juce::TabbedButtonBar &tb, juce::Graphics &g, int, int)
{
    int currentTabIndex = tb.getCurrentTabIndex();
    auto tabBounds = tb.getTabButton(currentTabIndex)->getBounds();
    g.setColour(KHOLORS_COLOR_HIGHLIGHT);
    auto line = tabBounds.withY(tabBounds.getY() + tabBounds.getHeight() - KHOLORS_TAB_HIGHLIGHT_LINE_WIDTH)
                    .withHeight(KHOLORS_TAB_HIGHLIGHT_LINE_WIDTH);
    g.fillRect(line);
}

int KholorsLookAndFeel::getPopupMenuBorderSize()
{
    return 0;
}

int KholorsLookAndFeel::getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options &)
{
    return 0;
}

void KholorsLookAndFeel::drawResizableFrame(juce::Graphics &, int, int, const juce::BorderSize<int> &)
{
    // not working for removing popup menu borders...
    return;
}

void KholorsLookAndFeel::drawMenuBarBackground(juce::Graphics &g, int, int, bool, juce::MenuBarComponent &)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void KholorsLookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &, const juce::Colour &backgroundColor,
                                              bool highlighted, bool down)
{
    g.setColour(backgroundColor);
    auto reducedClipBounds =
        g.getClipBounds().toFloat().reduced(KHOLORS_BUTTON_BORDER_THICKNESS + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
    if (down)
    {
        reducedClipBounds.setX(reducedClipBounds.getX() + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
        reducedClipBounds.setY(reducedClipBounds.getY() + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
    }
    g.fillRoundedRectangle(reducedClipBounds, KHOLORS_BUTTON_CORNERSIZE);
}

void KholorsLookAndFeel::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool highlighted, bool down)
{
    g.setFont(sharedFonts->robotoBold.withHeight(BUTTON_TEXT_HEIGHT));
    g.setColour(KHOLORS_COLOR_TEXT_DARKER);
    if (highlighted)
    {
        g.setColour(KHOLORS_COLOR_TEXT);
    }
    auto textBounds = g.getClipBounds();
    textBounds.reduce(KHOLORS_BUTTON_DOWN_DISPLACEMENT, KHOLORS_BUTTON_DOWN_DISPLACEMENT);
    if (down)
    {
        g.setColour(KHOLORS_COLOR_BUTTON_HIGHLIGHT);
        textBounds.setX(textBounds.getX() + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
        textBounds.setY(textBounds.getY() + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
    }
    g.drawText(b.getButtonText().toUpperCase(), textBounds, juce::Justification::centred, true);
}