#include "../url.hh"

static const char HEX_CHARS_INDEX[16 + 1] = "0123456789ABCDEF";

static const signed char DEC_CHARS_INDEX[256] = {
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

static const char SAFE_CHARS_INDEX[256] = {
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

namespace ssc::runtime::url {
  size_t url_encode_uri_component (
    const char *input,
    size_t inputSize,
    char *output,
    size_t outputSize
  ) {
    size_t i = 0;
    size_t j = 0;

    if (!input || !output || outputSize == 0) {
      return 0;
    }

    while (i < inputSize) {
      const auto c = (unsigned char)input[i++];
      if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
        if (j >= outputSize - 1) {
          return 0;
        }

        output[j++] = c;
      } else {
        if (j >= outputSize - 3) {
          return 0;
        }

        output[j++] = '%';
        output[j++] = HEX_CHARS_INDEX[c >> 4];
        output[j++] = HEX_CHARS_INDEX[c & 0x0F];
      }
    }

    if (j >= outputSize) {
      return 0;
    }

    output[j] = 0;
    return j;
  }

  size_t url_decode_uri_component (
    const char *input,
    size_t inputSize,
    char *output,
    size_t outputSize
  ) {
    size_t i = 0;
    size_t j = 0;

    if (!input || !output || outputSize == 0) {
      return 0;
    }

    while (i < inputSize) {
      if (
        input[i] == '%' &&
        i + 2 < inputSize &&
        isxdigit(input[i + 1]) &&
        isxdigit(input[i + 2])
      ) {
        if (j >= outputSize - 1) {
          return 0;
        }

        const char high = input[i + 1];
        const char low = input[i + 2];

        output[j++] = static_cast<char>(
          (isdigit(high) ? high - '0' : (toupper(high) - 'A' + 10)) << 4 |
          (isdigit(low) ? low - '0' : (toupper(low) - 'A' + 10))
        );

        i += 3;
      } else if (input[i] == '+') {
        if (j >= outputSize - 1) {
          return 0;
        }

        output[j++] = ' ';
        i++;
      } else {
        if (j >= outputSize - 1) {
          return 0;
        }

        output[j++] = input[i++];
      }
    }

    if (j >= outputSize) {
      return 0;
    }

    output[j] = 0;
    return j;
  }

  String encodeURIComponent (const String& input) {
    // maximum size if all characters are encoded
    Vector<char> output(input.size() * 3 + 1);
    const auto size = url_encode_uri_component(
      input.c_str(),
      input.size(),
      output.data(),
      output.size()
    );
    return String(output.data(), size);
  }

  String decodeURIComponent (const String& input) {
    // decoded string is never longer than input
    Vector<char> output(input.size() + 1);
    const auto size = url_decode_uri_component(
      input.c_str(),
      input.size(),
      output.data(),
      output.size()
    );
    return String(output.data(), size);
  }
}
