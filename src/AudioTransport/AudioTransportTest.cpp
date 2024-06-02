#include "AudioDataStoreTest.h"
#include "ColorBytesTest.h"
#include "SyncServerTest.h"

int main(int, char **)
{
    ColorBytesTestSuite suite1;
    suite1.runAll();

    AudioTransport::AudioDataStoreTestSuite suite2;
    suite2.runAll();

    AudioTransport::SyncServerTestSuite suite3;
    suite3.runAll();

    return 0;
}