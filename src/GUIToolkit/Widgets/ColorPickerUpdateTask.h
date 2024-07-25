#pragma once

#include "TaskManagement/Task.h"
#include <memory>
#include <nlohmann/json.hpp>

/**
 * @brief Emitted when user or software want to update the color
 * picker color choice.
 * The colorpicker will then set the previous color to retain the ability
 * to undo this task.
 */
class ColorPickerUpdateTask : public Task
{
  public:
    ColorPickerUpdateTask(std::string identifier, uint8_t r, uint8_t g, uint8_t b)
        : colorPickerIdentifier(identifier), red(r), green(g), blue(b)
    {
    }

    /**
    Dumps the task data to a string as json
    */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "color_picker_update"},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"color_picker_id", colorPickerIdentifier},
                                {"red", (int)(red)},
                                {"green", (int)(green)},
                                {"blue", (int)(blue)},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    std::vector<std::shared_ptr<Task>> getOppositeTasks() override
    {
        auto rev =
            std::make_shared<ColorPickerUpdateTask>(colorPickerIdentifier, redPrevious, greenPrevious, bluePrevious);
        std::vector<std::shared_ptr<Task>> response;
        response.push_back(rev);
        return response;
    }

    std::string colorPickerIdentifier;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    uint8_t redPrevious;
    uint8_t greenPrevious;
    uint8_t bluePrevious;
};