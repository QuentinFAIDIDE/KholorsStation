#include "LabeledLineStack.h"
#include "GUIToolkit/Consts.h"
#include "GUIToolkit/Widgets/LabeledLineContainer.h"
#include <memory>

LabeledLineStack::LabeledLineStack(std::string name) : stackName(name)
{
}

void LabeledLineStack::resized()
{
    auto bounds = getLocalBounds().reduced(LABELED_LINE_STACK_BORDER_PADDING);
    bounds.removeFromTop(LABELED_LINE_STACK_LABEL_HEIGHT);
    for (size_t i = 0; i < stackedComponents.size(); i++)
    {
        if (bounds.getHeight() < (LABELED_LINE_STACK_LINE_TOP_MARGIN + LABELED_LINE_CONTAINER_DEFAULT_HEIGHT))
        {
            stackedComponents[i]->setBounds(bounds.withHeight(0));
            continue;
        }
        bounds.removeFromTop(LABELED_LINE_STACK_LINE_TOP_MARGIN);
        stackedComponents[i]->setBounds(bounds.removeFromTop(LABELED_LINE_CONTAINER_DEFAULT_HEIGHT));
    }
}

void LabeledLineStack::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds().reduced(LABELED_LINE_STACK_BORDER_PADDING);
    auto labelSpace = bounds.removeFromTop(LABELED_LINE_STACK_LABEL_HEIGHT);
    g.setFont(sharedFonts->robotoBold.withHeight(KHOLORS_DEFAULT_FONT_SIZE));
    g.setColour(KHOLORS_COLOR_TEXT_DARKER);
    g.drawText(TRANS(stackName), labelSpace, juce::Justification::bottomLeft, false);
}

void LabeledLineStack::addComponent(std::shared_ptr<juce::Component> newComponent)
{
    stackedComponents.push_back(newComponent);
    addAndMakeVisible(newComponent.get());
}