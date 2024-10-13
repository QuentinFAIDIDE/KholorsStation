#include "SinkPlugin/BufferForwarder.h"
#include "AudioTransport/MockedAudioSegmentPayloadSender.h"
#include "TaskManagement/TaskingManager.h"
#include <limits>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

void testBufferForwarder01()
{
    TaskingManager tm;

    AudioTransport::MockedAudioSegmentPayloadSender fakePayloadSender;
    BufferForwarder audioInfoForwarder(fakePayloadSender, tm);

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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // there should now be a merge of those two buffers
    auto segs = fakePayloadSender.getAllReceivedSegments();

    if (segs.size() != 1)
    {
        throw std::runtime_error("1 unexpected number of segments coalesced: " + std::to_string(segs.size()));
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
            throw std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - float(i) / 5.0f) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    for (size_t i = 3000; i < DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE; i++)
    {
        int iter = (int)i - 3000;
        auto leftSample = segs[0]->segment_audio_samples()[i];
        auto rightSample = segs[0]->segment_audio_samples()[channelShift + i];

        if (std::abs(leftSample - float(iter) / 2.0f) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - float(iter) / 5.0f) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    // we send another but at a separate location and assert two get sent
    std::shared_ptr<AudioBlockInfo> blockInfo3 = audioInfoForwarder.getFreeBlockInfoStruct();
    blockInfo3->bpm = 130;
    blockInfo3->sampleRate = 44100;
    blockInfo3->timeSignature = juce::Optional<juce::AudioPlayHead::TimeSignature>();
    blockInfo3->isLooping = false;
    blockInfo3->isPlaying = true;
    blockInfo3->loopBounds = juce::Optional<juce::AudioPlayHead::LoopPoints>();
    blockInfo3->numUsedSamples = 0;
    blockInfo3->startSample = 483100;
    blockInfo3->numChannels = 2;
    blockInfo3->numTotalSamples = 4096;
    blockInfo3->firstChannelData.resize(4096);
    blockInfo3->secondChannelData.resize(4096);
    for (size_t i = 0; i < 4096; i++)
    {
        blockInfo3->firstChannelData[i] = float(i) / 2.0f;
        blockInfo3->secondChannelData[i] = float(i) / 5.0f;
    }
    audioInfoForwarder.forwardAudioBlockInfo(blockInfo3);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    segs = fakePayloadSender.getAllReceivedSegments();
    if (segs.size() != 3)
    {
        throw std::runtime_error("2 unexpected number of segments coalesced: " + std::to_string(segs.size()));
    }

    if (segs[1]->segment_sample_duration() != DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE)
    {
        throw std::runtime_error("unexpected number of samples in audio segment: " +
                                 std::to_string(segs[1]->segment_sample_duration()));
    }

    // segment zero should have a size of 4096, with 1904 samples from 1095 to 2999 times the factor
    for (size_t i = 1096; i < 3000; i++)
    {
        auto leftSample = segs[1]->segment_audio_samples()[i - 1096];
        auto rightSample = segs[1]->segment_audio_samples()[channelShift + i - 1096];
        if (std::abs(leftSample - float(i) / 2.0f) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - float(i) / 5.0f) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    // segment zero should have a size of 4096, with 1904 samples from 1095 to 2999 times the factor
    for (size_t i = 1904; i < 4096; i++)
    {
        auto leftSample = segs[1]->segment_audio_samples()[i];
        auto rightSample = segs[1]->segment_audio_samples()[channelShift + i];
        if (std::abs(leftSample) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    // other segment should be a perfect fill of the mocked iterated data from start to finish
    for (size_t i = 0; i < 4096; i++)
    {
        auto leftSample = segs[2]->segment_audio_samples()[i];
        auto rightSample = segs[2]->segment_audio_samples()[channelShift + i];
        if (std::abs(leftSample - (float(i) / 2.0f)) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - (float(i) / 5.0f)) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    // we send a last one
    // we send another but at a separate location and assert two get sent
    std::shared_ptr<AudioBlockInfo> blockInfo4 = audioInfoForwarder.getFreeBlockInfoStruct();
    blockInfo4->bpm = 130;
    blockInfo4->sampleRate = 44100;
    blockInfo4->timeSignature = juce::Optional<juce::AudioPlayHead::TimeSignature>();
    blockInfo4->isLooping = false;
    blockInfo4->isPlaying = true;
    blockInfo4->loopBounds = juce::Optional<juce::AudioPlayHead::LoopPoints>();
    blockInfo4->numUsedSamples = 0;
    blockInfo4->startSample = 483100;
    blockInfo4->numChannels = 2;
    blockInfo4->numTotalSamples = 4096;
    blockInfo4->firstChannelData.resize(3000);
    blockInfo4->secondChannelData.resize(3000);
    for (size_t i = 0; i < 4096; i++)
    {
        blockInfo4->firstChannelData[i] = float(i) / 2.0f;
        blockInfo4->secondChannelData[i] = float(i) / 5.0f;
    }
    audioInfoForwarder.forwardAudioBlockInfo(blockInfo4);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    segs = fakePayloadSender.getAllReceivedSegments();
    if (segs.size() != 4)
    {
        throw std::runtime_error("3 unexpected number of segments coalesced: " + std::to_string(segs.size()));
    }

    // other segment should be a perfect fill of the mocked iterated data from start to finish
    for (size_t i = 0; i < 4096; i++)
    {
        auto leftSample = segs[3]->segment_audio_samples()[i];
        auto rightSample = segs[3]->segment_audio_samples()[channelShift + i];
        if (std::abs(leftSample - (float(i) / 2.0f)) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - (float(i) / 5.0f)) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    spdlog::info("test passed");
}

void testBufferForwarder02()
{
    TaskingManager tm;

    AudioTransport::MockedAudioSegmentPayloadSender fakePayloadSender;
    BufferForwarder audioInfoForwarder(fakePayloadSender, tm);

    spdlog::set_level(spdlog::level::debug);

    for (size_t i = 0; i < 15; i++)
    {
        std::shared_ptr<AudioBlockInfo> blockInfo = audioInfoForwarder.getFreeBlockInfoStruct();
        blockInfo->bpm = 130;
        blockInfo->sampleRate = 44100;
        blockInfo->timeSignature = juce::Optional<juce::AudioPlayHead::TimeSignature>();
        blockInfo->isLooping = false;
        blockInfo->isPlaying = true;
        blockInfo->loopBounds = juce::Optional<juce::AudioPlayHead::LoopPoints>();
        blockInfo->numUsedSamples = 0;
        blockInfo->startSample = (int64_t)(700 * i);
        blockInfo->numChannels = 2;
        blockInfo->numTotalSamples = 700;
        blockInfo->firstChannelData.resize(700);
        blockInfo->secondChannelData.resize(700);
        for (size_t j = 0; j < 700; j++)
        {
            blockInfo->firstChannelData[j] = float((i * 700) + j) / 2.0f;
            blockInfo->secondChannelData[j] = float((i * 700) + j) / 5.0f;
        }
        audioInfoForwarder.forwardAudioBlockInfo(blockInfo);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto segs = fakePayloadSender.getAllReceivedSegments();
    if (segs.size() != 2)
    {
        throw std::runtime_error("unexpected number of segments coalesced (!=2): " + std::to_string(segs.size()));
    }

    size_t channelShift = DEFAULT_AUDIO_SEGMENT_CHANNEL_SIZE;

    // other segment should be a perfect fill of the mocked iterated data from start to finish
    for (size_t i = 0; i < 4096; i++)
    {
        auto leftSample = segs[0]->segment_audio_samples()[i];
        auto rightSample = segs[0]->segment_audio_samples()[channelShift + i];
        if (std::abs(leftSample - (float(i) / 2.0f)) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - (float(i) / 5.0f)) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }
    for (size_t i = 0; i < 4096; i++)
    {
        auto leftSample = segs[1]->segment_audio_samples()[i];
        auto rightSample = segs[1]->segment_audio_samples()[channelShift + i];
        if (std::abs(leftSample - (float(4096 + i) / 2.0f)) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }

        if (std::abs(rightSample - (float(4096 + i) / 5.0f)) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }
}

int main(int, char **)
{
    // testing the basic stereo buffer coalescing
    testBufferForwarder01();
    testBufferForwarder02();
}