#include "AudioSegment.h"
#include <memory>

#include "Constants.h"

namespace AudioTransport
{

AudioSegment::AudioSegment()
{
    audioSamples = std::make_shared<std::vector<float>>();
    audioSamples->resize(AUDIO_SEGMENTS_BLOCK_SIZE);
}

} // namespace AudioTransport