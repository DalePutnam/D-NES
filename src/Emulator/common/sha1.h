#include <cstdint>
#include <iostream>

namespace SHA1
{

struct Digest { uint8_t _data[20]; };

extern bool operator==(const Digest& left, const Digest& right);
extern bool operator!=(const Digest& left, const Digest& right);

extern std::ostream& operator<<(std::ostream& out, const Digest& hash);

extern Digest computeDigest(uint8_t* message, uint32_t length);

}