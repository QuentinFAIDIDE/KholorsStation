#pragma once

#include "TaskManagement/Task.h"
#include <nlohmann/json.hpp>

/**
 * @brief Tells about current mouse cursor position and related info
 * to show.
 */
class MouseCursorInfoTask : public SilentTask
{
  public:
    MouseCursorInfoTask(bool showCursorParam, float freqUnderMouseParam, int64_t samplePositionParam)
    {
        cursorOnSfftView = showCursorParam;
        frequencyUnderMouse = freqUnderMouseParam;
        timeUnderMouse = samplePositionParam;
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "mouse_cursor_info_task"},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"frequency_under_cursor", frequencyUnderMouse},
                                {"time_under_cursor", timeUnderMouse},
                                {"cusor_on_sfft_view", cursorOnSfftView},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    float frequencyUnderMouse; /**< Frequency in Hz under the mouse cursor */
    int64_t timeUnderMouse;    /**< position in audio samples under mouse cursor (at VISUAL_FFT_RATE) */
    bool cursorOnSfftView;     /**< is the cursor currently undet the view (and therefore showing info required) */
};