#include <math.h>

#include "../platform.hh"
#include "../bytes.hh"

#define UNSIGNED_IN_RANGE(value, min, max) (                                   \
  (unsigned char) (value) >= (unsigned char) (min) &&                          \
  (unsigned char) (value) <= (unsigned char) (max)                             \
)

static const char HEX_CHARS_TABLE[16 + 1] = "0123456789ABCDEF";
static const signed char DEC_CHARS_INDEX[256] = { /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
  /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

  /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

  /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

  /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

namespace ssc::runtime::bytes {
  const Array<uint8_t, 8> toByteArray (const uint64_t input) {
    Array<uint8_t, 8> bytes;
    // big endian, network order
    bytes[0] = input >> 8*7;
    bytes[1] = input >> 8*6;
    bytes[2] = input >> 8*5;
    bytes[3] = input >> 8*4;
    bytes[4] = input >> 8*3;
    bytes[5] = input >> 8*2;
    bytes[6] = input >> 8*1;
    bytes[7] = input >> 8*0;
    return bytes;
  }

  const Array<uint8_t, 4> toByteArray (const uint32_t input) {
    Array<uint8_t, 4> bytes;
    // big endian, network order
    bytes[0] = input >> 4*7;
    bytes[1] = input >> 4*6;
    bytes[2] = input >> 4*5;
    bytes[3] = input >> 4*4;
    return bytes;
  }

  const Array<uint8_t, 2> toByteArray (const uint16_t input) {
    Array<uint8_t, 2> bytes;
    // big endian, network order
    bytes[0] = input >> 2*7;
    bytes[1] = input >> 2*6;
    return bytes;
  }

  String encodeHexString (const String& input) {
    String output;
    const auto length = 2 * input.size();

    output.reserve(length);

    for (unsigned char character : input) {
      output.push_back(HEX_CHARS_TABLE[character >> 4]);
      output.push_back(HEX_CHARS_TABLE[character & 15]);
    }

    return output;
  }

  String encodeHexString (const Vector<uint8_t>& input) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto c : input) {
      oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
  }

  String decodeHexString (const String& input) {
    const auto length = input.length() / 2;
    String output;

    output.reserve(length);

    for (auto character = input.begin(); character != input.end();) {
      const int hi = DEC_CHARS_INDEX[(unsigned char) *character++];
      const int lo = DEC_CHARS_INDEX[(unsigned char) *character++];
      output.push_back(hi << 4 | lo);
    }

    return output;
  }
}
