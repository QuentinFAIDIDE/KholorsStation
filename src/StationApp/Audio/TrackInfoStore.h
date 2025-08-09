#pragma once

#include "AudioTransport/ColorBytes.h"
#include "StationApp/Audio/BrokenLicenseCheckTask.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/TrackColorUpdateTask.h"
#include "StationApp/Audio/TrackInfoUpdateTask.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>

#define MIN_LICENSE_CHECK_INTERVAL_MS (60000 * 15)

class TrackInfoStore : public TaskListener
{
  public:
    TrackInfoStore(TaskingManager &tm) : taskingManager(tm)
    {
        lastLicenseCheck = 0;
    }

    bool taskHandler(std::shared_ptr<Task> task)
    {
        auto trackinfoUpdate = std::dynamic_pointer_cast<TrackInfoUpdateTask>(task);
        if (trackinfoUpdate != nullptr && !trackinfoUpdate->isCompleted() && !trackinfoUpdate->hasFailed())
        {
            {
                std::lock_guard lock(mutex);

                // update name
                if (namesPerTrack.find(trackinfoUpdate->identifier) != namesPerTrack.end())
                {
                    if (trackinfoUpdate->name != namesPerTrack.find(trackinfoUpdate->identifier)->second)
                    {
                        namesPerTrack[trackinfoUpdate->identifier] = trackinfoUpdate->name;
                    }
                }
                else
                {
                    namesPerTrack[trackinfoUpdate->identifier] = trackinfoUpdate->name;
                }

                // update color
                if (colorsPerTrack.find(trackinfoUpdate->identifier) != colorsPerTrack.end())
                {
                    colorsPerTrack.erase(trackinfoUpdate->identifier);
                }

                auto emplacement_tuple = colorsPerTrack.emplace(
                    std::piecewise_construct, std::forward_as_tuple(trackinfoUpdate->identifier),
                    std::forward_as_tuple(trackinfoUpdate->redColorLevel, trackinfoUpdate->greenColorLevel,
                                          trackinfoUpdate->blueColorLevel, 0));
                if (!emplacement_tuple.second)
                {
                    throw std::runtime_error("failed to insert color into TrackInfoStore");
                }

                auto colorUpdateTask = std::make_shared<TrackColorUpdateTask>(
                    trackinfoUpdate->identifier, trackinfoUpdate->redColorLevel, trackinfoUpdate->greenColorLevel,
                    trackinfoUpdate->blueColorLevel);
                taskingManager.broadcastNestedTaskNow(colorUpdateTask);
            }

            trackinfoUpdate->setCompleted(true);
        }

        /*

        // spy on new fft data tasks (that have a timestamp) to initiate license
        // checks
        auto newFftDataTask = std::dynamic_pointer_cast<NewFftDataTask>(task);
        if (newFftDataTask != nullptr && !newFftDataTask->isCompleted() && !newFftDataTask->hasFailed())
        {
            // we only process on fftDataTask every 500 iterations
            iterCheck++;
            if (iterCheck % 500 != 0)
            {
                return false;
            }
            if (lastLicenseCheck == 0)
            {
                lastLicenseCheck = newFftDataTask->sentTimeUnixMs;
                return false;
            }
            // this check can only happen if days since unix epoch start are == 0 [mod] 3
            int64_t daysSinceUnixEpoch = newFftDataTask->sentTimeUnixMs / (60000 * 60 * 24);
            if (daysSinceUnixEpoch % 3 == 0)
            {
                if ((newFftDataTask->sentTimeUnixMs - lastLicenseCheck) > MIN_LICENSE_CHECK_INTERVAL_MS)
                {
                    lastLicenseCheck = newFftDataTask->sentTimeUnixMs;
                    auto newLicenseCheckTask = std::make_shared<BrokenLicenseCheckTask>();
                    taskingManager.broadcastTask(newLicenseCheckTask);
                }
            }
        }

        */

        return false;
    }

    std::optional<AudioTransport::ColorContainer> getTrackColor(uint64_t trackId)
    {
        if (colorsPerTrack.find(trackId) != colorsPerTrack.end())
        {
            return colorsPerTrack.find(trackId)->second;
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<std::string> getTrackName(uint64_t trackId)
    {
        if (namesPerTrack.find(trackId) != namesPerTrack.end())
        {
            return namesPerTrack.find(trackId)->second;
        }
        else
        {
            return std::nullopt;
        }
    }

  private:
    std::map<uint64_t, AudioTransport::ColorContainer> colorsPerTrack;
    std::map<uint64_t, std::string> namesPerTrack;
    std::mutex mutex;
    TaskingManager &taskingManager;
    int64_t lastLicenseCheck;
    int64_t iterCheck;
};