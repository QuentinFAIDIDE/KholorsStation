#include "AudioDataWorker.h"
#include "AudioTransport/AudioSegment.h"
#include "AudioTransport/DawInfo.h"
#include "AudioTransport/SyncServer.h"
#include "AudioTransport/TrackInfo.h"
#include "StationApp/Audio/BpmUpdateTask.h"
#include "StationApp/Audio/FftResultVectorReuseTask.h"
#include "StationApp/Audio/NewFftDataTask.h"
#include "StationApp/Audio/NewTrackVolumeDataTask.h"
#include "StationApp/Audio/ProcessingTimer.h"
#include "StationApp/Audio/TimeSignatureUpdateTask.h"
#include "StationApp/Audio/TrackInfoUpdateTask.h"
#include "TaskManagement/TaskingManager.h"
#include <memory>
#include <mutex>
#include <new>
#include <spdlog/spdlog.h>

#define NUM_AUDIO_WORKER_THREADS 2

AudioDataWorker::AudioDataWorker(AudioTransport::SyncServer &server, TaskingManager &tm)
    : shouldStop(false), taskingManager(tm), audioDataServer(server), processingTimerDelayMs(0)
{
    // create the worker threads
    for (size_t i = 0; i < NUM_AUDIO_WORKER_THREADS; i++)
    {
        dataProcessingThreads.emplace_back(std::thread(&AudioDataWorker::workerThreadLoop, this));
    }
    tm.registerTaskListener(this);
}

void AudioDataWorker::processAudioSegment(std::shared_ptr<AudioTransport::AudioSegment> audioSegment,
                                          std::shared_ptr<juce::AudioSampleBuffer> audioBuffer)
{
    try
    {
        audioBuffer->setSize(1, audioSegment->noAudioSamples, true, false, true);
    }
    catch (const std::bad_alloc &e)
    {
        spdlog::warn("Ignored an audio buffer after failing to allocate memory");
        return;
    }

    // NOTE: the audioSegment only contains one channel of data, but it contains
    // the number of channels of the original tracks, which helps figures out if
    // is mono and should be drawn on both channels on the UI.

    for (size_t i = 0; i < audioSegment->noAudioSamples; i++)
    {
        audioBuffer->setSample(0, (int)i, audioSegment->audioSamples[i]);
    }

    float volume = audioBuffer->getRMSLevel(0, 0, audioSegment->noAudioSamples);
    auto volumeUpdateTask = std::make_shared<NewTrackVolumeDataTask>(
        audioSegment->trackIdentifier, audioSegment->noChannels, audioSegment->channel,
        audioSegment->segmentStartSample, audioSegment->noAudioSamples, volume);
    taskingManager.broadcastTask(volumeUpdateTask);

    int numFFTs = fftProcessor.getNumFftFromNumSamples(audioSegment->noAudioSamples);
    auto shortTimeFFTs = fftProcessor.performFft(audioBuffer);

    auto newDataTask = std::make_shared<NewFftDataTask>(
        audioSegment->trackIdentifier, audioSegment->noChannels, audioSegment->channel, audioSegment->sampleRate,
        audioSegment->segmentStartSample, audioSegment->noAudioSamples, (uint32_t)numFFTs, shortTimeFFTs,
        audioSegment->payloadSentTimeMs);

    if (processingTimerDelayMs > MAX_AUDIO_SEGMENT_PROCESSING_DELAY_MS)
    {
        spdlog::warn("skipped a NewFftDataTask due to high processing delay");
        newDataTask->skip = true;
    }

    taskingManager.broadcastTask(newDataTask);
}

void AudioDataWorker::processTrackInfo(std::shared_ptr<AudioTransport::TrackInfo> trackInfo)
{
    auto trackudpateTask =
        std::make_shared<TrackInfoUpdateTask>(trackInfo->identifier, trackInfo->name, trackInfo->redColorLevel,
                                              trackInfo->greenColorLevel, trackInfo->blueColorLevel);
    taskingManager.broadcastTask(trackudpateTask);
}

void AudioDataWorker::processDawInfo(std::shared_ptr<AudioTransport::DawInfo> dawInfo)
{
    auto timeSignatureUpdate = std::make_shared<TimeSignatureUpdateTask>(dawInfo->timeSignatureNumerator);
    taskingManager.broadcastTask(timeSignatureUpdate);

    auto bpmUpdate = std::make_shared<BpmUpdateTask>(dawInfo->bpm);
    taskingManager.broadcastTask(bpmUpdate);
}

void AudioDataWorker::workerThreadLoop()
{
    auto audioBuffer = std::make_shared<juce::AudioSampleBuffer>();
    audioBuffer->setSize(1, AUDIO_SEGMENTS_BLOCK_SIZE);

    while (true)
    {
        {
            std::lock_guard lock(shouldStopMutex);
            if (shouldStop)
            {
                return;
            }
        }

        auto audioDataUpdate = audioDataServer.waitForDatum();
        if (audioDataUpdate.has_value())
        {
            if (auto audioSegment = std::dynamic_pointer_cast<AudioTransport::AudioSegment>(audioDataUpdate->datum))
            {
                processAudioSegment(audioSegment, audioBuffer);
            }
            else if (auto trackInfo = std::dynamic_pointer_cast<AudioTransport::TrackInfo>(audioDataUpdate->datum))
            {
                processTrackInfo(trackInfo);
            }
            else if (auto dawInfo = std::dynamic_pointer_cast<AudioTransport::DawInfo>(audioDataUpdate->datum))
            {
                processDawInfo(dawInfo);
            }
            audioDataServer.freeStoredDatum(audioDataUpdate->storageIdentifier);
        }
    }
}

bool AudioDataWorker::taskHandler(std::shared_ptr<Task> task)
{
    auto reuseVectorTask = std::dynamic_pointer_cast<FftResultVectorReuseTask>(task);
    if (reuseVectorTask != nullptr && !reuseVectorTask->isCompleted())
    {
        fftProcessor.reuseResultArray(reuseVectorTask->resultArray);
        reuseVectorTask->setCompleted(true);
        return true;
    }

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