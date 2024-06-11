#include "BufferForwarder.h"

#define NUM_PREALLOCATED_BLOCKINFO 16
#define PREALLOCATED_BLOCKINFO_SAMPLE_SIZE 4096

BufferForwarder::BufferForwarder()
{
    // create the block info structs to pass to audio thread in order to fetch signal info
    preallocatedBlockInfo.resize(NUM_PREALLOCATED_BLOCKINFO);
    for (size_t i = 0; i < NUM_PREALLOCATED_BLOCKINFO; i++)
    {
        preallocatedBlockInfo[i].firstChannelData.reserve(PREALLOCATED_BLOCKINFO_SAMPLE_SIZE);
        preallocatedBlockInfo[i].secondChannelData.reserve(PREALLOCATED_BLOCKINFO_SAMPLE_SIZE);
        freeBlockInfos.push(i);
    }
}