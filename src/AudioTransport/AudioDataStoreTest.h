#pragma once

namespace AudioTransport
{

class AudioDataStoreTestSuite
{
  public:
    /**
     * @brief Testing that the AudioSegmentPayloads are properly parsed,
     * within a single thread.
     */
    void testParse01();

    /**
     * @brief Testing that the structs are properly reused.
     *
     */
    void testPreallocation01();

    // run all tests
    void runAll();
};

} // namespace AudioTransport
