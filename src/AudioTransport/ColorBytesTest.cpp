#include "ColorBytesTest.h"
#include "ColorBytes.h"
#include <stdexcept>

void ColorBytesTestSuite::runAll()
{
    testColorConversion();
    testColorConversion2();
    testColorConversion3();
}

void ColorBytesTestSuite::testColorConversion()
{
    AudioTransport::ColorContainer const cc(10, 20, 50, 60);
    uint32_t const colorBytes = cc.toColorBytes();
    AudioTransport::ColorContainer const cc2(colorBytes);
    if (!(cc == cc2))
    {
        std::runtime_error("color bytes conversion failed");
    }
}

void ColorBytesTestSuite::testColorConversion2()
{
    AudioTransport::ColorContainer const cc(10, 20, 30, 40);
    if (cc.toColorBytes() != 0x281e140a)
    {
        throw std::runtime_error("ColorContainer(10, 20, 50, 60) bytes are not 0x281e140a");
    }
}

void ColorBytesTestSuite::testColorConversion3()
{
    AudioTransport::ColorContainer const cc(0x281e140a);
    if (cc.red != 10)
    {
        throw std::runtime_error("red wrong");
    }
    if (cc.green != 20)
    {
        throw std::runtime_error("green wrong");
    }
    if (cc.blue != 30)
    {
        throw std::runtime_error("blue wrong");
    }
    if (cc.alpha != 40)
    {
        throw std::runtime_error("alpha wrong");
    }
}