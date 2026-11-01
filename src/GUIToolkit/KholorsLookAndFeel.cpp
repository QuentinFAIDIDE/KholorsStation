#include "KholorsLookAndFeel.h"

#include "Consts.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_extra/juce_gui_extra.h"

constexpr int MENU_SEPARATOR_HEIGHT = 6;
constexpr int MENU_ITEM_HEIGHT = 28;
constexpr int MENU_ITEM_SIDE_PADDING = 60; /**< note that it will be divided by two on each side*/

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

void KholorsLookAndFeel::drawTabButton(juce::TabBarButton &tb, juce::Graphics &g, bool isMouseOver,
                                       bool /*isMouseDown*/)
{
    auto const bounds = g.getClipBounds();

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
    return {KHOLORS_DEFAULT_FONT_SIZE + 2};
}

juce::Font KholorsLookAndFeel::getMenuBarFont(juce::MenuBarComponent & /*unused*/, int /*itemIndex*/,
                                              const juce::String & /*itemText*/)
{
    return {KHOLORS_DEFAULT_FONT_SIZE + 3};
}

void KholorsLookAndFeel::getIdealPopupMenuItemSize(const juce::String &text, bool isSeparator,
                                                   int /*standardMenuItemHeight*/, int &idealWidth, int &idealHeight)
{
    if (isSeparator)
    {
        idealHeight = MENU_SEPARATOR_HEIGHT;
        return;
    }

    auto const font = getPopupMenuFont();
    int const textWidth = juce::GlyphArrangement::getStringWidthInt(font, text);
    idealWidth = textWidth + MENU_ITEM_SIDE_PADDING;
    idealHeight = MENU_ITEM_HEIGHT;
}

int KholorsLookAndFeel::getTabButtonBestWidth(juce::TabBarButton &tbb, int /*tabDepth*/)
{
    auto const font = juce::Font(KHOLORS_DEFAULT_FONT_SIZE);
    int const i = tbb.getIndex();
    return juce::GlyphArrangement::getStringWidthInt(font, tbb.getTabbedButtonBar().getTabNames()[i]) +
           KHOLORS_TAB_BUTTONS_INNER_PADDING;
}

void KholorsLookAndFeel::drawTabbedButtonBarBackground(juce::TabbedButtonBar & /*unused*/, juce::Graphics &g)
{
    g.setColour(KHOLORS_COLOR_BACKGROUND);
    g.fillAll();

    g.setColour(KHOLORS_COLOR_SEPARATOR_LINE);
    auto const line = g.getClipBounds().withHeight(1);
    g.drawRect(line);
}

void KholorsLookAndFeel::drawTabAreaBehindFrontButton(juce::TabbedButtonBar &tb, juce::Graphics &g, int /*w*/,
                                                      int /*h*/)
{
    int const currentTabIndex = tb.getCurrentTabIndex();
    auto const tabBounds = tb.getTabButton(currentTabIndex)->getBounds();
    g.setColour(KHOLORS_COLOR_HIGHLIGHT);
    auto const line = tabBounds.withY(tabBounds.getY() + tabBounds.getHeight() - KHOLORS_TAB_HIGHLIGHT_LINE_WIDTH)
                          .withHeight(KHOLORS_TAB_HIGHLIGHT_LINE_WIDTH);
    g.fillRect(line);
}

int KholorsLookAndFeel::getPopupMenuBorderSize()
{
    return 0;
}

int KholorsLookAndFeel::getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options & /*unused*/)
{
    return 0;
}

void KholorsLookAndFeel::drawResizableFrame(juce::Graphics & /*unused*/, int /*w*/, int /*h*/,
                                            const juce::BorderSize<int> & /*unused*/)
{
    // not working for removing popup menu borders...
}

void KholorsLookAndFeel::drawMenuBarBackground(juce::Graphics &g, int /*width*/, int /*height*/,
                                               bool /*isMouseOverBar*/, juce::MenuBarComponent & /*unused*/)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void KholorsLookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &b, const juce::Colour &backgroundColor,
                                              bool /*highlighted*/, bool down)
{
    // use the button bounds and not the clip bounds, which only cover the repainted area
    auto buttonBounds =
        b.getLocalBounds().toFloat().reduced(KHOLORS_BUTTON_BORDER_THICKNESS + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
    if (down)
    {
        buttonBounds.setX(buttonBounds.getX() + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
        buttonBounds.setY(buttonBounds.getY() + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
    }
    g.setColour(backgroundColor);
    g.fillRoundedRectangle(buttonBounds, KHOLORS_BUTTON_CORNERSIZE);
    g.setColour(KHOLORS_COLOR_BUTTON_BORDER);
    g.drawRoundedRectangle(buttonBounds.reduced(0.5f), KHOLORS_BUTTON_CORNERSIZE, 1.0f);
}

void KholorsLookAndFeel::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool highlighted, bool down)
{
    g.setFont(sharedFonts->robotoMedium.withHeight(BUTTON_TEXT_HEIGHT));
    g.setColour(KHOLORS_COLOR_TEXT_DARKER);
    if (highlighted)
    {
        g.setColour(KHOLORS_COLOR_TEXT);
    }
    auto textBounds = b.getLocalBounds();
    textBounds.reduce(KHOLORS_BUTTON_DOWN_DISPLACEMENT, KHOLORS_BUTTON_DOWN_DISPLACEMENT);
    if (down)
    {
        g.setColour(KHOLORS_COLOR_BUTTON_HIGHLIGHT);
        textBounds.setX(textBounds.getX() + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
        textBounds.setY(textBounds.getY() + KHOLORS_BUTTON_DOWN_DISPLACEMENT);
    }
    g.drawText(b.getButtonText().toUpperCase(), textBounds, juce::Justification::centred, true);
}

void KholorsLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height, float /*sliderPos*/,
                                          float /*minSliderPos*/, float /*maxSliderPos*/,
                                          juce::Slider::SliderStyle /*unused*/, juce::Slider &sl)
{
    auto const area = juce::Rectangle<int>(x, y, width, height);

    float const posRatio = (sl.getValue() - sl.getMinimum()) / (sl.getMaximum() - sl.getMinimum());

    g.setColour(KHOLORS_COLOR_TEXT_DARKER);
    g.fillRoundedRectangle(area.reduced(0, (area.getHeight() - (LINEAR_SLIDER_HEIGHT - 2)) / 2).toFloat(),
                           LINEAR_SLIDER_BAR_CORNER_RADIUS);

    g.setColour(KHOLORS_COLOR_HIGHLIGHT);
    g.fillRoundedRectangle(
        area.reduced(0, (area.getHeight() - LINEAR_SLIDER_HEIGHT) / 2).toFloat().withWidth(area.getWidth() * posRatio),
        LINEAR_SLIDER_BAR_CORNER_RADIUS);

    auto const handleArea = area.reduced(0, (area.getHeight() - LINEAR_SLIDER_HANDLE_RADIUS) / 2)
                                .toFloat()
                                .withX(area.getX() + (area.getWidth() * posRatio) -
                                       (static_cast<float>(LINEAR_SLIDER_HANDLE_RADIUS) / 2.0f))
                                .withWidth(LINEAR_SLIDER_HANDLE_RADIUS);
    g.fillEllipse(handleArea);
}
