#include "PluginProcessor.h"
#include "AudioTransport/Client.h"
#include "PluginEditor.h"
#include "SinkPlugin/BufferForwarder.h"
#include <cstddef>
#include <spdlog/spdlog.h>

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      audioTransportGrpcClient(DEFAULT_SERVER_PORT), audioInfoForwarder(audioTransportGrpcClient)
{
    auto uuid = juce::Uuid();
    audioInfoForwarder.setTrackInfo(uuid.hash());

    // set log level based on envar
    if (const char *logLevel = std::getenv("KHOLORS_LOG_LEVEL"))
    {
        if (std::strcmp(logLevel, "DEBUG") == 0)
        {
            spdlog::set_level(spdlog::level::debug);
            spdlog::debug("KHOLORS_LOG_LEVEL was set so the Kholors Sink plugin will show debug info");
        }
    }
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
    return false;
}

bool AudioPluginAudioProcessor::producesMidi() const
{
    return false;
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
    return false;
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

void AudioPluginAudioProcessor::prepareToPlay(double, int)
{
}

void AudioPluginAudioProcessor::releaseResources()
{
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    {
        return false;
    }
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    {
        return false;
    }

    return true;
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
    // a common thing to disabled for DSP, not sure if I should not remove it
    juce::ScopedNoDenormals noDenormals;

    // ignore sending batch of signals when used in an offline rendering mode
    if (isNonRealtime())
    {
        return;
    }

    // fetch daw info and abort if not available
    juce::AudioPlayHead *playHead = getPlayHead();
    if (playHead == nullptr)
    {
        audioInfoForwarder.setDawIsCompatible(false);
        return;
    }
    auto positionInfo = playHead->getPosition();
    if (!positionInfo.hasValue())
    {
        audioInfoForwarder.setDawIsCompatible(false);
        return;
    }

    if (!positionInfo->getIsPlaying())
    {
        return;
    }

    // get a pointer to a preallocated structure we can ship our audio data into
    std::shared_ptr<AudioBlockInfo> blockInfo = audioInfoForwarder.getFreeBlockInfoStruct();
    if (blockInfo == nullptr)
    {
        // we ignore audio if we are under stress
        spdlog::warn("Ignored an audio block because all AudioBlockInfo allocated are in use");
        return;
    }

    // all this information will be matched against last known values and shipped to the Station
    blockInfo->sampleRate = (int64_t)(getSampleRate() + 0.5);
    blockInfo->bpm = positionInfo->getBpm();
    blockInfo->timeSignature = positionInfo->getTimeSignature();
    blockInfo->isLooping = positionInfo->getIsLooping();
    blockInfo->isPlaying = positionInfo->getIsPlaying();
    blockInfo->loopBounds = positionInfo->getLoopPoints();
    blockInfo->numUsedSamples = 0;
    auto segmentStartSampleIfAvailable = positionInfo->getTimeInSamples();

    if (!segmentStartSampleIfAvailable.hasValue())
    {
        audioInfoForwarder.setDawIsCompatible(false);
        return;
    }
    blockInfo->startSample = *segmentStartSampleIfAvailable;

    blockInfo->numChannels = getTotalNumInputChannels();
    if (blockInfo->numChannels < buffer.getNumChannels())
    {
        audioInfoForwarder.setDawIsCompatible(false);
        return;
    }
    audioInfoForwarder.setDawIsCompatible(true);

    blockInfo->numTotalSamples = buffer.getNumSamples();

    // We then copy all audio samples one by one. Note than in theory, resize should
    // not have to allocate as we're suppose to reserve the size of the channelData vectors before.
    if (getTotalNumInputChannels() >= 1)
    {
        const float *firstChannelData = buffer.getReadPointer(0);
        blockInfo->firstChannelData.resize((size_t)blockInfo->numTotalSamples);
        for (size_t i = 0; i < (size_t)blockInfo->numTotalSamples; i++)
        {
            blockInfo->firstChannelData[i] = firstChannelData[i];
        }
    }
    if (getTotalNumInputChannels() >= 2)
    {
        const float *secondChannelData = buffer.getReadPointer(1);
        blockInfo->secondChannelData.resize((size_t)blockInfo->numTotalSamples);
        for (size_t i = 0; i < (size_t)blockInfo->numTotalSamples; i++)
        {
            blockInfo->secondChannelData[i] = secondChannelData[i];
        }
    }

    audioInfoForwarder.forwardAudioBlockInfo(blockInfo);
}

bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}