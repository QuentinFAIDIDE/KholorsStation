#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "juce_gui_basics/juce_gui_basics.h"

// pixel size of the space reserved for label at the top
#define LABELED_LINE_STACK_LABEL_HEIGHT 20
// shrinking amount in pixel of the entire widget
#define LABELED_LINE_STACK_BORDER_PADDING 8
// space above each line (and therefore between each line)
#define LABELED_LINE_STACK_LINE_TOP_MARGIN 8

/**
 * @brief A label with multiple lines below containing components.
 */
class LabeledLineStack : public juce::Component
{
  public:
    LabeledLineStack(std::string name);
    void resized() override;
    void paint(juce::Graphics &g) override;
    void addComponent(std::shared_ptr<juce::Component> newComponent);

  private:
    std::string stackName;
    std::vector<std::shared_ptr<juce::Component>> stackedComponents;
    juce::SharedResourcePointer<FontsLoader> sharedFonts;
};