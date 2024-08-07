#include "SyncServerTest.h"
#include "AudioSegment.h"
#include "Client.h"
#include "ColorBytes.h"
#include "SyncServer.h"
#include <grpcpp/support/status.h>
#include <stdexcept>

using namespace AudioTransport;

void SyncServerTestSuite::runAll()
{
    smokeTest01();
    testTransport01();
}

void SyncServerTestSuite::smokeTest01()
{
    SyncServer server;

    auto state = server.setServerToListenOnPort(8879);
    if (state != NO_ERROR)
    {
        throw std::runtime_error("Unable to start gRPC server");
    }
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
    sleep(2);
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
    // should do nothing
    state = server.setServerToListenOnPort(8879);
    if (state != ALREADY_RUNNING)
    {
        throw std::runtime_error("Unable to start gRPC server");
    }
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
    state = server.setServerToListenOnPort(8899);
    if (state != NO_ERROR)
    {
        throw std::runtime_error("Unable to start gRPC server");
    }
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
    server.stopServer();
    if (server.isRunning())
    {
        throw std::runtime_error("Server state is running");
    }
    state = server.setServerToListenOnPort(8899);
    if (state != NO_ERROR)
    {
        throw std::runtime_error("Unable to start gRPC server");
    }
    if (!server.isRunning())
    {
        throw std::runtime_error("Server state is not running");
    }
}

void SyncServerTestSuite::testTransport01()
{
    SyncServer server;
    server.setServerToListenOnPort(8794);

    Client client(8794);

    ColorContainer a = ColorContainer(10, 20, 30, 40);

    std::vector<float> data(2000 * 2);
    for (size_t i = 0; i < 4000; i++)
    {
        data[i] = (float)i;
    }

    AudioSegmentPayload payload;
    payload.set_track_identifier(1);
    payload.set_track_color(a.toColorBytes());
    payload.set_track_name("track number 1");
    payload.set_daw_sample_rate(48000);
    payload.set_daw_bpm(125);
    payload.set_daw_time_signature_denominator(1);
    payload.set_daw_time_signature_numerator(4);
    payload.set_daw_is_looping(false);
    payload.set_daw_is_playing(true);
    payload.set_daw_loop_start(10);
    payload.set_daw_loop_end(20);
    payload.set_segment_start_sample(100);
    payload.set_segment_sample_duration(2000);
    payload.set_segment_no_channels(2);
    for (int i = 0; i < 4000; i++)
    {
        payload.add_segment_audio_samples(data[i]);
    }

    if (!client.sendAudioSegment(&payload))
    {
        throw std::runtime_error("Unable to reach server");
    }

    auto datum1 = server.waitForDatum();
    auto datum2 = server.waitForDatum();
    auto datum3 = server.waitForDatum();
    auto datum4 = server.waitForDatum();

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
    server.freeStoredDatum(datum1->storageIdentifier);
    server.freeStoredDatum(datum2->storageIdentifier);
    server.freeStoredDatum(datum3->storageIdentifier);
    server.freeStoredDatum(datum4->storageIdentifier);

    server.setServerToListenOnPort(8799);
    client.changeDestinationPort(8799);

    if (!client.sendAudioSegment(&payload))
    {
        throw std::runtime_error("Unable to reach server");
    }

    auto datum5 = server.waitForDatum();
    auto datum6 = server.waitForDatum();

    server.freeStoredDatum(datum5->storageIdentifier);
    server.freeStoredDatum(datum6->storageIdentifier);

    server.stopServer();
}