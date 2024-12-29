#ifndef SOCKET_RUNTIME_VERSION_H
#define SOCKET_RUNTIME_VERSION_H

#include "string.hh"
#include "config.hh"

namespace ssc::runtime::version {
  inline const auto VERSION_FULL_STRING = String(CONVERT_TO_STRING(SOCKET_RUNTIME_VERSION) " (" CONVERT_TO_STRING(SOCKET_RUNTIME_VERSION_HASH) ")");
  inline const auto VERSION_HASH_STRING = String(CONVERT_TO_STRING(SOCKET_RUNTIME_VERSION_HASH));
  inline const auto VERSION_STRING = String(CONVERT_TO_STRING(SOCKET_RUNTIME_VERSION));
}
#endif
