#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#define LABELED_LINE_CONTAINER_DEFAULT_HEIGHT TABS_HEIGHT
#define LABELED_LINE_CONTAINER_PADDING 12
#define LABELED_LINE_CONTAINER_SPACING 8
#define DEFAULT_LABELED_LINE_WIDTH 260

class LabeledLineContainer : public juce::Component
{
  public:
    LabeledLineContainer(std::string label, std::shared_ptr<juce::Component> content, int maxLabelWidth,
                         int contentWidth);
    std::shared_ptr<juce::Component> getContent();
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    std::shared_ptr<juce::Component> content;
    std::string label;
    int maxLabelWidth;
    int contentWidth;
    juce::Rectangle<int> labelLocalBounds;
    juce::Rectangle<int> contentLocalBounds;
};