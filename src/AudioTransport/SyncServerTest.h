#pragma once

namespace AudioTransport
{

class SyncServerTestSuite
{
  public:
    /**
     * @brief Run all tests
     *
     */
    void runAll();

    static void smokeTest01();
    static void testTransport01();
};

} // namespace AudioTransport