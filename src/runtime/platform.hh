#ifndef SOCKET_RUNTIME_PLATFORM_H
#define SOCKET_RUNTIME_PLATFORM_H

#include "platform/system.hh"

namespace ssc::runtime {
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
  uint64_t rand64 ();
}

#endif
