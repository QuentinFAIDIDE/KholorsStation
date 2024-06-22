#pragma once

#include "AudioTransport/ColorBytes.h"
#include "StationApp/Audio/TrackInfoUpdateTask.h"
#include "TaskManagement/TaskListener.h"
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
                    auto existingColor = colorsPerTrack.find(trackinfoUpdate->identifier)->second;
                    if (existingColor.red != trackinfoUpdate->redColorLevel ||
                        existingColor.green != trackinfoUpdate->greenColorLevel ||
                        existingColor.blue != trackinfoUpdate->blueColorLevel)
                    {
                        existingColor.red = trackinfoUpdate->redColorLevel;
                        existingColor.green = trackinfoUpdate->greenColorLevel,
                        existingColor.blue = trackinfoUpdate->blueColorLevel;
                    }
                }
                else
                {
                    auto emplacement_tuple = colorsPerTrack.emplace(
                        std::piecewise_construct, std::forward_as_tuple(trackinfoUpdate->identifier),
                        std::forward_as_tuple(trackinfoUpdate->redColorLevel, trackinfoUpdate->greenColorLevel,
                                              trackinfoUpdate->blueColorLevel, 0));
                    if (!emplacement_tuple.second)
                    {
                        throw std::runtime_error("failed to insert color into TrackInfoStore");
                    }
                }
            }

            trackinfoUpdate->setCompleted(true);
        }
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
};