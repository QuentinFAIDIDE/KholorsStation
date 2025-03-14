#include "LicenseDialog.h"
#include "GUIToolkit/Consts.h"
#include "StationPlugin/Licensing/DummyLicenseManager.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "spdlog/spdlog.h"

#define SIDE_MARGINS 18
#define TEXT_ENTRIES_PADDING 14
#define TOP_PADDING 12
#define TEXT_ENTRIES_WIDTH 300
#define TEXT_ENTRIES_HEIGHT 40
#define BUTTONS_WIDTH 60
#define BUTTONS_AREA_HEIGHT 44
#define BUTTONS_PADDING 8
#define TITLESPACE_HEIGHT 80
#define TITLE_ENTRIES_PADDING 4

#define INSTRUCTIONS_HEIGHTS 60

#define DIALOG_WIDTH 450
#define DIALOG_HEIGHT 400

LicenseDialog::LicenseDialog(juce::Component *parentComponent)
    : juce::ResizableWindow("License Dialog", KHOLORS_COLOR_BACKGROUND, parentComponent == nullptr)
{
    closeButton.setColour(juce::TextButton::ColourIds::textColourOnId, KHOLORS_COLOR_TEXT);
    closeButton.addListener(this);
    closeButton.setButtonText("Quit");
    addAndMakeVisible(&closeButton);

    confirmButton.setColour(juce::TextButton::ColourIds::textColourOnId, KHOLORS_COLOR_TEXT);
    confirmButton.addListener(this);
    confirmButton.setButtonText("Ok");
    addAndMakeVisible(&confirmButton);

    mailEntry.setFont(juce::Font(18));
    mailEntry.setColour(juce::TextEditor::ColourIds::textColourId, KHOLORS_COLOR_BLACK);
    mailEntry.setColour(juce::TextEditor::ColourIds::backgroundColourId, KHOLORS_COLOR_INPUT_BACKGROUND);
    mailEntry.setJustification(juce::Justification::centredLeft);
    mailEntry.setTextToShowWhenEmpty("Email...", KHOLORS_COLOR_BLACK.brighter(0.5f));

    nameEntry.setFont(juce::Font(18));
    nameEntry.setColour(juce::TextEditor::ColourIds::textColourId, KHOLORS_COLOR_BLACK);
    nameEntry.setColour(juce::TextEditor::ColourIds::backgroundColourId, KHOLORS_COLOR_INPUT_BACKGROUND);
    nameEntry.setJustification(juce::Justification::centredLeft);
    nameEntry.setTextToShowWhenEmpty("Name or Username...", KHOLORS_COLOR_BLACK.brighter(0.5f));

    LicenseKeyEntry.setFont(juce::Font(18));
    LicenseKeyEntry.setColour(juce::TextEditor::ColourIds::textColourId, KHOLORS_COLOR_BLACK);
    LicenseKeyEntry.setColour(juce::TextEditor::ColourIds::backgroundColourId, KHOLORS_COLOR_INPUT_BACKGROUND);
    LicenseKeyEntry.setJustification(juce::Justification::centredLeft);
    LicenseKeyEntry.setTextToShowWhenEmpty("License key...", KHOLORS_COLOR_BLACK.brighter(0.5f));

    addAndMakeVisible(&mailEntry);
    addAndMakeVisible(&nameEntry);
    addAndMakeVisible(&LicenseKeyEntry);
    setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
    setResizable(false, false);
    if (parentComponent != nullptr)
    {
        parentComponent->addAndMakeVisible(this);
    }
    else
    {
        setAlwaysOnTop(true);
    }
}

void LicenseDialog::paint(juce::Graphics &g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto localBounds = getLocalBounds();
    localBounds.removeFromLeft(SIDE_MARGINS);
    localBounds.removeFromRight(SIDE_MARGINS);
    localBounds.removeFromTop(TOP_PADDING);

    auto titleSpace = localBounds.removeFromTop(TITLESPACE_HEIGHT);
    titleSpace.removeFromTop(TOP_PADDING);
    iconsLoader->artifaktNdLogo->drawWithin(g, titleSpace.toFloat(), juce::RectanglePlacement::centred, 1.0f);

    auto textSpace = localBounds.removeFromTop(INSTRUCTIONS_HEIGHTS);
    g.setFont(18);
    g.setColour(KHOLORS_COLOR_TEXT);
    g.drawText("Please enter your KholorsStation license key.", textSpace, juce::Justification::centred, false);
}

void LicenseDialog::userTriedToCloseWindow()
{
}

void LicenseDialog::resized()
{
    auto localBounds = getLocalBounds();
    localBounds.removeFromTop(TITLESPACE_HEIGHT + TITLE_ENTRIES_PADDING + INSTRUCTIONS_HEIGHTS);

    // position the two buttons at the bottom
    auto buttonsArea = localBounds.removeFromBottom(BUTTONS_AREA_HEIGHT);
    buttonsArea.removeFromBottom(TOP_PADDING);
    buttonsArea.removeFromRight(SIDE_MARGINS);
    confirmButton.setBounds(buttonsArea.removeFromRight(BUTTONS_WIDTH));
    buttonsArea.removeFromRight(BUTTONS_PADDING);
    closeButton.setBounds(buttonsArea.removeFromRight(BUTTONS_WIDTH));

    // position the text entries from top to bottom
    localBounds.removeFromLeft(SIDE_MARGINS);
    localBounds.removeFromRight(SIDE_MARGINS);
    localBounds.removeFromTop(TOP_PADDING);

    mailEntry.setBounds(localBounds.removeFromTop(TEXT_ENTRIES_HEIGHT));
    localBounds.removeFromTop(TEXT_ENTRIES_PADDING);
    nameEntry.setBounds(localBounds.removeFromTop(TEXT_ENTRIES_HEIGHT));
    localBounds.removeFromTop(TEXT_ENTRIES_PADDING);
    LicenseKeyEntry.setBounds(localBounds.removeFromTop(TEXT_ENTRIES_HEIGHT));

    centreWithSize(getWidth(), getHeight());
}

void LicenseDialog::closeDialog()
{
}

void LicenseDialog::buttonClicked(juce::Button *button)
{
    if (button == &confirmButton)
    {
        // test if the key is okay
        UserDataAndKey data;
        data.email = mailEntry.getText().toStdString();
        data.username = nameEntry.getText().toStdString();
        data.licenseKey = LicenseKeyEntry.getText().toStdString();
        try
        {
            // if the key is invalid, dismiss
            if (!DummyLicenseManager::isKeyValid(data))
            {
                return;
            }
            // try to write the key file
            DummyLicenseManager::writeUserDataAndKeyToDisk(data);
            // if it's okay, close
            exitModalState(0);
        }
        catch (...)
        {
            spdlog::warn("Key license validation did not work");
        }
    }
    if (button == &closeButton)
    {
        exitModalState(1);
    }
}

void LicenseDialog::textEditorTextChanged(juce::TextEditor &)
{
}

void LicenseDialog::textEditorReturnKeyPressed(juce::TextEditor &)
{
}
