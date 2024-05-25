#ifndef DEF_TRACK_INFO_HPP
#define DEF_TRACK_INFO_HPP

#include "AudioTransportData.h"
#include "Constants.h"
#include <cstdint>

namespace AudioTransport
{

/**
 * @brief Decribe a set of informations about
 * the current track.
 */
struct TrackInfo : public AudioTransportData
{
    uint64_t identifier;                  /**< Hash of the uuid of the track (provided by juce uuid implementatioon )*/
    char name[MAXIMUM_TRACK_NAME_LENGTH]; /**< Name of the track to display in the user interface */
    uint8_t redColorLevel;                /**< Amount of red in the track color */
    uint8_t greenColorLevel;              /**< Amount of green in the track color */
    uint8_t blueColorLevel;               /**< Amount of blue in the track color */
    uint8_t alphaColorLevel; /**< Amount of alpha in the track color (probably will be unused but is there for
                                maintaining 32 bits alignment ) */
};

} // namespace AudioTransport

#endif // DEF_TRACK_INFO_HPP