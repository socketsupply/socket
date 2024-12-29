#include "../bytes.hh"

namespace ssc::runtime::bytes {
  static const char TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

  static unsigned char base64_decode_char (char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 255; // invalid
  }

  size_t base64_encode_length (size_t inputSize) {
    return 4 * ((inputSize + 2) / 3);
  }

  size_t base64_decode_length (size_t inputSize) {
    return (inputSize / 4) * 3;
  }

  size_t base64_encode (
    const unsigned char *input,
    size_t inputSize,
    char *output,
    size_t outputSize
  ) {
    size_t encodedSize = 4 * ((inputSize + 2) / 3);
    size_t i = 0;
    size_t j = 0;

    if (outputSize < encodedSize + 1) {
      return 0;
    }

    while (i < inputSize) {
      const uint32_t a = i < inputSize ? input[i++] : 0;
      const uint32_t b = i < inputSize ? input[i++] : 0;
      const uint32_t c = i < inputSize ? input[i++] : 0;

      const uint32_t triple = (a << 16) | (b << 8) | c;

      output[j++] = TABLE[(triple >> 18) & 0x3F];
      output[j++] = TABLE[(triple >> 12) & 0x3F];
      output[j++] = (i > inputSize + 1) ? '=' : TABLE[(triple >> 6) & 0x3F];
      output[j++] = (i > inputSize) ? '=' : TABLE[triple & 0x3F];
    }

    size_t paddingSize = (3 - (inputSize % 3)) % 3;
    for (size_t p = 0; p < paddingSize; ++p) {
      output[encodedSize - 1 - p] = '=';
    }

    output[j] = 0;
    return encodedSize;
  }

  size_t base64_decode (
    const char *input,
    size_t inputSize,
    unsigned char *output,
    size_t outputSize
  ) {
    if (inputSize % 4 != 0) {
      return 0;
    }

    size_t decodedSize = (inputSize / 4) * 3;
    size_t i = 0;
    size_t j = 0;

    if (input[inputSize - 1] == '=') decodedSize--;
    if (input[inputSize - 2] == '=') decodedSize--;

    if (outputSize < decodedSize) {
      return 0;
    }

    while (i < inputSize) {
      const auto a = base64_decode_char(input[i++]);
      const auto b = base64_decode_char(input[i++]);
      const auto c = base64_decode_char(input[i++]);
      const auto d = base64_decode_char(input[i++]);

      const uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;

      if (j < decodedSize) output[j++] = (triple >> 16) & 0xFF;
      if (j < decodedSize) output[j++] = (triple >> 8) & 0xFF;
      if (j < decodedSize) output[j++] = triple & 0xFF;
    }

    return decodedSize;
  }

  namespace base64 {
    String encode (const Vector<uint8_t>& input) {
      const size_t size = 4 * ((input.size() + 2) / 3) + 1;
      String output(size, 0);
      const auto encodedSize = base64_encode(
        input.data(),
        input.size(),
        &output[0],
        output.size()
      );
      output.resize(encodedSize);
      return output;
    }

    String encode (const String& input) {
      const size_t size = 4 * ((input.size() + 2) / 3) + 1;
      String output(size, 0);
      const auto encodedSize = base64_encode(
        reinterpret_cast<const unsigned char *>(input.data()),
        input.size(),
        &output[0],
        output.size()
      );
      output.resize(encodedSize);
      return output;
    }

    String decode (const String& input) {
      const auto size = (input.size() / 4) * 3;
      String output(size, 0);
      const auto decodedSize = base64_decode(
        input.data(),
        input.size(),
        reinterpret_cast<unsigned char *>(&output[0]),
        output.size()
      );
      output.resize(decodedSize);
      return output;
    }
  }
}
