#pragma once

#include <nlohmann/json.hpp>

#include "TaskManagement/Task.h"

/**
 * @brief This task will serve as a randomly emmited task for checking
 * the license key, and eventually deleting the file and closing the software if it's invalid.
 */
class SimpleLicenseCheckTask : public SilentTask
{
  public:
    SimpleLicenseCheckTask()
    {
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "simple_license_checking_task"},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }
};