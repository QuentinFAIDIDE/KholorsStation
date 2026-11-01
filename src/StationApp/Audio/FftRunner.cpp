#include "FftRunner.h"
#include "fft.h"
#include "fft_internal.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <math.h>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

FftRunner::FftRunner()
{
    lowIntensityBounds =
        std::pow(10.0f, MIN_DB / 10.0f) / (HANN_AMPLITUDE_CORRECTION_FACTOR * HANN_AMPLITUDE_CORRECTION_FACTOR);
    highIntensityBounds = 1.0f / (HANN_AMPLITUDE_CORRECTION_FACTOR * HANN_AMPLITUDE_CORRECTION_FACTOR);

    invNoIntensityF = 1.0f / static_cast<float>(FFT_INPUT_NO_INTENSITIES);

    precomputedHannAmplitudeCorrectionShift = 20.0f * std::log10(HANN_AMPLITUDE_CORRECTION_FACTOR);

    // preallocate jobs data structures
    for (int i = 0; i < FFT_PREALLOCATED_JOB_STRUCTS; i++)
    {
        auto const newEmptyJob = std::make_shared<FftRunnerJob>();
        emptyJobPool.push(newEmptyJob);
    }

    // precompute hanning windowing function based on fft windowing size
    hannWindowTable.resize(FFT_INPUT_NO_INTENSITIES);
    for (size_t i = 0; i < hannWindowTable.size(); i++)
    {
        hannWindowTable[i] = 0.5 * (1 - std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i) /
                                                 static_cast<float>(hannWindowTable.size() - 1)));
    }

    // Pick the number of threads and start them.
    // Copy pasted from the post linked in the header file, it's already perfect like this.
    const uint32_t num_threads = std::thread::hardware_concurrency(); // Max # of threads the system supports
    for (uint32_t ii = 0; ii < num_threads; ++ii)
    {
        workerThreads.emplace_back(std::thread(&FftRunner::fftThreadsLoop, this));
    }
}

FftRunner::~FftRunner()
{
    {
        std::scoped_lock<std::mutex> const lock(queueMutex);
        exiting = true;
    }

    mutexCondition.notify_all();

    for (auto &workerThread : workerThreads)
    {
        workerThread.join();
    }
    workerThreads.clear();
}

int FftRunner::getNumFftFromNumSamples(int numSamples)
{
    // how many non overlapping fft windows we can fit if we pad the end with zeros
    int const numWindowsNoOverlap =
        std::ceil(static_cast<float>(numSamples) / static_cast<float>(FFT_INPUT_NO_INTENSITIES));
    // this formula get the exact amount of available overlapped bins.
    return (numWindowsNoOverlap * FFT_OVERLAP_DIVISION) - (FFT_OVERLAP_DIVISION - 1);
}

void FftRunner::reuseResultArray(std::shared_ptr<std::vector<float>> ptr)
{
    {
        std::scoped_lock const lock(resultsArrayMutex);
        freeResultsArrays.push(ptr);
    }
}

const float *FftRunner::getTiltTable(uint32_t sampleRate)
{
    std::scoped_lock const lock(tiltTablesMutex);
    auto existingTable = tiltTablesBySampleRate.find(sampleRate);
    if (existingTable != tiltTablesBySampleRate.end())
    {
        return existingTable->second.data();
    }

    std::vector<float> tiltTable(FFT_OUTPUT_NO_FREQS, 0.0f);
    for (size_t i = 0; i < FFT_OUTPUT_NO_FREQS; i++)
    {
        float const binFrequency = static_cast<float>(i) * static_cast<float>(sampleRate) / FFT_INPUT_SIZE;
        if (binFrequency > FFT_TILT_PIVOT_HZ)
        {
            tiltTable[i] = FFT_TILT_DB_PER_OCTAVE * std::log2(binFrequency / FFT_TILT_PIVOT_HZ);
        }
    }
    return tiltTablesBySampleRate.emplace(sampleRate, std::move(tiltTable)).first->second.data();
}

std::shared_ptr<std::vector<float>> FftRunner::performFft(std::shared_ptr<juce::AudioSampleBuffer> audioFile,
                                                          uint32_t sampleRate)
{
    const float *tiltDb = getTiltTable(sampleRate);

    // NOTE: one job = one fft

    // OPTIMIZATION: reuse result, wg and batchJobs to avoid allocating at every fft

    // number of jobs to send per channel
    int const noJobsPerChannel = getNumFftFromNumSamples(audioFile->getNumSamples());

    // compute size (in # of floats!) and allocate response array
    int const respArraySize = audioFile->getNumChannels() * noJobsPerChannel * FFT_OUTPUT_NO_FREQS;

    std::shared_ptr<std::vector<float>> result;
    {
        std::scoped_lock const lock(resultsArrayMutex);
        if (!freeResultsArrays.empty())
        {
            result = freeResultsArrays.front();
            freeResultsArrays.pop();
        }
        else
        {
            result = std::make_shared<std::vector<float>>();
        }
    }
    result->resize(static_cast<size_t>(respArraySize));

    // jobs sent in the current batch
    std::shared_ptr<std::vector<std::shared_ptr<FftRunnerJob>>> batchJobs;
    {
        std::scoped_lock const lock(jobListsMutex);
        if (!freeJobLists.empty())
        {
            batchJobs = freeJobLists.front();
            freeJobLists.pop();
        }
        else
        {
            batchJobs = std::make_shared<std::vector<std::shared_ptr<FftRunnerJob>>>();
        }
    }
    batchJobs->resize(0);
    batchJobs->reserve(FFT_JOBS_BATCH_SIZE);

    // waitgroup for successive batches of jobs
    auto const wg = std::make_shared<WaitGroup>();

    size_t const windowPadding =
        (static_cast<size_t>(FFT_INPUT_NO_INTENSITIES) / static_cast<size_t>(FFT_OVERLAP_DIVISION));

    auto const numSamples = static_cast<size_t>(audioFile->getNumSamples());

    // repeat for each channel
    for (int ch = 0; ch < audioFile->getNumChannels(); ch++)
    {
        // channel offset in the destination array (result)
        size_t const channelResultArrayOffset =
            static_cast<size_t>(ch) * static_cast<size_t>(noJobsPerChannel) * FFT_OUTPUT_NO_FREQS;

        // pointer to the start of the next job
        const float *nextJobStart = audioFile->getReadPointer(ch);

        // total jobs still to be sent for this channel
        int remainingJobs = noJobsPerChannel;

        // index of the fft in posted jobs
        int fftPosition = 0;

        // keep sending batch of jobs to thread while we are not finished
        while (remainingJobs != 0)
        {
            // first get as many preallocated jobs as possible
            batchJobs->clear();
            {
                int maxJobsToPick = remainingJobs;
                maxJobsToPick = std::min(maxJobsToPick, FFT_JOBS_BATCH_SIZE);

                std::scoped_lock<std::mutex> const lock(emptyJobsMutex);
                for (int i = 0; i < maxJobsToPick; ++i)
                {
                    if (emptyJobPool.empty())
                    {
                        break;
                    }
                    batchJobs->emplace_back(emptyJobPool.front());
                    emptyJobPool.pop();
                }
            }

            // If we have not been able to pull any empty job, emit a warning and sleep
            // Note that this can happen if more than FFT_PREALLOCATED_JOB_STRUCTS / FFT_JOBS_BATCH_SIZE
            // threads are racing to use the preallocated empty jobs. But as of now only one thread is requesting FFTs
            // so t's safe to assume that it should not happen.
            if (batchJobs->empty())
            {
                std::cerr << "Too many threads are trying to perform ffts and buffered jobs could not handle batch "
                             "size for that amount of threads!"
                          << '\n';
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                continue;
            }

            // iterate over empty job and set the appropriate input data, length, and reset bool statuses
            std::shared_ptr<FftRunnerJob> const *batchJobPtr = batchJobs->data();
            for (int i = 0; std::cmp_less(i, batchJobs->size()); i++)
            {
                // set job properties and increment wg here
                FftRunnerJob *currentJob = (*batchJobPtr).get();
                currentJob->input = nextJobStart;
                currentJob->wg = wg;
                currentJob->tiltDb = tiltDb;
                currentJob->position = fftPosition;
                wg->Add(1);
                // if our buffer will extend past end of channel, prevent it
                size_t const windowStart = (static_cast<size_t>(fftPosition) * windowPadding);
                size_t const windowEnd = windowStart + (FFT_INPUT_NO_INTENSITIES - 1);
                if (windowEnd >= numSamples)
                {
                    currentJob->inputLength = numSamples - windowStart;
                }
                // if buffer will not overflow, use the full size
                else
                {
                    currentJob->inputLength = FFT_INPUT_NO_INTENSITIES;
                }
                // decrement remaining job and increment position pointer
                remainingJobs--;
                fftPosition++;
                // move the data pointer forward
                nextJobStart += windowPadding;
                // iterate to next batchJobPtr
                batchJobPtr++;
            }

            // push all jobs on the todo queue
            {
                std::scoped_lock<std::mutex> const lock(queueMutex);

                batchJobPtr = batchJobs->data();
                for (size_t i = 0; i < batchJobs->size(); i++)
                {
                    todoJobQueue.push(*batchJobPtr);
                    batchJobPtr++;
                }
            }

            // here, we notify the threads that work have been pushed
            mutexCondition.notify_all();

            // wait for waitgroup
            wg->Wait();

            // copy back the responses once we're done
            batchJobPtr = batchJobs->data();
            for (int i = 0; std::cmp_less(i, batchJobs->size()); i++)
            {
                // helper to locate the destination area
                FftRunnerJob *currentJob = (*batchJobPtr).get();
                size_t const fftChannelOffset = (static_cast<size_t>(currentJob->position) * FFT_OUTPUT_NO_FREQS);
                size_t const fftResultArrayOffset = channelResultArrayOffset + fftChannelOffset;
                // copy the memory from job data to destination buffer
                memcpy(result->data() + fftResultArrayOffset, currentJob->output, sizeof(float) * FFT_OUTPUT_NO_FREQS);
                {
                    std::scoped_lock<std::mutex> const lock(emptyJobsMutex);
                    emptyJobPool.push(*batchJobPtr);
                }
                batchJobPtr++;
            }

            // note that the batchJobs vector is cleared on loop restart
        }
    }

    // let's reuse this heap allocated vector
    {
        std::scoped_lock const lock(jobListsMutex);
        freeJobLists.push(batchJobs);
    }

    return result;
}

void FftRunner::fftThreadsLoop()
{
    // instanciate mufft objects
    float *fftInput = nullptr;
    cfloat *fftOutput = nullptr;
    mufft_plan_1d *mufftPlan = nullptr;
    // allocate them and compute the plan
    {
        std::scoped_lock<std::mutex> const lock(fftMutex);
        fftInput = static_cast<float *>(mufft_alloc(FFT_INPUT_SIZE * sizeof(float)));
        fftOutput = static_cast<cfloat *>(mufft_alloc(FFT_OUTPUT_NO_FREQS * sizeof(cfloat)));
        mufftPlan = mufft_create_plan_1d_r2c(FFT_INPUT_SIZE, MUFFT_FLAG_CPU_ANY);
        if (mufftPlan == nullptr)
        {
            throw std::runtime_error("Unable to initialize muFFT plan, is the FFT_INPUT_SIZE not a power of two ?");
        }
    }

    // Write zeros in input as zero padded part can stay untouched all along.
    // The job processing will only write the first FFT_INPUT_NO_INTENSITIES floats.
    for (size_t i = 0; i < FFT_INPUT_SIZE; i++)
    {
        fftInput[i] = 0.0f;
    }

    while (true)
    {
        // try to pull a job on the queue
        std::shared_ptr<FftRunnerJob> nextJob = nullptr;
        {
            std::scoped_lock<std::mutex> const lock(queueMutex);
            if (!todoJobQueue.empty())
            {
                nextJob = todoJobQueue.front();
                todoJobQueue.pop();
            }
        }

        // if there is one, process it
        if (nextJob != nullptr)
        {
            // note that the processJob function will
            // call the WaitGroup pointer at by the job
            // to notify the job poster that is currently waiting.
            processJob(nextJob, mufftPlan, fftInput, fftOutput);
        }
        else
        // if no more job on the queue, wait for condition variable
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            mutexCondition.wait(lock, [this] { return !todoJobQueue.empty() || exiting; });
            if (exiting)
            {
                // free the muFFT resources
                {
                    std::scoped_lock<std::mutex> const lockFft(fftMutex);
                    mufft_free_plan_1d(mufftPlan);
                    mufft_free(fftInput);
                    mufft_free(fftOutput);
                }
                return;
            }
        }
    }
}

void FftRunner::processJob(std::shared_ptr<FftRunnerJob> job, mufft_plan_1d *plan, float *in, cfloat *out)
{

    // copy data into the input
    memcpy(in, job->input, sizeof(float) * job->inputLength);
    // eventually pad rest of the input with zero if job input is not full size
    int const diffToFullSize = FFT_INPUT_NO_INTENSITIES - job->inputLength;
    float *inPtr = in + (FFT_INPUT_NO_INTENSITIES - 1);
    if (diffToFullSize > 0)
    {
        spdlog::warn("Received segment has a size not aligned zith FFT size!");
        for (size_t i = FFT_INPUT_NO_INTENSITIES - 1; i >= job->inputLength; --i)
        {
            *inPtr = 0.0f;
            inPtr--;
        }
    }
    // apply the hanning windowing function
    inPtr = in;
    float const *hannPtr = hannWindowTable.data();
    for (size_t i = 0; i < FFT_INPUT_NO_INTENSITIES >> 3; ++i)
    {
        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;

        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;

        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;

        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;

        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;

        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;

        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;

        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;
    }
    // execute the muFFT plan (and the DFT)
    mufft_execute_plan_1d(plan, out, in);
    // copy back the output intensities normalized
    float re = NAN;
    float im = NAN;
    float dist = NAN; /**< real and imaginary parts buffers */

    float *outPtr = job->output;
    const float *tiltPtr = job->tiltDb;

    for (size_t i = 0; i < FFT_OUTPUT_NO_FREQS; ++i)
    {
        // Read and normalize output complex.
        // Note that zero padding is not accounted for.
        re = out[i].real * invNoIntensityF;
        im = out[i].imag * invNoIntensityF;
        // absolute value of the complex number
        dist = (re * re) + (im * im);
        if (dist <= lowIntensityBounds)
        {
            *outPtr = MIN_DB;
        }
        else if (dist < highIntensityBounds)
        {
            *outPtr = std::min(0.0f, (10.0f * std::log10(dist)) + precomputedHannAmplitudeCorrectionShift + tiltPtr[i]);
        }
        else
        {
            *outPtr = 0.0f;
        }

        outPtr++;
    }

    job->wg->Done();
}

///////////////////////////////////////////
