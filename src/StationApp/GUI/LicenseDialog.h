#pragma once

#include "GUIToolkit/IconsLoader.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

class LicenseDialog : public juce::ResizableWindow, juce::Button::Listener, juce::TextEditor::Listener
{
  public:
    LicenseDialog(juce::Component *parentComponent);
    void resized() override;
    /**
     * @brief      Closes self by trying to reach parent juce::DialogWindow.
     */
    void closeDialog();

    void paint(juce::Graphics &g) override;

    /**
     * @brief      Called when a button is clicked.
     *
     * @param      button  The button
     */
    void buttonClicked(juce::Button *button) override;

    /** Called when the user changes the text in some way. */
    void textEditorTextChanged(juce::TextEditor &) override;

    /** Called when the user presses the return key. */
    void textEditorReturnKeyPressed(juce::TextEditor &) override;

    void userTriedToCloseWindow() override;

  private:
    juce::SharedResourcePointer<IconsLoader> iconsLoader;
    juce::TextButton closeButton;
    juce::TextButton confirmButton;
    juce::TextEditor mailEntry;
    juce::TextEditor nameEntry;
    juce::TextEditor LicenseKeyEntry;
};