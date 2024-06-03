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

    void smokeTest01();
    void testTransport01();
};

} // namespace AudioTransport