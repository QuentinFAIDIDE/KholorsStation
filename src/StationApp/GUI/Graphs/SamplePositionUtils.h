#pragma once

#include <cstdint>

/**
 * @brief Utilities to correct sample positions
 * based on sample rate and shift necessary to align the grid.
 *
 */
class SamplePositionUtils
{
  public:
    /**
     * @brief Modify startSample and endSample to move from sourceSampleRate to visualSampleRate.
     *
     * @param startSample
     * @param endSample
     * @param sourceSampleRate
     */
    void static toVisualSampleRate(int64_t &startSample, int64_t &endSample, int64_t sourceSampleRate);

    /**
     * @brief Shift the provided sample position in visual sample rate so the DAW origin aligns
     * with the 0 grid origin.
     *
     * @param startSample
     * @param endSample
     */
    void static shiftToAlignWithOrigin(int64_t &startSample, int64_t &endSample);
};