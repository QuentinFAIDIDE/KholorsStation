#pragma once

#include "AudioTransport.pb.h"
#include "AudioTransportData.h"
#include <cstdint>

namespace AudioTransport
{

/**
 * @brief Holds information about DAW settings.
 *
 */
struct DawInfo : public AudioTransportData
{
    void parseFromApiPayload(const AudioSegmentPayload *payload);
    bool operator!=(const DawInfo &o);

    double loopStartQuarterNotePos;    /**< Start position of the loop in quarter notes fractions */
    double loopEndQuarterNotePos;      /**< End position of the loop in quarter notes fractions */
    uint32_t bpm;                      /**< Beats per minute in the daw */
    uint32_t timeSignatureNumerator;   /**< Time signature numerator from the daw */
    uint32_t timeSignatureDenominator; /**< Time signature denominator in the daw */
    bool isLooping;                    /**< is the daw currently looping ? */
};

} // namespace AudioTransport
