#ifndef DEF_SYNC_SERVER_TEST_HPP
#define DEF_SYNC_SERVER_TEST_HPP

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

#endif // DEF_SYNC_SERVER_TEST_HPP