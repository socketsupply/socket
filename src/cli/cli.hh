#ifndef SSC_CLI_HH
#define SSC_CLI_HH

#include "../core/platform.hh"
#include "../core/string.hh"
#include "../core/env.hh"

#include <signal.h>

namespace SSC::CLI {
  inline void notify (int signal) {
  #if !defined(_WIN32)
    static auto ppid = Env::get("SSC_CLI_PID");
    static auto pid = ppid.size() > 0 ? std::stoi(ppid) : 0;
    if (pid > 0) {
      kill(pid, signal);
    }
  #endif
  }

  inline void notify () {
  #if !defined(_WIN32)
    return notify(SIGUSR1);
  #endif
  }
}
#endif
