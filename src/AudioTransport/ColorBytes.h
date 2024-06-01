#ifndef DEF_COLOR_BYTES_HPP
#define DEF_COLOR_BYTES_HPP

#include <cstdint>
#include <cstring>
namespace AudioTransport
{

struct ColorContainer
{
    ColorContainer(uint32_t colorBytes)
    {
        uint8_t colors[4];
        memcpy(colors, &colorBytes, sizeof(uint32_t));
        red = colors[0];
        green = colors[1];
        blue = colors[2];
        alpha = colors[3];
    }
    ColorContainer(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : red(r), green(g), blue(b), alpha(a)
    {
    }
    uint32_t toColorBytes()
    {
        uint8_t colors[4];
        colors[0] = red;
        colors[1] = green;
        colors[2] = blue;
        colors[3] = alpha;
        uint32_t colorBytes;
        memcpy(&colorBytes, colors, 4 * sizeof(uint8_t));
        return colorBytes;
    }
    bool operator==(const ColorContainer &other)
    {
        return red == other.red && green == other.green && blue == other.blue && alpha == other.alpha;
    }

    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

};     // namespace AudioTransport
#endif // DEF_COLOR_BYTES_HPP