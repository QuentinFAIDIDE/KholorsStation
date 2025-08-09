#pragma once

#include <nlohmann/json.hpp>

#include "TaskManagement/Task.h"

/**
 * @brief This task trigger testing of a bogus license key
 * and assert that the test indeed fails.
 * Would it suceed, someone might have tricked the license checker
 * into always passing.
 * stage == 0 -> licensee should be included by BottomInfoLine, and the task rebroacasted
 * stage == 1 -> bogus check should be performed by FreqTimeView
 */
class BrokenLicenseCheckTask : public SilentTask
{
  public:
    BrokenLicenseCheckTask() : stage(0)
    {
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "time_sync_task"}, // not its real name lol
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    int stage;
    std::string username, email, key;
};