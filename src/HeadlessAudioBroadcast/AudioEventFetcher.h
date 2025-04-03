#pragma once

#include "HeadlessAudioBroadcast/TrackInfo.h"
#include "TaskManagement/Task.h"
#include <memory>
#include <vector>

namespace HeadlessAudioBroadcast
{

class AudioEventFetcher
{
  public:
    virtual std::vector<std::shared_ptr<Task>> getNextAudioEvents() = 0;
    virtual std::vector<TrackInfo> getAllKnownTrackInfos() = 0;

    /**
     * @brief Get the last known BPM of the DAW.
     *
     * @return float beats per minute of the daw.
     */
    virtual float getKnownBpm() = 0;

    /**
     * @brief Return the last known time signature numerator.
     *
     * @return int time signature numerator.
     */
    virtual int getKnownTimeSignature() = 0;
};

}; // namespace HeadlessAudioBroadcast