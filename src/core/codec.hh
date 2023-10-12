#ifndef SSC_CORE_CODEC_HH
#define SSC_CORE_CODEC_HH

#include "types.hh"

namespace SSC {
  String encodeURIComponent (const String& input);
  String decodeURIComponent (const String& input);
  String encodeHexString (const String& input);
  String decodeHexString (const String& input);
  size_t decodeUTF8 (char *output, const char *input, size_t length);

  const Array<uint8_t, 8> toBytes (const uint64_t input);
}

#endif
