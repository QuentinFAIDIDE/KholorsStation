#pragma once

#include <nlohmann/json.hpp>

#include "TaskManagement/Task.h"

/**
 * @brief Bpm Update task emitted upon DawInfo receivable by server AudioDataWorker.
 * Will be completed by FreqTimeView.
 */
class BpmUpdateTask : public SilentTask
{
  public:
    BpmUpdateTask(float nbpm)
    {
        bpm = nbpm;
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "bpm_update_task"},
                                {"bpm", bpm},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    float bpm; /**< bpm to set or that was set if completed */
};
