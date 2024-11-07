#include "Base58.h"

int DecodeBase58(const std::string input, int len, unsigned char *result)
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