#pragma once

#include "AudioTransport.pb.h"
#include <memory>
#include <mutex>
#include <thread>

#include "juce_audio_basics/juce_audio_basics.h"

class BufferForwarder
{
  public:
    BufferForwarder();

    struct AudioBlockInfo
    {
        int64_t nonce;                        /**< Monotically increasing sequence number */
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

    /**
     * @brief Get a preallocated BlockInfo struct to copy audio block data into;
     *
     * @return std::shared_ptr<AudioBlockInfo> a shared_ptr with the nonce value set where
     * audio block data should be copied.
     * It should not be used anymore after forwardAudioBlockInfo is called.
     */
    std::shared_ptr<AudioBlockInfo> getFreeBlockInfoStruct();

    /**
     * @brief Pass the AudioBlockInfo onto the thread responsible for shipping audio data to the Station.
     * @param blockInfo collection of daw and buffer info to be shipped to the station, that will be
     * considered not used anymore by the called (audio thread) and will be reused in getFreeBlockInfoStruct once
     * processed.
     */
    void forwardAudioBlockInfo(std::shared_ptr<AudioBlockInfo> blockInfo);

    /**
     * @brief Set the track identifier this forwarder is responsible for.
     *
     * @param trackIdentifier track identifier, preferrably generated from hash of UUID at plugin startup.
     */
    void setTrackIdentifier(uint64_t trackIdentifier);

    /**
     * @brief Set the boolean telling if the daw is compatible with what we wanna do or not.
     * If the daw is not compatible, we will still ship data but with the compatibility flag to flag
     * to ignore the data.
     *
     * @param isCompatible true if the daw is compatible, false otherwise.
     */
    void setDawIsCompatible(bool isCompatible);

  private:
    std::shared_ptr<std::thread> coalescerSenderThread;

    std::vector<AudioBlockInfo> preallocatedBlockInfo;
    std::vector<AudioTransport::AudioSegmentPayload> preallocatedPayloads;

    std::queue<size_t> freeBlockInfos;
    std::mutex freeBlockInfosMutex;
    std::queue<size_t> blockInfosToCoalesce;
    std::queue<size_t> payloadsToSend;

    std::atomic<uint64_t> trackIdentifier;
    std::atomic<uint8_t> trackColorRed;
    std::atomic<uint8_t> trackColorGreen;
    std::atomic<uint8_t> trackColorBlue;
    std::atomic<uint8_t> trackColorAlpha;
    std::string trackName;
    std::mutex trackNameMutex;

    std::atomic<bool> dawIsCompatible;
};