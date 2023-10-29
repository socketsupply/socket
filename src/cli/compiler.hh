#ifndef SSC_CLI_COMPILER_H
#define SSC_CLI_COMPILER_H

#include "../core/core.hh"

namespace SSC::CLI {
  class Compiler {
    public:
      enum class PlatformType {
        Unknown,
        Host,
        Desktop = Host,
        Android,
        AndroidEmulator,
        iOS,
        iOSSimulator
      };

      const PlatformType type;

      Set<String> cflags;
      Set<String> ldflags;

      Compiler (const PlatformType platformType);
  };
}
#endif
