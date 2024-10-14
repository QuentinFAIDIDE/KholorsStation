#pragma once

#include <nlohmann/json.hpp>

#include "TaskManagement/Task.h"

class VolumeSensitivityTask : public SilentTask
{
  public:
    VolumeSensitivityTask(float newSensitivity)
    {
        sensitivity = newSensitivity;
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "volume_sensitivity_update_task"},
                                {"sensitivity", sensitivity},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    float sensitivity;
};