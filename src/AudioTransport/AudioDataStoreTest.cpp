#include "AudioDataStoreTest.h"
#include "AudioDataStore.h"
#include "AudioSegment.h"
#include "AudioTransport.pb.h"
#include "ColorBytes.h"
#include "TrackInfo.h"
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>

using namespace AudioTransport;

void AudioDataStoreTestSuite::runAll()
{
    testPreallocation01();
    testParse01();
}

void AudioDataStoreTestSuite::testPreallocation01()
{
    AudioDataStore store(10);

    auto storesSize = store.countFreePreallocatedStructs();
    if (storesSize.size() != 3)
    {
        throw std::runtime_error("Invalid storeSize");
    }
    for (size_t i = 0; i < 3; i++)
    {
        if (storesSize[i] != 10)
        {
            throw std::runtime_error("unexpected size of store");
        }
    }

    // this should not change the store size
    auto a = store.reserveAudioSegment();
    if (!a.has_value())
    {
        throw std::runtime_error("unable to allocate AudioSegment");
    }
    if (store.countFreePreallocatedStructs()[0] != 9)
    {
        throw std::runtime_error("reserving did not free audio segment from set");
    }
    store.freeStoredDatum(a->storageIdentifier);

    auto b = store.reserveDawInfo();
    if (!b.has_value())
    {
        throw std::runtime_error("unable to allocate DawInfo");
    }
    if (store.countFreePreallocatedStructs()[1] != 9)
    {
        throw std::runtime_error("reserving did not free audio segment from set");
    }
    store.freeStoredDatum(b->storageIdentifier);

    auto c = store.reserveTrackInfo();
    if (!c.has_value())
    {
        throw std::runtime_error("unable to allocate TrackInfo");
    }
    if (store.countFreePreallocatedStructs()[2] != 9)
    {
        throw std::runtime_error("reserving did not free audio segment from set");
    }
    store.freeStoredDatum(c->storageIdentifier);

    // we verify store capacity is at its maximum
    storesSize = store.countFreePreallocatedStructs();
    if (storesSize.size() != 3)
    {
        throw std::runtime_error("Invalid storeSize");
    }
    for (size_t i = 0; i < 3; i++)
    {
        if (storesSize[i] != 10)
        {
            throw std::runtime_error("unexpected size of store");
        }
    }

    // drain all available audio buffers and assert it works
    for (size_t i = 0; i < 10; i++)
    {
        auto reservedSegment = store.reserveAudioSegment();
        if (!reservedSegment.has_value())
        {
            throw std::runtime_error("Unable to drain as much audio segments as there is available");
        }
    }
    // assert that the next alloc fail
    auto reservedSegment = store.reserveAudioSegment();
    if (reservedSegment.has_value())
    {
        throw std::runtime_error("Drained all available AudioSegment but still could allocate");
    }

    // drain all available audio buffers and assert it works
    for (size_t i = 0; i < 10; i++)
    {
        auto reservedInfo = store.reserveDawInfo();
        if (!reservedInfo.has_value())
        {
            throw std::runtime_error("Unable to drain as much daw info as there is available");
        }
    }
    // assert that the next alloc fail
    auto reservedInfo = store.reserveDawInfo();
    if (reservedInfo.has_value())
    {
        throw std::runtime_error("Drained all available DawInfo but still could allocate");
    }

    // drain all available audio buffers and assert it works
    for (size_t i = 0; i < 10; i++)
    {
        auto reservedInfo = store.reserveTrackInfo();
        if (!reservedInfo.has_value())
        {
            throw std::runtime_error("Unable to drain as much track info as there is available");
        }
    }
    // assert that the next alloc fail
    reservedInfo = store.reserveTrackInfo();
    if (reservedInfo.has_value())
    {
        throw std::runtime_error("Drained all available TrackInfo but still could allocate");
    }
}

void AudioDataStoreTestSuite::testParse01()
{

    AudioDataStore store(10);

    std::vector<float> data(2000 * 2);
    for (size_t i = 0; i < 4000; i++)
    {
        data[i] = (float)i;
    }

    ColorContainer a = ColorContainer(10, 20, 30, 40);

    AudioSegmentPayload payload;
    payload.set_track_identifier(1);
    payload.set_track_color(a.toColorBytes());
    payload.set_track_name("track number 1");
    payload.set_daw_sample_rate(48000);
    payload.set_daw_bpm(125);
    payload.set_daw_time_signature_denominator(1);
    payload.set_daw_time_signature_numerator(4);
    payload.set_daw_is_looping(false);
    payload.set_daw_loop_start(10);
    payload.set_daw_loop_end(20);
    payload.set_segment_start_sample(100);
    payload.set_segment_sample_duration(2000);
    payload.set_segment_no_channels(2);
    for (int i = 0; i < 4000; i++)
    {
        payload.add_segment_audio_samples(data[i]);
    }

    // assert that there is nothing in the queue
    {
        std::lock_guard<std::mutex> lock(store.pendingAudioDataMutex);
        if (store.pendingAudioData.size() != 0)
        {
            throw std::runtime_error("queue is not empty");
        }
    }

    store.parseNewData(&payload);

    auto datum1 = store.waitForDatum();
    auto datum2 = store.waitForDatum();
    auto datum3 = store.waitForDatum();
    auto datum4 = store.waitForDatum();

    {
        std::lock_guard<std::mutex> lock(store.pendingAudioDataMutex);
        if (store.pendingAudioData.size() != 0)
        {
            throw std::runtime_error("Size of queue is not 3 after the 3 first events");
        }
    }

    auto datum1Segment = std::dynamic_pointer_cast<AudioSegment>(datum1->datum);
    if (datum1Segment == nullptr)
    {
        throw std::runtime_error("d1 not an AudioSegment");
    }
    if (datum1Segment->trackIdentifier != 1)
    {
        throw std::runtime_error("d1 wrong identifier");
    }
    if (datum1Segment->channel != 0)
    {
        throw std::runtime_error("d1 wrong channel");
    }
    if (datum1Segment->noChannels != 2)
    {
        throw std::runtime_error("d1 wrong noChannels");
    }
    if (datum1Segment->sampleRate != 48000)
    {
        throw std::runtime_error("d1 wrong sampleRate");
    }
    if (datum1Segment->segmentStartSample != 100)
    {
        throw std::runtime_error("d1 wrong segmentStartSample");
    }
    if (datum1Segment->noAudioSamples != 2000)
    {
        throw std::runtime_error("d1 wrong noAudioSamples");
    }
    for (size_t i = 0; i < 2000; i++)
    {
        if (std::abs((float)i - datum1Segment->audioSamples[i]) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    auto datum2Segment = std::dynamic_pointer_cast<AudioSegment>(datum2->datum);
    if (datum2Segment == nullptr)
    {
        throw std::runtime_error("d2 not AudioSegment");
    }
    if (datum2Segment->trackIdentifier != 1)
    {
        throw std::runtime_error("d2 wrong identifier");
    }
    if (datum2Segment->channel != 1)
    {
        throw std::runtime_error("d2 wrong channel");
    }
    if (datum2Segment->noChannels != 2)
    {
        throw std::runtime_error("d2 wrong noChannels");
    }
    if (datum2Segment->sampleRate != 48000)
    {
        throw std::runtime_error("d2 wrong sampleRate");
    }
    if (datum2Segment->segmentStartSample != 100)
    {
        throw std::runtime_error("d2 wrong segmentStartSample");
    }
    if (datum2Segment->noAudioSamples != 2000)
    {
        throw std::runtime_error("d2 wrong noAudioSamples");
    }
    for (size_t i = 0; i < 2000; i++)
    {
        if (std::abs((float)(i + 2000) - datum2Segment->audioSamples[i]) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    auto dawInfo = std::dynamic_pointer_cast<DawInfo>(datum3->datum);
    if (dawInfo == nullptr)
    {
        throw std::runtime_error("3rd thing is not dawInfo");
    }
    if (dawInfo->isLooping != false)
    {
        throw std::runtime_error("dawInfo wrong isLooping");
    }
    if (std::abs(dawInfo->loopStartQuarterNotePos - 10) > std::numeric_limits<double>::epsilon())
    {
        throw std::runtime_error("dawInfo wrong loopStartQuarterNotePos");
    }
    if (std::abs(dawInfo->loopEndQuarterNotePos - 20) > std::numeric_limits<double>::epsilon())
    {
        throw std::runtime_error("dawInfo wrong loopEndQuarterNotePos");
    }
    if (dawInfo->bpm != 125)
    {
        throw std::runtime_error("dawInfo wrong bpm");
    }
    if (dawInfo->timeSignatureNumerator != 4)
    {
        throw std::runtime_error("dawInfo wrong timeSignatureNumerator");
    }
    if (dawInfo->timeSignatureDenominator != 1)
    {
        throw std::runtime_error("dawInfo wrong timeSignatureDenominator");
    }
    if (dawInfo->timeSignatureDenominator != 1)
    {
        throw std::runtime_error("dawInfo wrong timeSignatureDenominator");
    }

    auto trackInfo = std::dynamic_pointer_cast<TrackInfo>(datum4->datum);
    if (trackInfo == nullptr)
    {
        throw std::runtime_error("datum4 is not a TrackInfo");
    }
    if (trackInfo->identifier != 1)
    {
        throw std::runtime_error("datum4 wrong identifier");
    }
    if (trackInfo->name != std::string("track number 1"))
    {
        throw std::runtime_error("datum4 wrong identifier");
    }
    if (trackInfo->redColorLevel != 10)
    {
        throw std::runtime_error("datum4 wrong redColorLevel");
    }
    if (trackInfo->greenColorLevel != 20)
    {
        throw std::runtime_error("datum4 wrong greenColorLevel");
    }
    if (trackInfo->blueColorLevel != 30)
    {
        throw std::runtime_error("datum4 wrong blueColorLevel");
    }
    if (trackInfo->alphaColorLevel != 40)
    {
        throw std::runtime_error("datum4 wrong alphaColorLevel");
    }

    // we'll free whathever we took
    store.freeStoredDatum(datum1->storageIdentifier);
    store.freeStoredDatum(datum2->storageIdentifier);
    store.freeStoredDatum(datum3->storageIdentifier);
    store.freeStoredDatum(datum4->storageIdentifier);

    // assert that there is nothing in the queue
    {
        std::lock_guard<std::mutex> lock(store.pendingAudioDataMutex);
        if (store.pendingAudioData.size() != 0)
        {
            throw std::runtime_error("queue is not empty");
        }
    }

    // we'll send the same segment again and assert that we do get only the AudioSegment (since daw and track info
    // didn't change)
    store.parseNewData(&payload);
    // we push it two times because we want to assert that the 3rd is not a track or daw info
    store.parseNewData(&payload);

    auto datum5 = store.waitForDatum();
    auto datum6 = store.waitForDatum();
    auto datum7 = store.waitForDatum();
    auto datum8 = store.waitForDatum();

    auto datum5AudioSegment = std::dynamic_pointer_cast<AudioSegment>(datum5->datum);
    if (datum5AudioSegment == nullptr)
    {
        throw std::runtime_error("datum5AudioSegment nullptr");
    }
    auto datum6AudioSegment = std::dynamic_pointer_cast<AudioSegment>(datum6->datum);
    if (datum6AudioSegment == nullptr)
    {
        throw std::runtime_error("datum6AudioSegment nullptr");
    }
    auto datum7AudioSegment = std::dynamic_pointer_cast<AudioSegment>(datum7->datum);
    if (datum7AudioSegment == nullptr)
    {
        throw std::runtime_error("datum7AudioSegment nullptr");
    }
    auto datum8AudioSegment = std::dynamic_pointer_cast<AudioSegment>(datum8->datum);
    if (datum8AudioSegment == nullptr)
    {
        throw std::runtime_error("datum8AudioSegment nullptr");
    }

    // test audio segment values

    if (datum5AudioSegment->trackIdentifier != 1)
    {
        throw std::runtime_error("d1 wrong identifier");
    }
    if (datum5AudioSegment->channel != 0)
    {
        throw std::runtime_error("d1 wrong channel");
    }
    if (datum5AudioSegment->noChannels != 2)
    {
        throw std::runtime_error("d1 wrong noChannels");
    }
    if (datum5AudioSegment->sampleRate != 48000)
    {
        throw std::runtime_error("d1 wrong sampleRate");
    }
    if (datum5AudioSegment->segmentStartSample != 100)
    {
        throw std::runtime_error("d1 wrong segmentStartSample");
    }
    if (datum5AudioSegment->noAudioSamples != 2000)
    {
        throw std::runtime_error("d1 wrong noAudioSamples");
    }
    for (size_t i = 0; i < 2000; i++)
    {
        if (std::abs((float)i - datum5AudioSegment->audioSamples[i]) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    if (datum6AudioSegment->trackIdentifier != 1)
    {
        throw std::runtime_error("d2 wrong identifier");
    }
    if (datum6AudioSegment->channel != 1)
    {
        throw std::runtime_error("d2 wrong channel");
    }
    if (datum6AudioSegment->noChannels != 2)
    {
        throw std::runtime_error("d2 wrong noChannels");
    }
    if (datum6AudioSegment->sampleRate != 48000)
    {
        throw std::runtime_error("d2 wrong sampleRate");
    }
    if (datum6AudioSegment->segmentStartSample != 100)
    {
        throw std::runtime_error("d2 wrong segmentStartSample");
    }
    if (datum6AudioSegment->noAudioSamples != 2000)
    {
        throw std::runtime_error("d2 wrong noAudioSamples");
    }
    for (size_t i = 0; i < 2000; i++)
    {
        if (std::abs((float)(i + 2000) - datum6AudioSegment->audioSamples[i]) > std::numeric_limits<float>::epsilon())
        {
            throw std::runtime_error("samples don't match");
        }
    }

    store.freeStoredDatum(datum5->storageIdentifier);
    store.freeStoredDatum(datum6->storageIdentifier);
    store.freeStoredDatum(datum7->storageIdentifier);
    store.freeStoredDatum(datum8->storageIdentifier);

    // assert that there is nothing in the queue
    {
        std::lock_guard<std::mutex> lock(store.pendingAudioDataMutex);
        if (store.pendingAudioData.size() != 0)
        {
            throw std::runtime_error("queue is not empty");
        }
    }

    // now we modify track info and assert that it does exists
    a.red = 5;
    payload.set_track_color(a.toColorBytes());

    store.parseNewData(&payload);
    store.parseNewData(&payload);

    auto datum9 = store.waitForDatum();  // AudioSegment
    auto datum10 = store.waitForDatum(); // AudioSegment
    auto datum11 = store.waitForDatum(); // TrackInfo
    auto datum12 = store.waitForDatum(); // AudioSegment
    auto datum13 = store.waitForDatum(); // AudioSegment

    // assert that there is nothing in the queue
    {
        std::lock_guard<std::mutex> lock(store.pendingAudioDataMutex);
        if (store.pendingAudioData.size() != 0)
        {
            throw std::runtime_error("queue is not empty");
        }
    }

    auto AudioSeg9 = std::dynamic_pointer_cast<AudioSegment>(datum9->datum);
    auto AudioSeg10 = std::dynamic_pointer_cast<AudioSegment>(datum10->datum);
    auto AudioSeg12 = std::dynamic_pointer_cast<AudioSegment>(datum12->datum);
    auto AudioSeg13 = std::dynamic_pointer_cast<AudioSegment>(datum13->datum);
    if (AudioSeg9 == nullptr)
    {
        throw std::runtime_error("datum not audio seg");
    }
    if (AudioSeg10 == nullptr)
    {
        throw std::runtime_error("datum not audio seg");
    }
    if (AudioSeg12 == nullptr)
    {
        throw std::runtime_error("datum not audio seg");
    }
    if (AudioSeg13 == nullptr)
    {
        throw std::runtime_error("datum not audio seg");
    }

    auto datum11TrackInfo = std::dynamic_pointer_cast<TrackInfo>(datum11->datum);
    if (datum11TrackInfo == nullptr)
    {
        throw std::runtime_error("datum not track info");
    }
    if (datum11TrackInfo->redColorLevel != 5)
    {
        throw std::runtime_error("red not 5");
    }

    store.freeStoredDatum(datum9->storageIdentifier);
    store.freeStoredDatum(datum10->storageIdentifier);
    store.freeStoredDatum(datum11->storageIdentifier);
    store.freeStoredDatum(datum11->storageIdentifier);
    store.freeStoredDatum(datum12->storageIdentifier);

    // we do the same for dawinfo
    payload.set_daw_bpm(100);

    store.parseNewData(&payload);
    store.parseNewData(&payload);

    auto datum14 = store.waitForDatum(); // AudioSegment
    auto datum15 = store.waitForDatum(); // AudioSegment
    auto datum16 = store.waitForDatum(); // DawInfo
    auto datum17 = store.waitForDatum(); // AudioSegment
    auto datum18 = store.waitForDatum(); // AudioSegment

    // assert that there is nothing in the queue
    {
        std::lock_guard<std::mutex> lock(store.pendingAudioDataMutex);
        if (store.pendingAudioData.size() != 0)
        {
            throw std::runtime_error("queue is not empty");
        }
    }

    auto AudioSeg14 = std::dynamic_pointer_cast<AudioSegment>(datum14->datum);
    auto AudioSeg15 = std::dynamic_pointer_cast<AudioSegment>(datum15->datum);
    auto AudioSeg17 = std::dynamic_pointer_cast<AudioSegment>(datum17->datum);
    auto AudioSeg18 = std::dynamic_pointer_cast<AudioSegment>(datum18->datum);
    if (AudioSeg14 == nullptr)
    {
        throw std::runtime_error("datum not audio seg");
    }
    if (AudioSeg15 == nullptr)
    {
        throw std::runtime_error("datum not audio seg");
    }
    if (AudioSeg17 == nullptr)
    {
        throw std::runtime_error("datum not audio seg");
    }
    if (AudioSeg18 == nullptr)
    {
        throw std::runtime_error("datum not audio seg");
    }

    auto dawInfo16 = std::dynamic_pointer_cast<DawInfo>(datum16->datum);
    if (dawInfo16 == nullptr)
    {
        throw std::runtime_error("daw info 16 not a DawInfo");
    }
    if (dawInfo16->bpm != 100)
    {
        throw std::runtime_error("daw info has wrong bpm");
    }

    store.freeStoredDatum(datum14->storageIdentifier);
    store.freeStoredDatum(datum15->storageIdentifier);
    store.freeStoredDatum(datum16->storageIdentifier);
    store.freeStoredDatum(datum17->storageIdentifier);
    store.freeStoredDatum(datum18->storageIdentifier);
}