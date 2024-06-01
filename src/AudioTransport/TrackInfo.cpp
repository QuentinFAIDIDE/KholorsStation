#include "TrackInfo.h"
#include "AudioTransport.pb.h"
#include "ColorBytes.h"
#include "Constants.h"
#include <stdexcept>

namespace AudioTransport
{

void TrackInfo::parseFromApiPayload(AudioSegmentPayload *payload)
{
    if (payload == nullptr)
    {
        throw std::invalid_argument("nullptr payload passed to TrackInfo parseFromApiPayload");
    }

    identifier = payload->track_identifier();
    size_t nameSize = std::min(payload->track_name().size(), (size_t)MAXIMUM_TRACK_NAME_LENGTH);
    name = payload->track_name().substr(0, nameSize);

    ColorContainer colorBytesContainer(payload->track_color());
    redColorLevel = colorBytesContainer.red;
    greenColorLevel = colorBytesContainer.green;
    blueColorLevel = colorBytesContainer.blue;
    alphaColorLevel = colorBytesContainer.alpha;
}

bool TrackInfo::operator!=(const TrackInfo &o)
{
    return name != o.name || redColorLevel != o.redColorLevel || greenColorLevel != o.greenColorLevel ||
           blueColorLevel != o.blueColorLevel || alphaColorLevel != o.alphaColorLevel;
}

} // namespace AudioTransport