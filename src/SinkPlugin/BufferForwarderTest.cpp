#include "SinkPlugin/BufferForwarder.h"
#include "AudioTransport/MockedAudioSegmentPayloadSender.h"
#include <limits>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
int main(int, char **)
{
    AudioTransport::MockedAudioSegmentPayloadSender fakePayloadSender;
    BufferForwarder audioInfoForwarder(fakePayloadSender);

    spdlog::set_level(spdlog::level::debug);

    std::shared_ptr<AudioBlockInfo> blockInfo = audioInfoForwarder.getFreeBlockInfoStruct();
    blockInfo->bpm = 130;
    blockInfo->sampleRate = 44100;
    blockInfo->timeSignature = juce::Optional<juce::AudioPlayHead::TimeSignature>();
    blockInfo->isLooping = false;
    blockInfo->isPlaying = true;
    blockInfo->loopBounds = juce::Optional<juce::AudioPlayHead::LoopPoints>();
    blockInfo->numUsedSamples = 0;
    blockInfo->startSample = 450100;
    blockInfo->numChannels = 2;
    blockInfo->numTotalSamples = 3000;
    blockInfo->firstChannelData.resize(3000);
    blockInfo->secondChannelData.resize(3000);
    for (size_t i = 0; i < 3000; i++)
    {
        blockInfo->firstChannelData[i] = float(i) / 2.0f;
        blockInfo->secondChannelData[i] = float(i) / 5.0f;
    }
    audioInfoForwarder.forwardAudioBlockInfo(blockInfo);

    std::shared_ptr<AudioBlockInfo> blockInfo2 = audioInfoForwarder.getFreeBlockInfoStruct();
    blockInfo2->bpm = 130;
    blockInfo2->sampleRate = 44100;
    blockInfo2->timeSignature = juce::Optional<juce::AudioPlayHead::TimeSignature>();
    blockInfo2->isLooping = false;
    blockInfo2->isPlaying = true;
    blockInfo2->loopBounds = juce::Optional<juce::AudioPlayHead::LoopPoints>();
    blockInfo2->numUsedSamples = 0;
    blockInfo2->startSample = 453100;
    blockInfo2->numChannels = 2;
    blockInfo2->numTotalSamples = 3000;
    blockInfo2->firstChannelData.resize(3000);
    blockInfo2->secondChannelData.resize(3000);
    for (size_t i = 0; i < 3000; i++)
    {
        blockInfo2->firstChannelData[i] = float(i) / 2.0f;
        blockInfo2->secondChannelData[i] = float(i) / 5.0f;
    }
    audioInfoForwarder.forwardAudioBlockInfo(blockInfo2);

    sleep(2);

    // there should now be a merge of those two buffers
    auto segs = fakePayloadSender.getAllReceivedSegments();

    if (segs.size() != 1)
    {
        throw std::runtime_error("unexpected number of segments coalesced: " + std::to_string(segs.size()));
    }

    if (segs[0]->segment_sample_duration() != DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE)
    {
        throw std::runtime_error("unexpected number of samples in audio segment: " +
                                 std::to_string(segs[0]->segment_sample_duration()));
    }

    size_t channelShift = DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE;
    for (size_t i = 0; i < 3000; i++)
    {
        auto leftSample = segs[0]->segment_audio_samples()[i];
        auto rightSample = segs[0]->segment_audio_samples()[channelShift + i];

        if (std::abs(leftSample - float(i) / 2.0f) > std::numeric_limits<float>::epsilon())
        {
            std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - float(i) / 5.0f) > std::numeric_limits<float>::epsilon())
        {
            std::runtime_error("samples don't match");
        }
    }

    size_t remainingSpace = DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE - 3000;
    for (size_t i = 3000; i < remainingSpace; i++)
    {
        int iter = (int)i - 3000;
        auto leftSample = segs[0]->segment_audio_samples()[i];
        auto rightSample = segs[0]->segment_audio_samples()[channelShift + i];

        if (std::abs(leftSample - float(iter) / 2.0f) > std::numeric_limits<float>::epsilon())
        {
            std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - float(iter) / 5.0f) > std::numeric_limits<float>::epsilon())
        {
            std::runtime_error("samples don't match");
        }
    }

    spdlog::info("test passed");
}