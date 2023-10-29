#ifndef SSC_CORE_CODEC_HH
#define SSC_CORE_CODEC_HH

#include "types.hh"

namespace SSC {
  /**
   * Encodes input by replacing certain characters by
   * one, two, three, or four escape sequences representing the UTF-8
   * encoding of the character.
   * @param input The input string to encode
   * @return An encoded string value
   */
  String encodeURIComponent (const String& input);

  /**
   * Decodes a value encoded with `encodeURIComponent`
   * @param input The input string to decode
   * @return A decoded string
   */
  String decodeURIComponent (const String& input);

  /**
   * Encodes input as a string of hex characters.
   * @param input The input string to encode
   * @return An encoded string value
   */
  String encodeHexString (const String& input);

  /**
   * Decodes input as a string of hex characters to a normal string.
   * @param input The input string to encode
   * @return An encoded string value
   */
  String decodeHexString (const String& input);

  /**
   * Decodes UTF8 byte string of variable `length` size in `input` to
   * `output` returning `size_t` bytes written to `output`.
   * @param output Pointer owned by caller to write decoded output to
   * @param input Pointer owned by caller to decode `length` bytes
   * @param length Size of `input` in bytes
   * @return The number of bytes written to `output`
   */
  size_t decodeUTF8 (char *output, const char *input, size_t length);

  /**
   * Converts a `uint64_t` to a array of `uint8_t` values (bytes)
   * @param input The `uint64_t` to convert to an array of bytes
   * @return An array of `uint8_t` values
   */
  const Array<uint8_t, 8> toBytes (const uint64_t input);
}

#endif
