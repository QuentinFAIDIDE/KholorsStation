#pragma once

#include "TaskManagement/Task.h"
#include <nlohmann/json.hpp>

class TimeSignatureUpdateTask : public SilentTask
{
  public:
    TimeSignatureUpdateTask(int pnumerator)
    {
        numerator = pnumerator;
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "time_signature_update_task"},
                                {"time_signature_numerator", numerator},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    int numerator;
};