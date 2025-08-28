#pragma once

#include <cstdint>

/**
 * @brief Retains information about a track on a range
 * to be cleared.
 */
struct ClearTrackInfoRange
{
    uint64_t trackIdentifier;
    int64_t startSample;
    uint64_t length;
};