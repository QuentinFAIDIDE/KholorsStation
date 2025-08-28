#include "HelpButton.h"
#include "StationApp/GUI/HelpDialogContent.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"

#define KHOLORS_DOC_URL "https://kholors-doc.artifaktnd.com/"

HelpButton::HelpButton() : juce::TextButton("Help", "Open online user manual")
{
}

HelpButton::~HelpButton()
{
}

void HelpButton::clicked()
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Usage Tips";
    juce::OptionalScopedPointer<HelpDialogContent> contentContainer;
    options.resizable = false;
    options.useNativeTitleBar = false;
    options.content.set(new HelpDialogContent(), true);
    options.launchAsync();
}