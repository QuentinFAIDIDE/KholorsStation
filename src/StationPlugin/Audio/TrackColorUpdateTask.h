#pragma once

#include <nlohmann/json.hpp>

#include "TaskManagement/Task.h"

class TrackColorUpdateTask : public SilentTask
{
  public:
    TrackColorUpdateTask(uint64_t trackId, uint8_t red, uint8_t green, uint8_t blue)
    {
        identifier = trackId;
        redColorLevel = red;
        greenColorLevel = green;
        blueColorLevel = blue;
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "track_color_update_task"},
                                {"track_identifier", identifier},
                                {"red_color_level", redColorLevel},
                                {"green_color_level", greenColorLevel},
                                {"blue_color_level", blueColorLevel},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    uint64_t identifier;
    uint8_t redColorLevel;
    uint8_t greenColorLevel;
    uint8_t blueColorLevel;
};
