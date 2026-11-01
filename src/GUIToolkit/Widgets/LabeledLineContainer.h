#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

inline constexpr int LABELED_LINE_CONTAINER_DEFAULT_HEIGHT = KHOLORS_TABS_HEIGHT;
inline constexpr int LABELED_LINE_CONTAINER_PADDING = 12;
inline constexpr int LABELED_LINE_CONTAINER_SPACING = 8;
inline constexpr int DEFAULT_LABELED_LINE_WIDTH = 260;

class LabeledLineContainer : public juce::Component
{
  public:
    LabeledLineContainer(std::string label, std::shared_ptr<juce::Component> content, int maxLabelWidth,
                         int contentWidth);
    std::shared_ptr<juce::Component> getContent();
    void paint(juce::Graphics & /*g*/) override;
    void resized() override;

  private:
    std::shared_ptr<juce::Component> content;
    std::string label;
    int maxLabelWidth;
    int contentWidth;
    juce::Rectangle<int> labelLocalBounds;
    juce::Rectangle<int> contentLocalBounds;
};