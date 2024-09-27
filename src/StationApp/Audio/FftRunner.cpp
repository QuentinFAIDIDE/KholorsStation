#include "FftRunner.h"
#include <cstring>
#include <fftw3.h>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>

using namespace std::chrono_literals;

FftRunner::FftRunner() : exiting(false)
{
    lowIntensityBounds =
        std::pow(10.0f, MIN_DB / 10.0f) / (HANN_AMPLITUDE_CORRECTION_FACTOR * HANN_AMPLITUDE_CORRECTION_FACTOR);
    highIntensityBounds = 1.0f / (HANN_AMPLITUDE_CORRECTION_FACTOR * HANN_AMPLITUDE_CORRECTION_FACTOR);

    noItensityF = float(FFT_INPUT_NO_INTENSITIES);

    // preallocate jobs data structures
    for (int i = 0; i < FFT_PREALLOCATED_JOB_STRUCTS; i++)
    {
        auto newEmptyJob = std::make_shared<FftRunnerJob>();
        emptyJobPool.push(newEmptyJob);
    }

    // precompute hanning windowing function based on fft windowing size
    hannWindowTable.resize(FFT_INPUT_NO_INTENSITIES);
    for (size_t i = 0; i < hannWindowTable.size(); i++)
    {
        hannWindowTable[i] = 0.5 * (1 - std::cos(2.0f * juce::MathConstants<float>::pi * (float)i / float(hannWindowTable.size() - 1)));
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
        std::scoped_lock<std::mutex> lock(queueMutex);
        exiting = true;
    }

    mutexCondition.notify_all();

    for (size_t i = 0; i < workerThreads.size(); i++)
    {
        workerThreads[i].join();
    }
    workerThreads.clear();
}

int FftRunner::getNumFftFromNumSamples(int numSamples)
{
    // how many non overlapping fft windows we can fit if we pad the end with zeros
    int numWindowsNoOverlap = std::ceil(float(numSamples) / float(FFT_INPUT_NO_INTENSITIES));
    // this formula get the exact amount of available overlapped bins.
    return (numWindowsNoOverlap * FFT_OVERLAP_DIVISION) - (FFT_OVERLAP_DIVISION - 1);
}

void FftRunner::reuseResultArray(std::shared_ptr<std::vector<float>> ptr)
{
    {
        std::lock_guard lock(resultsArrayMutex);
        freeResultsArrays.push(ptr);
    }
}

std::shared_ptr<std::vector<float>> FftRunner::performFft(std::shared_ptr<juce::AudioSampleBuffer> audioFile)
{
    // NOTE: one job = one fft

    // OPTIMIZATION: reuse result, wg and batchJobs to avoid allocating at every fft

    // number of jobs to send per channel
    int noJobsPerChannel = getNumFftFromNumSamples(audioFile->getNumSamples());

    // compute size (in # of floats!) and allocate response array
    int respArraySize = audioFile->getNumChannels() * noJobsPerChannel * FFT_OUTPUT_NO_FREQS;

    std::shared_ptr<std::vector<float>> result;
    {
        std::lock_guard lock(resultsArrayMutex);
        if (freeResultsArrays.size() > 0)
        {
            result = freeResultsArrays.front();
            freeResultsArrays.pop();
        }
        else
        {
            result = std::make_shared<std::vector<float>>();
        }
    }
    result->resize((size_t)respArraySize);

    // jobs sent in the current batch
    std::shared_ptr<std::vector<std::shared_ptr<FftRunnerJob>>> batchJobs;
    {
        std::lock_guard lock(jobListsMutex);
        if (freeJobLists.size() > 0)
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
    auto wg = std::make_shared<WaitGroup>();

    size_t windowPadding = ((size_t)FFT_INPUT_NO_INTENSITIES / (size_t)FFT_OVERLAP_DIVISION);

    size_t numSamples = (size_t)audioFile->getNumSamples();

    // repeat for each channel
    for (int ch = 0; ch < audioFile->getNumChannels(); ch++)
    {
        // channel offset in the destination array (result)
        size_t channelResultArrayOffset = (size_t)ch * (size_t)noJobsPerChannel * FFT_OUTPUT_NO_FREQS;

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
                if (maxJobsToPick > FFT_JOBS_BATCH_SIZE)
                {
                    maxJobsToPick = FFT_JOBS_BATCH_SIZE;
                }

                std::scoped_lock<std::mutex> lock(emptyJobsMutex);
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
            if (batchJobs->size() == 0)
            {
                std::cerr << "Too many threads are trying to perform ffts and buffered jobs could not handle batch "
                             "size for that amount of threads!"
                          << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
                continue;
            }

            // iterate over empty job and set the appropriate input data, length, and reset bool statuses
            std::shared_ptr<FftRunnerJob> *batchJobPtr = batchJobs->data();
            for (int i = 0; i < (int)batchJobs->size(); i++)
            {
                // set job properties and increment wg here
                FftRunnerJob *currentJob = (*batchJobPtr).get();
                currentJob->input = nextJobStart;
                currentJob->wg = wg;
                currentJob->position = fftPosition;
                wg->Add(1);
                // if our buffer will extend past end of channel, prevent it
                size_t windowStart = ((size_t)fftPosition * windowPadding);
                size_t windowEnd = windowStart + (FFT_INPUT_NO_INTENSITIES - 1);
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
                std::scoped_lock<std::mutex> lock(queueMutex);

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
            for (int i = 0; i < (int)batchJobs->size(); i++)
            {
                // helper to locate the destination area
                FftRunnerJob *currentJob = (*batchJobPtr).get();
                size_t fftChannelOffset = ((size_t)currentJob->position * FFT_OUTPUT_NO_FREQS);
                size_t fftResultArrayOffset = channelResultArrayOffset + fftChannelOffset;
                // copy the memory from job data to destination buffer
                memcpy(result->data() + fftResultArrayOffset, currentJob->output, sizeof(float) * FFT_OUTPUT_NO_FREQS);
                {
                    std::scoped_lock<std::mutex> lock(emptyJobsMutex);
                    emptyJobPool.push(*batchJobPtr);
                }
                batchJobPtr++;
            }

            // note that the batchJobs vector is cleared on loop restart
        }
    }

    // let's reuse this heap allocated vector
    {
        std::lock_guard lock(jobListsMutex);
        freeJobLists.push(batchJobs);
    }

    return result;
}

void FftRunner::fftThreadsLoop()
{
    // instanciate fftw objects
    float *fftInput;
    fftwf_complex *fftOutput;
    fftwf_plan fftwPlan;
    // allocate them and compute the plan
    {
        std::scoped_lock<std::mutex> lock(fftwMutex);
        fftInput = fftwf_alloc_real(FFTW_INPUT_SIZE);
        fftOutput = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * FFT_OUTPUT_NO_FREQS);
        fftwPlan = fftwf_plan_dft_r2c_1d(FFTW_INPUT_SIZE, fftInput, fftOutput, FFTW_PATIENT);
    }

    // Write zeros in input as zero padded part can stay untouched all along.
    // The job processing will only write the first FFT_INPUT_NO_INTENSITIES floats.
    for (size_t i = 0; i < FFTW_INPUT_SIZE; i++)
    {
        fftInput[i] = 0.0f;
    }

    while (true)
    {
        // try to pull a job on the queue
        std::shared_ptr<FftRunnerJob> nextJob = nullptr;
        {
            std::scoped_lock<std::mutex> lock(queueMutex);
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
            processJob(nextJob, &fftwPlan, fftInput, fftOutput);
        }
        else
        // if no more job on the queue, wait for condition variable
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            mutexCondition.wait(lock, [this] { return !todoJobQueue.empty() || exiting; });
            if (exiting)
            {
                // free the FFTW resources
                {
                    std::scoped_lock<std::mutex> lockFftw(fftwMutex);
                    fftwf_destroy_plan(fftwPlan);
                    fftwf_free(fftInput);
                    fftwf_free(fftOutput);
                }
                return;
            }
        }
    }
}

void FftRunner::processJob(std::shared_ptr<FftRunnerJob> job, fftwf_plan *plan, float *in, fftwf_complex *out)
{

    // copy data into the input
    memcpy(in, job->input, sizeof(float) * job->inputLength);
    // eventually pad rest of the input with zero if job input is not full size
    int diffToFullSize = FFT_INPUT_NO_INTENSITIES - job->inputLength;
    float *inPtr = in + (FFT_INPUT_NO_INTENSITIES - 1);
    if (diffToFullSize > 0)
    {
        for (size_t i = FFT_INPUT_NO_INTENSITIES - 1; i >= job->inputLength; --i)
        {
            *inPtr = 0.0f;
            inPtr--;
        }
    }
    // apply the hanning windowing function
    inPtr = in;
    float *hannPtr = hannWindowTable.data();
    for (size_t i = 0; i < FFT_INPUT_NO_INTENSITIES; ++i)
    {
        *inPtr = (*hannPtr) * (*inPtr);
        inPtr++;
        hannPtr++;
    }
    // execute the FFTW plan (and the DFT)
    fftwf_execute(*plan);
    // copy back the output intensities normalized
    float re, im, dist; /**< real and imaginary parts buffers */

    float *outPtr = job->output;

    for (size_t i = 0; i < FFT_OUTPUT_NO_FREQS; ++i)
    {
        // Read and normalize output complex.
        // Note that zero padding is not accounted for.
        re = out[i][0] / noItensityF;
        im = out[i][1] / noItensityF;
        // absolute value of the complex number
        dist = (re * re) + (im * im);
        if (dist <= lowIntensityBounds)
        {
            *outPtr = MIN_DB;
        }
        else if (dist >= highIntensityBounds)
        {
            *outPtr = 0.0f;
        }
        else
        {
            *outPtr = std::sqrt(dist) * HANN_AMPLITUDE_CORRECTION_FACTOR;
            *outPtr = 20.0f * std::log10(*outPtr);
        }
        outPtr++;
    }

    job->wg->Done();
}

///////////////////////////////////////////
