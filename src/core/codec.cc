#include <math.h>

#include "../platform/platform.hh"
#include "codec.hh"

#define UNSIGNED_IN_RANGE(value, min, max) (                                   \
  (unsigned char) (value) >= (unsigned char) (min) &&                          \
  (unsigned char) (value) <= (unsigned char) (max)                             \
)

static const char DEC2HEX[16 + 1] = "0123456789ABCDEF";

//
// All ipc uses a URI schema, so all ipc data needs to be
// encoded as a URI component. This prevents escaping the
// protocol.
//
static const signed char HEX2DEC[256] = {
  /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
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

static const char SAFE[256] = {
  /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
  /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

  /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
  /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
  /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
  /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

  /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

  /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

namespace SSC {
  const Array<uint8_t, 8> toBytes (const uint64_t input) {
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

  String encodeURIComponent (const String& input) {
    auto pointer = (unsigned char*) input.c_str();
    const auto length = (int) input.length();
    auto const start = new unsigned char[length* 3];
    auto end = start;
    const unsigned char* const maxLength = pointer + length;

    for (; pointer < maxLength; ++pointer) {
      if (SAFE[*pointer]) {
        *end++ = *pointer;
      } else {
        // escape this char
        *end++ = '%';
        *end++ = DEC2HEX[*pointer >> 4];
        *end++ = DEC2HEX[*pointer & 0x0F];
      }
    }

    String result((char*) start, (char*) end);
    delete [] start;
    return result;
  }

  String decodeURIComponent (const String& input) {
    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"

    const auto string = replace(input, "\\+", " ");
    auto pointer = (const unsigned char *) string.c_str();
    const auto length = (int) string.length();
    const auto maxLength = pointer + length;

    char* const start = new char[length];
    char* end = start;

    while (pointer < maxLength - 2) {
      if (*pointer == '%') {
        char dec1, dec2;
        if (-1 != (dec1 = HEX2DEC[*(pointer + 1)])
            && -1 != (dec2 = HEX2DEC[*(pointer + 2)])) {

            *end++ = (dec1 << 4) + dec2;
            pointer += 3;
            continue;
        }
      }

      *end++ = *pointer++;
    }

    // the last 2- chars
    while (pointer < maxLength) {
      *end++ = *pointer++;
    }

    String result(start, end);
    delete [] start;
    return result;
  }

  String encodeHexString (const String& input) {
    String output;
    auto length = 2 * input.length();

    output.reserve(length);

    for (unsigned char character : input) {
      output.push_back(DEC2HEX[character >> 4]);
      output.push_back(DEC2HEX[character & 15]);
    }

    return output;
  }

  String decodeHexString (const String& input) {
    const auto length = input.length() / 2;
    String output;

    output.reserve(length);

    for (auto character = input.begin(); character != input.end();) {
      const int hi = HEX2DEC[(unsigned char) *character++];
      const int lo = HEX2DEC[(unsigned char) *character++];
      output.push_back(hi << 4 | lo);
    }

    return output;
  }

  size_t decodeUTF8 (char *output, const char *input, size_t length) {
    unsigned char cp = 0; // code point
    unsigned char lower = 0x80;
    unsigned char upper = 0xBF;

    int x = 0; // cp needed
    int y = 0; // cp  seen
    int size = 0; // output size

    for (int i = 0; i < length; ++i) {
      auto b = (unsigned char) input[i];

      if (b == 0) {
        output[size++] = 0;
        continue;
      }

      if (x == 0) {
        // 1 byte
        if (UNSIGNED_IN_RANGE(b, 0x00, 0x7F)) {
          output[size++] = b;
          continue;
        }

        if (!UNSIGNED_IN_RANGE(b, 0xC2, 0xF4)) {
          break;
        }

        // 2 byte
        if (UNSIGNED_IN_RANGE(b, 0xC2, 0xDF)) {
          x = 1;
          cp = b - 0xC0;
        }

        // 3 byte
        if (UNSIGNED_IN_RANGE(b, 0xE0, 0xEF)) {
          if (b == 0xE0) {
            lower = 0xA0;
          } else if (b == 0xED) {
            upper = 0x9F;
          }

          x = 2;
          cp = b - 0xE0;
        }

        // 4 byte
        if (UNSIGNED_IN_RANGE(b, 0xF0, 0xF4)) {
          if (b == 0xF0) {
            lower = 0x90;
          } else if (b == 0xF4) {
            upper = 0x8F;
          }

          x = 3;
          cp = b - 0xF0;
        }

        cp = cp * pow(64, x);
        continue;
      }

      if (!UNSIGNED_IN_RANGE(b, lower, upper)) {
        lower = 0x80;
        upper = 0xBF;

        // revert
        cp = 0;
        x = 0;
        y = 0;
        i--;
        continue;
      }

      lower = 0x80;
      upper = 0xBF;
      y++;
      cp += (b - 0x80) * pow(64, x - y);

      if (y != x) {
        continue;
      }

      output[size++] = cp;
      // continue to next
      cp = 0;
      x = 0;
      y = 0;
    }

    return size;
  }
}
