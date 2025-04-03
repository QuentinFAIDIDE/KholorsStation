#pragma once

namespace HeadlessAudioBroadcast
{
class AbstractFftProcessor
{
  public:
    /**
     * @brief Returns how many fft are covering an audio file
     *        with that much samples.
     *
     * @param numSamples The number of samples an audio files haves.
     * @return int The number of FFTs that will be returned for an audio file with that size.
     */
    static int getNumFftFromNumSamples(int numSamples);

    /**
     * @brief Perfom a (sequence of short) Fast Fourier Transform on an audio buffer and return its data.
     *
     * @param audioFile A JUCE library audio sample buffer with the audio samples inside.
     * @return std::shared_ptr<std::vector<float>>  A vector of resulting fourier transform.
     */
    std::shared_ptr<std::vector<float>> performFft(std::shared_ptr<juce::AudioSampleBuffer> audioFile);

    /**
     * @brief Reuse the vector returned by processJob for another processJob call.
     *
     * @param ptr a vector that processJob returned and that is not used anymore
     */
    void reuseResultArray(std::shared_ptr<std::vector<float>> ptr);
};
} // namespace HeadlessAudioBroadcast