#pragma once

#include <nlohmann/json.hpp>
#include <optional>

#include "TaskManagement/Task.h"

/**
 * @brief Emmited whenever user select a track and we want to highlight it.
 */
class TrackSelectionTask : public SilentTask
{
  public:
    TrackSelectionTask(std::optional<uint64_t> selectedTrackParam) : selectedTrack(selectedTrackParam)
    {
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        uint64_t trackIdentifier = 0;
        if (selectedTrack.has_value())
        {
            trackIdentifier = selectedTrack.value();
        }
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "track_selection_task"},
                                {"track_identifier", trackIdentifier},
                                {"a_track_is_selected", selectedTrack.has_value()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    std::optional<uint64_t> selectedTrack;
};
