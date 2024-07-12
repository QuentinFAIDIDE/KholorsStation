#pragma once

#include "GUIToolkit/FontsLoader.h"
#include "GUIToolkit/IconsLoader.h"
#include "GUIToolkit/KholorsLookAndFeel.h"
#include "GUIToolkit/Widgets/ColorPicker.h"
#include "GUIToolkit/Widgets/TextEntry.h"
#include "PluginProcessor.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"

/**
 * @brief Class that describes the GUI of the plugin.
 */
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &, TaskingManager &, juce::Colour);
    ~AudioPluginAudioProcessorEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent &ev) override;
    void mouseDown(const juce::MouseEvent &ev) override;
    void mouseUp(const juce::MouseEvent &ev) override;

  private:
    void drawHeader(juce::Graphics &g);

    AudioPluginAudioProcessor &processorRef; /**< Provided as an easy way to
                                                access the audio/midi processor. */
    juce::Typeface::Ptr typeface;            /**< Default font of the app */
    KholorsLookAndFeel appLookAndFeel;       /**< Defines the app look and feel */

    juce::SharedResourcePointer<IconsLoader> sharedSvgs; /**< singleton object that loads svg images */
    juce::SharedResourcePointer<FontsLoader> sharedFonts;

    TaskingManager &taskManager;
    ColorPicker colorPicker;
    TextEntry textEntry;

    int64_t taskListenerId; /** Used to remove the task listener when the GUI is destructed */

    // destroy copy constructors
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
