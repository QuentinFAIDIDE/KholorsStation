#include "PluginEditor.h"
#include "PluginProcessor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(650, 143);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a
    // solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::mouseDrag(const juce::MouseEvent &)
{
}

void AudioPluginAudioProcessorEditor::mouseDown(const juce::MouseEvent &)
{
}

void AudioPluginAudioProcessorEditor::mouseUp(const juce::MouseEvent &)
{
}

void AudioPluginAudioProcessorEditor::resized()
{
    // this plugin can't really be resized and we don't have much layout to do
}
