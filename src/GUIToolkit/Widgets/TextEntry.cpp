#include "TextEntry.h"
#include "GUIToolkit/Consts.h"

TextEntry::TextEntry()
{
}

void TextEntry::paint(juce::Graphics &g)
{
    g.fillAll(COLOR_INPUT_BACKGROUND);
}

void TextEntry::resized()
{
}

void TextEntry::setText(std::string newText)
{
}