#include "ColorBytesTest.h"
#include "ColorBytes.h"
#include <stdexcept>

void ColorBytesTestSuite::runAll()
{
    testColorConversion();
}

void ColorBytesTestSuite::testColorConversion()
{
    AudioTransport::ColorContainer cc(10, 20, 50, 60);
    uint32_t colorBytes = cc.toColorBytes();
    AudioTransport::ColorContainer cc2(colorBytes);
    if (!(cc == cc2))
    {
        std::runtime_error("color bytes conversion failed");
    }
}