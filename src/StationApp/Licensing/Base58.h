#pragma once

#include <string>
using namespace std;

// NOTE: All license related function are inlined to make
// it just a tiny bit harder for crackers to modify this code
// in all places.

// taken from https://gist.github.com/miguelmota/f8cc2502641fb06aeb71de5882c6c086

const char *const BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const char BASE58_ALPHABET_MAP[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,
    3,  4,  5,  6,  7,  8,  -1, -1, -1, -1, -1, -1, -1, 9,  10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20,
    21, -1, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1, -1, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, -1, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1};

inline int DecodeBase58(const std::string input, int len, unsigned char *result)
{
    unsigned char const *str = (unsigned const char *)(input.c_str());
    result[0] = 0;
    int resultlen = 1;
    for (int i = 0; i < len; i++)
    {
        unsigned int carry = (unsigned int)BASE58_ALPHABET_MAP[str[i]];
        for (int j = 0; j < resultlen; j++)
        {
            carry += (unsigned int)(result[j]) * 58;
            result[j] = (unsigned char)(carry & 0xff);
            carry >>= 8;
        }
        while (carry > 0)
        {
            result[resultlen++] = (unsigned int)(carry & 0xff);
            carry >>= 8;
        }
    }

    for (int i = 0; i < len && str[i] == '1'; i++)
        result[resultlen++] = 0;

    for (int i = resultlen - 1, z = (resultlen >> 1) + (resultlen & 1); i >= z; i--)
    {
        int k = result[i];
        result[i] = result[resultlen - i - 1];
        result[resultlen - i - 1] = k;
    }
    return resultlen;
}