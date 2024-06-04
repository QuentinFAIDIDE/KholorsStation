#pragma once

#include "TaskManagement/Task.h"
#include <cstdint>

#include <nlohmann/json.hpp>

namespace AudioTransport
{

class ServerStatusTask : public Task
{
  public:
    ServerStatusTask(std::pair<bool, uint32_t> status) : runningAndPort(status)
    {
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "server_status_update"},
                                {"is_running", runningAndPort.first},
                                {"port", runningAndPort.second},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    std::pair<bool, uint32_t> runningAndPort; /**<first is true if running false otherwise, and second is port */
};
} // namespace AudioTransport