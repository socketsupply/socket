#ifndef SOCKET_RUNTIME_CORE_CODEC_H
#define SOCKET_RUNTIME_CORE_CODEC_H

#include "../platform/types.hh"

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

  /**
   * Rotates an unsigned integer value left by a specified number of steps.
   * @param value The unsigned integer value to rotate
   * @param steps The number of steps to rotate left
   * @return The rotated unsigned integer value
   */
  inline const unsigned int rol (const unsigned int value, const unsigned int steps) {
    return ((value << steps) | (value >> (32 - steps)));
  }

  /**
   * Clears a buffer of unsigned integers by setting each element to zero.
   * @param buffert Pointer to the buffer to clear
   */
  inline void clearWBuffert (unsigned int* buffert) {
    int pos = 0;
    for (pos = 16; --pos >= 0;) {
      buffert[pos] = 0;
    }
  }

  /**
   * Computes the inner hash for the SHA-1 algorithm.
   * @param result Pointer to an array of unsigned integers to store the result
   * @param w Pointer to an array of unsigned integers to use in computation
   */
  void innerHash (unsigned int* result, unsigned int* w);

  /**
   * Calculates the SHA-1 hash of a given input string.
   * @param src Pointer to the input string
   * @param dest Pointer to the destination buffer to store the hash
   */
  void shacalc (const char* src, char* dest);

  /**
   * Encodes a given input data to a Base64 encoded string.
   * @param data Pointer to the input data
   * @param input_length Length of the input data
   * @param output_length Pointer to store the length of the encoded output
   * @return Pointer to the Base64 encoded string
   */
  unsigned char* base64Encode(const unsigned char *data, size_t input_length, size_t *output_length);

  // Encoding table for base64 encoding
  const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  // Modulus table for base64 encoding
  const int mod_table[] = {0, 2, 1};
}

#endif
