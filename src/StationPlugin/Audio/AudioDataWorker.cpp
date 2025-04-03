#include "AudioDataWorker.h"
#include "AudioTransport/AudioSegment.h"
#include "AudioTransport/DawInfo.h"
#include "AudioTransport/SyncServer.h"
#include "AudioTransport/TrackInfo.h"
#include "StationPlugin/Audio/BpmUpdateTask.h"
#include "StationPlugin/Audio/FftResultVectorReuseTask.h"
#include "StationPlugin/Audio/NewFftDataTask.h"
#include "StationPlugin/Audio/ProcessingTimer.h"
#include "StationPlugin/Audio/TimeSignatureUpdateTask.h"
#include "StationPlugin/Audio/TrackInfoUpdateTask.h"
#include "TaskManagement/TaskingManager.h"
#include <memory>
#include <mutex>
#include <new>
#include <spdlog/spdlog.h>

#define NUM_AUDIO_WORKER_THREADS 2

AudioDataWorker::AudioDataWorker(AudioTransport::SyncServer &server, TaskingManager &tm)
    : shouldStop(false), taskingManager(tm), audioDataServer(server), processingTimerDelayMs(0)
{
    lastSimpleLicenseCheck = 0;
    // create the worker threads
    for (size_t i = 0; i < NUM_AUDIO_WORKER_THREADS; i++)
    {
        dataProcessingThreads.emplace_back(std::thread(&AudioDataWorker::workerThreadLoop, this));
    }
    tm.registerTaskListener(this);
}

void AudioDataWorker::workerThreadLoop()
{
    auto audioBuffer = std::make_shared<juce::AudioSampleBuffer>();
    audioBuffer->setSize(1, AUDIO_SEGMENTS_BLOCK_SIZE);

    // loop on requesting server for audio transport data
    while (true)
    {
        // if shouldStop is true, abort
        {
            std::lock_guard lock(shouldStopMutex);
            if (shouldStop)
            {
                return;
            }
        }
        // poll on an audio data update
        auto audioDataUpdate = audioDataServer.waitForDatum();
        if (audioDataUpdate.has_value())
        {
            // if it's an audio segment, perform SFFT and update cursor position
            std::shared_ptr<AudioTransport::AudioSegment> audioSegment =
                std::dynamic_pointer_cast<AudioTransport::AudioSegment>(audioDataUpdate->datum);
            if (audioSegment != nullptr)
            {
                // eventually resize the buffer that will hold the data we perform FFT on. Should not do anything most
                // of the time as we preallocated with the AUDIO_SEGMENTS_BLOCK_SIZE at thread start.
                try
                {
                    audioBuffer->setSize(1, audioSegment->noAudioSamples, true, false, true);
                }
                catch (const std::bad_alloc &e)
                {
                    spdlog::warn("Ignored an audio buffer after failing to allocate memory");
                    audioDataServer.freeStoredDatum(audioDataUpdate->storageIdentifier);
                    continue;
                }

                // copy the audio data from the server received buffer to the buffer on which we perform FFT
                for (size_t i = 0; i < audioSegment->noAudioSamples; i++)
                {
                    audioBuffer->setSample(0, (int)i, audioSegment->audioSamples[i]);
                }

                // perform SFFTs
                int numFFTs = fftProcessor.getNumFftFromNumSamples(audioSegment->noAudioSamples);
                auto shortTimeFFTs = fftProcessor.performFft(audioBuffer);

                // emit a task with the new data to be added to the visualizer
                auto newDataTask = std::make_shared<NewFftDataTask>(
                    audioSegment->trackIdentifier, audioSegment->noChannels, audioSegment->channel,
                    audioSegment->sampleRate, audioSegment->segmentStartSample, audioSegment->noAudioSamples,
                    (uint32_t)numFFTs, shortTimeFFTs, audioSegment->payloadSentTimeMs);

                // if the delay is too severe, skip processing this audio segment
                if (processingTimerDelayMs > MAX_AUDIO_SEGMENT_PROCESSING_DELAY_MS)
                {
                    spdlog::warn("skipped a NewFftDataTask due to high processing delay");
                    newDataTask->skip = true;
                }

                taskingManager.broadcastTask(newDataTask);

                /*
                // this part safely ensures that the simple license check tasks are emmited.
                {
                    std::lock_guard lock(audioWorkerThreadMutex);

                    if (lastSimpleLicenseCheck == 0)
                    {
                        lastSimpleLicenseCheck = audioSegment->payloadSentTimeMs;
                    }

                    // every X minutes, trigger a license check
                    if ((audioSegment->payloadSentTimeMs - lastSimpleLicenseCheck) > SIMPLE_PAYLOAD_CHECK_INTERVAL_MS)
                    {
                        lastSimpleLicenseCheck = audioSegment->payloadSentTimeMs;
                        auto newSimpleLicenseCheckTask = std::make_shared<SimpleLicenseCheckTask>();
                        taskingManager.broadcastTask(newSimpleLicenseCheckTask);
                    }
                }
                */
            }
            // if it's a TrackInfo, copy it and emit a task
            auto trackInfo = std::dynamic_pointer_cast<AudioTransport::TrackInfo>(audioDataUpdate->datum);
            if (trackInfo != nullptr)
            {
                auto trackudpateTask = std::make_shared<TrackInfoUpdateTask>(
                    trackInfo->identifier, trackInfo->name, trackInfo->redColorLevel, trackInfo->greenColorLevel,
                    trackInfo->blueColorLevel);
                taskingManager.broadcastTask(trackudpateTask);
            }
            // if it's a DawInfo, copy it and emit related tasks
            auto dawInfo = std::dynamic_pointer_cast<AudioTransport::DawInfo>(audioDataUpdate->datum);
            if (dawInfo != nullptr)
            {
                auto timeSignatureUpdate = std::make_shared<TimeSignatureUpdateTask>(dawInfo->timeSignatureNumerator);
                taskingManager.broadcastTask(timeSignatureUpdate);

                auto bpmUpdate = std::make_shared<BpmUpdateTask>(dawInfo->bpm);
                taskingManager.broadcastTask(bpmUpdate);
            }
            audioDataServer.freeStoredDatum(audioDataUpdate->storageIdentifier);
        }
    }
}

bool AudioDataWorker::taskHandler(std::shared_ptr<Task> task)
{
    auto processingTimerDelayUpdate = std::dynamic_pointer_cast<ProcessingTimeUpdateTask>(task);
    if (processingTimerDelayUpdate != nullptr)
    {
        processingTimerDelayMs = processingTimerDelayUpdate->averageProcesingTimeMs;
    }

    return false;
}

AudioDataWorker::~AudioDataWorker()
{
    // take lock and set shouldStop to true
    {
        std::lock_guard lock(shouldStopMutex);
        shouldStop = true;
    }
    // for each thread, join it
    for (size_t i = 0; i < dataProcessingThreads.size(); i++)
    {
        dataProcessingThreads[i].join();
    }
}