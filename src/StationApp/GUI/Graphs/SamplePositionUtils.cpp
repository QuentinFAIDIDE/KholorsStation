#include "SamplePositionUtils.h"
#include "StationApp/GUI/AudioConstants.h"

void SamplePositionUtils::toVisualSampleRate(int64_t &startSample, int64_t &endSample, int64_t sourceSampleRate)
{
    if (sourceSampleRate != VISUAL_SAMPLE_RATE)
    {
        float sampleRateRatio = float(VISUAL_SAMPLE_RATE) / float(sourceSampleRate);
        startSample = float(startSample) * sampleRateRatio;
        endSample = float(endSample) * sampleRateRatio;
    }
}

void SamplePositionUtils::shiftToAlignWithOrigin(int64_t &startSample, int64_t &endSample)
{
    startSample += GRID_ALIGN_SAMPLE_POS_SHIFT;
    endSample += GRID_ALIGN_SAMPLE_POS_SHIFT;
}
