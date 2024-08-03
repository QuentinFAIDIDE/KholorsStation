#pragma once

#include "TaskManagement/Task.h"
#include <nlohmann/json.hpp>

/**
 * @brief This job is emmited once we finished using the result array
 * provided by FftRunner's performFFT. It will then be reused.
 *
 */
class FftResultVectorReuseTask : public SilentTask
{
  public:
    FftResultVectorReuseTask(std::shared_ptr<std::vector<float>> vectorToReuse) : resultArray(vectorToReuse)
    {
    }

    /**
     * Dumps the task data to a string as json
     */
    std::string marshal() override
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "fft_result_vector_reuse_task"},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    std::shared_ptr<std::vector<float>> resultArray;
};