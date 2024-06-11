#pragma once

#include "SinkPlugin/BufferForwarder.h"
#include <juce_audio_processors/juce_audio_processors.h>

#define MAX_DB_BOOST 30.0f

/**
 * @brief Class that describes the audio plugin processing and GUI creation.
 *
 */
class AudioPluginAudioProcessor : public juce::AudioProcessor
{
  public:
    /**
     * @brief Construct a new Audio Plugin Audio Processor object
     */
    AudioPluginAudioProcessor();

    /**
     * @brief Destroy the Audio Plugin Audio Processor object
     *
     */
    ~AudioPluginAudioProcessor() override;

    /**
     * @brief Prepare the internal state of the class before playback starts.
     *
     * @param sampleRate Audio sample rate.
     * @param samplesPerBlock Ideally enforced audio block size (can be different
     * in some situations)
     */
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    /**
     * @brief Clear the internal state of the class after playback is stopped.
     */
    void releaseResources() override;

    /**
     * @brief Checks if the bus layout provided is supported.
     *
     * @param layouts Layout of the input/output busses.
     * @return true The layout is supported.
     * @return false The layout is not supported.
     */
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    /**
     * @brief Process a block of audio and midi.
     *
     * @param audioBuffer Block of audio samples.
     * @param midiBuffer Block of midi signal.
     */
    void processBlock(juce::AudioBuffer<float> &audioBuffer, juce::MidiBuffer &midiBuffer) override;

    using AudioProcessor::processBlock; /**< Prevent hiding the double parent
                                           function by the float one above */

    /**
     * @brief Instanciate a new GUI
     *
     * @return juce::AudioProcessorEditor* pointer to the created GUI class
     */
    juce::AudioProcessorEditor *createEditor() override;

    /**
     * @brief Does this plugin has GUI ?
     *
     * @return true this plugin has GUI
     * @return false this plugin does not has GUI
     */
    bool hasEditor() const override;

    /**
     * @brief Get the plugin name
     *
     * @return const juce::String plugin name
     */
    const juce::String getName() const override;

    /**
     * @brief Tells if the plugin accepts MIDI input
     *
     * @return true this plugin accepts midi input
     * @return false this plugin does not accepts midi input
     */
    bool acceptsMidi() const override;

    /**
     * @brief Tells if the plugin produce midi output
     *
     * @return true this plugin produces MIDI
     * @return false this plugin does not produces MIDI
     */
    bool producesMidi() const override;

    /**
     * @brief Tells if this plugin is a midi effect
     *
     * @return true this is a midi effect
     * @return false this is not a midi effect
     */
    bool isMidiEffect() const override;

    /**
     * @brief Get the tail of the processor in seconds.
     *
     * @return double length of the tail in seconds
     */
    double getTailLengthSeconds() const override;

    /**
     * @brief Unused program utility
     */
    int getNumPrograms() override;

    /**
     * @brief Unused program utility
     */
    int getCurrentProgram() override;

    /**
     * @brief Unused program utility
     */
    void setCurrentProgram(int index) override;

    /**
     * @brief Unused program utility
     */
    const juce::String getProgramName(int index) override;

    /**
     * @brief Unused program utility
     */
    void changeProgramName(int index, const juce::String &newName) override;

    /**
     * @brief Dump the plugin state to the destData byte array. Should stay
     * compatible with setStateInformation.
     *
     * @param destData
     */
    void getStateInformation(juce::MemoryBlock &destData) override;

    /**
     * @brief Parse the raw bytes and unmarshal it into the plugin state
     * (parameters). Should stay compatible with getStateInformation.
     *
     * @param data raw bytes where the desired software state is stored
     * @param sizeInBytes size of data in bytes
     */
    void setStateInformation(const void *data, int sizeInBytes) override;

    float getGainBoostDB() const;
    void setGainBoostDB(float);
    float getGainReductionDB() const;

  private:
    BufferForwarder audioInfoForwarder;

    // Juce utility to delete copy constructors
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
