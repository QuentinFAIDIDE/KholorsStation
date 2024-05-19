#pragma once

#include "PluginProcessor.h"

/**
 * @brief Class that describes the GUI of the plugin.
 */
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent &ev) override;
    void mouseDown(const juce::MouseEvent &ev) override;
    void mouseUp(const juce::MouseEvent &ev) override;

  private:
    AudioPluginAudioProcessor &processorRef; /**< Provided as an easy way to
                                                access the audio/midi processor. */

    // destroy copy constructors
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
