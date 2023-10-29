#ifndef SSC_CORE_VERSION
#define SSC_CORE_VERSION

#include "config.hh"
#include "string.hh"
#include "types.hh"

namespace SSC {
  inline const auto VERSION_FULL_STRING = String(CONVERT_TO_STRING(SSC_VERSION) " (" CONVERT_TO_STRING(SSC_VERSION_HASH) ")");
  inline const auto VERSION_HASH_STRING = String(CONVERT_TO_STRING(SSC_VERSION_HASH));
  inline const auto VERSION_STRING = String(CONVERT_TO_STRING(SSC_VERSION));
}

#endif
