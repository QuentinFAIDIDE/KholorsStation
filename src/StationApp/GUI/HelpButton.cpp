#include "HelpButton.h"
#include "juce_core/juce_core.h"

#define KHOLORS_DOC_URL "https://kholors-doc.artifaktnd.com/"

HelpButton::HelpButton() : juce::TextButton("Help", "Open online user manual")
{
}

HelpButton::~HelpButton()
{
}

void HelpButton::clicked()
{
    juce::URL(KHOLORS_DOC_URL).launchInDefaultBrowser();
}