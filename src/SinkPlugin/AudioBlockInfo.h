#pragma once

#include <cstdint>
#include <vector>

#include "juce_audio_basics/juce_audio_basics.h"

struct AudioBlockInfo
{
    size_t storageId;                     /**< Index of this buffer in the storage array */
    int64_t sampleRate;                   /**< Sample rate of the daw */
    int64_t startSample;                  /**< Start sample of this segment */
    int32_t numChannels;                  /**< Number of channels of this track */
    int32_t numSamples;                   /**< Number of samples in this audio segment */
    std::vector<float> firstChannelData;  /**< left channel audio samples */
    std::vector<float> secondChannelData; /**< right channel audio samples, if numChan==1 => size==0 */
    juce::Optional<double> bpm;           /**< beats per minutes of the daw */
    juce::Optional<juce::AudioPlayHead::TimeSignature> timeSignature; /**< DAW time signature (ex 4/4) */
    juce::Optional<juce::AudioPlayHead::LoopPoints> loopBounds;       /**< option loops upper and lower bounds*/
    bool isLooping;                                                   /**< true if the daw is currently looping */
    bool isPlaying;                                                   /**< true if the daw is currently playing */
};
