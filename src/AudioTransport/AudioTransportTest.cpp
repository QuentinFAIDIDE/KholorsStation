#include "AudioDataStoreTest.h"
#include "ColorBytesTest.h"

int main(int, char **)
{
    ColorBytesTestSuite suite1;
    suite1.runAll();

    AudioTransport::AudioDataStoreTestSuite suite2;
    suite2.runAll();

    return 0;
}