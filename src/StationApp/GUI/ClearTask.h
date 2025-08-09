#pragma once

#include "TaskManagement/Task.h"
#include <nlohmann/json.hpp>

/**
 * @brief Clear the FreqTime view of all FFTs.
 */
class ClearTask : public Task
{
  public:
    ClearTask()
    {
    }

    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "clear_task"},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }
};