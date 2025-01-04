#ifndef SOCKET_RUNTIME_PLATFORM_H
#define SOCKET_RUNTIME_PLATFORM_H

#include "platform/system.hh"
#include "platform/types.hh"

namespace ssc::runtime {
  using namespace ssc::runtime::types;
  struct RuntimePlatform {
    const String arch;
    const String os;
    bool mac = false;
    bool ios = false;
    bool win = false;
    bool android = false;
    bool linux = false;
    bool unix = false;
  };

  extern const RuntimePlatform platform;
  void msleep (uint64_t ms);
}
#endif
