#pragma once

#include "AudioTransport/ColorBytes.h"
#include "StationApp/Audio/TrackColorUpdateTask.h"
#include "StationApp/Audio/TrackInfoUpdateTask.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>

class TrackInfoStore : public TaskListener
{
  public:
    explicit TrackInfoStore(TaskingManager &tm) : taskingManager(tm)
    {
    }

    bool taskHandler(std::shared_ptr<Task> task) override
    {
        auto const trackinfoUpdate = std::dynamic_pointer_cast<TrackInfoUpdateTask>(task);
        if (trackinfoUpdate != nullptr && !trackinfoUpdate->isCompleted() && !trackinfoUpdate->hasFailed())
        {
            {
                std::scoped_lock const lock(mutex);

                // update name
                if (namesPerTrack.contains(trackinfoUpdate->identifier))
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
                if (colorsPerTrack.contains(trackinfoUpdate->identifier))
                {
                    colorsPerTrack.erase(trackinfoUpdate->identifier);
                }

                auto const emplacement_tuple = colorsPerTrack.emplace(
                    std::piecewise_construct, std::forward_as_tuple(trackinfoUpdate->identifier),
                    std::forward_as_tuple(trackinfoUpdate->redColorLevel, trackinfoUpdate->greenColorLevel,
                                          trackinfoUpdate->blueColorLevel, 0));
                if (!emplacement_tuple.second)
                {
                    throw std::runtime_error("failed to insert color into TrackInfoStore");
                }

                auto const colorUpdateTask = std::make_shared<TrackColorUpdateTask>(
                    trackinfoUpdate->identifier, trackinfoUpdate->redColorLevel, trackinfoUpdate->greenColorLevel,
                    trackinfoUpdate->blueColorLevel);
                taskingManager.broadcastNestedTaskNow(colorUpdateTask);
            }

            trackinfoUpdate->setCompleted(true);
        }

        return false;
    }

    std::optional<AudioTransport::ColorContainer> getTrackColor(uint64_t trackId)
    {
        if (colorsPerTrack.contains(trackId))
        {
            return colorsPerTrack.find(trackId)->second;
        }

        return std::nullopt;
    }

    std::optional<std::string> getTrackName(uint64_t trackId)
    {
        if (namesPerTrack.contains(trackId))
        {
            return namesPerTrack.find(trackId)->second;
        }

        return std::nullopt;
    }

  private:
    std::map<uint64_t, AudioTransport::ColorContainer> colorsPerTrack;
    std::map<uint64_t, std::string> namesPerTrack;
    std::mutex mutex;
    TaskingManager &taskingManager;
};