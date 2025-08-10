#pragma once

#include "AudioTransport/SyncServer.h"
#include "StationApp/Audio/FftRunner.h"
#include "TaskManagement/TaskListener.h"
#include "TaskManagement/TaskingManager.h"
#include <memory>
#include <nlohmann/json.hpp>

#define MAX_AUDIO_SEGMENT_PROCESSING_DELAY_MS 500.0f

#define SIMPLE_PAYLOAD_CHECK_INTERVAL_MS 600000

class AudioDataWorker : public TaskListener
{
  public:
    /**
     * @brief Construct a new Audio Data Worker object
     *
     * @param server the server that receives audio data to read from
     * @param tm Tasking manager to send tasks to that were parsed from server
     */
    AudioDataWorker(AudioTransport::SyncServer &server, TaskingManager &tm);

    ~AudioDataWorker();

    /**
     * @brief Loop the worker threads will run until stopProcessing is called.
     */
    void workerThreadLoop();

    bool taskHandler(std::shared_ptr<Task> task) override;

  private:
    bool shouldStop;            /**< This will switch to true if we are waiting to stop the task processing */
    std::mutex shouldStopMutex; /**< Mutex to protect concurrent access of shouldStop variable */
    std::vector<std::thread> dataProcessingThreads; /**< threads that read data from server and emit tasks from it */
    std::mutex audioWorkerThreadMutex;              /**< protectd variables shared between audio processing threads */
    TaskingManager &taskingManager;                 /**< Tasking manager to emit new tasks */
    AudioTransport::SyncServer &audioDataServer;    /**< Audio server to read audio data from */
    FftRunner fftProcessor;                         /**< Multi threaded FFT processor */
    std::atomic<float> processingTimerDelayMs;      /**< last average audio segment processing delay */
};
