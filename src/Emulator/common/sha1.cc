#include "sha1.h"

#include <cstring>
#include <memory>
#include <vector>
#include <iomanip>

#if defined(_WIN32) || defined(_WIN64)
#define LITTLE_ENDIAN
#else
#include <endian.h>
#endif


bool SHA1::operator==(const Digest& left, const Digest& right)
{
    for (uint32_t i = 0; i < 20; ++i)
    {
        if (left._data[i] != right._data[i])
        {
            return false;
        }
    }

    return true;
}

bool SHA1::operator!=(const Digest& left, const Digest& right)
{
    return !(left == right);
}

std::ostream& SHA1::operator<<(std::ostream& out, const Digest& hash)
{
    std::ios_base::fmtflags flags = out.flags();

    out << std::hex << std::setfill('0');
    for (uint32_t i = 0; i < 20; ++i)
    {
        out << std::setw(2) << static_cast<unsigned int>(hash._data[i]);
    }

    out.flags(flags);

    return out;
}

SHA1::Digest SHA1::computeDigest(uint8_t* message, uint32_t length)
{
    std::vector<uint8_t> m(message, message + length);
    m.push_back(0x80);

    uint32_t paddingSize = 64 - (m.size() % 64) - 8;
    m.insert(m.end(), paddingSize, 0);

    uint64_t bitLength = length * 8;
    m.push_back(static_cast<uint8_t>((bitLength & 0xFF00000000000000UL) >> 56));
    m.push_back(static_cast<uint8_t>((bitLength & 0x00FF000000000000UL) >> 48));
    m.push_back(static_cast<uint8_t>((bitLength & 0x0000FF0000000000UL) >> 40));
    m.push_back(static_cast<uint8_t>((bitLength & 0x000000FF00000000UL) >> 32));
    m.push_back(static_cast<uint8_t>((bitLength & 0x00000000FF000000UL) >> 24));
    m.push_back(static_cast<uint8_t>((bitLength & 0x0000000000FF0000UL) >> 16));
    m.push_back(static_cast<uint8_t>((bitLength & 0x000000000000FF00UL) >>  8));
    m.push_back(static_cast<uint8_t>((bitLength & 0x00000000000000FFUL) >>  0));

    uint32_t h0 = 0x67452301L;
    uint32_t h1 = 0xEFCDAB89L;
    uint32_t h2 = 0x98BADCFEL;
    uint32_t h3 = 0x10325476L;
    uint32_t h4 = 0xC3D2E1F0L;

    uint32_t w[80];
    for (uint64_t chunkIndex = 0; chunkIndex < m.size(); chunkIndex += 64)
    {
#ifdef LITTLE_ENDIAN
        uint8_t* chunk = m.data() + chunkIndex;
        for (uint32_t i = 0, f = 0; i < 64 && f < 16; i += 4, ++f)
        {
            w[f] = chunk[i] << 24 | 
                   chunk[i + 1] << 16 |
                   chunk[i + 2] << 8 |
                   chunk[i + 3];
        }
#elif defined(BIG_ENDIAN)
        std::memcpy(w, m.data() + chunkIndex, 64);
#endif

        for (uint32_t i = 16; i < 80; ++i)
        {
            uint32_t word = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (word << 1) | (word >> 31);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        for (uint32_t i = 0; i < 80; ++i)
        {
            uint32_t f, k;
            if (0 <= i && i <= 19)
            {
                f = (b & c) | (~b & d);
                k = 0x5A827999U;
            }
            else if (20 <= i && i <= 39)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1U;
            }
            else if (40 <= i && i <= 59)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDCU;
            }
            else if (60 <= i && i <= 79)
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6U;
            }

            uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = ((b << 30) | (b >> 2));
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    Digest digest;

#ifdef LITTLE_ENDIAN
    digest._data[0]  = static_cast<uint8_t>(h0 >> 24);
    digest._data[1]  = static_cast<uint8_t>(h0 >> 16);
    digest._data[2]  = static_cast<uint8_t>(h0 >> 8);
    digest._data[3]  = static_cast<uint8_t>(h0);
    digest._data[4]  = static_cast<uint8_t>(h1 >> 24);
    digest._data[5]  = static_cast<uint8_t>(h1 >> 16);
    digest._data[6]  = static_cast<uint8_t>(h1 >> 8);
    digest._data[7]  = static_cast<uint8_t>(h1);
    digest._data[8]  = static_cast<uint8_t>(h2 >> 24);
    digest._data[9]  = static_cast<uint8_t>(h2 >> 16);
    digest._data[10] = static_cast<uint8_t>(h2 >> 8);
    digest._data[11] = static_cast<uint8_t>(h2);
    digest._data[12] = static_cast<uint8_t>(h3 >> 24);
    digest._data[13] = static_cast<uint8_t>(h3 >> 16);
    digest._data[14] = static_cast<uint8_t>(h3 >> 8);
    digest._data[15] = static_cast<uint8_t>(h3);
    digest._data[16] = static_cast<uint8_t>(h4 >> 24);
    digest._data[17] = static_cast<uint8_t>(h4 >> 16);
    digest._data[18] = static_cast<uint8_t>(h4 >> 8);
    digest._data[19] = static_cast<uint8_t>(h4);
#elif BIG_ENDIAN
    std::memcpy(digest._data , &h0, 4);
    std::memcpy(digest._data + 4, &h1, 4);
    std::memcpy(digest._data + 8, &h2, 4);
    std::memcpy(digest._data + 12, &h3, 4);
    std::memcpy(digest._data + 16, &h4, 4);
#endif

    return digest;
}