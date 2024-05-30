#include "DawInfo.h"
#include "AudioTransport.pb.h"
#include <limits>
#include <stdexcept>

namespace AudioTransport
{

void DawInfo::parseFromApiPayload(AudioSegmentPayload *payload)
{
    if (payload == nullptr)
    {
        throw std::invalid_argument("nullptr payload passed to DawInfo parseFromApiPayload");
    }

    loopStartQuarterNotePos = payload->daw_loop_start();
    loopEndQuarterNotePos = payload->daw_loop_end();
    bpm = payload->daw_bpm();
    timeSignatureNumerator = payload->daw_time_signature_numerator();
    timeSignatureDenominator = payload->daw_time_signature_denominator();
    isLooping = payload->daw_is_looping();
}

bool DawInfo::operator!=(const DawInfo &o)
{
    return std::abs(loopEndQuarterNotePos - o.loopEndQuarterNotePos) > std::numeric_limits<double>::epsilon() ||
           std::abs(loopStartQuarterNotePos - o.loopStartQuarterNotePos) > std::numeric_limits<double>::epsilon() ||
           bpm != o.bpm || timeSignatureNumerator != o.timeSignatureNumerator ||
           timeSignatureDenominator != o.timeSignatureDenominator || isLooping != o.isLooping;
}

} // namespace AudioTransport