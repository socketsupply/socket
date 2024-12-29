#ifndef SOCKET_RUNTIME_IO_H
#define SOCKET_RUNTIME_IO_H

#include <iostream>

#include "../cli/cli.hh"
#include "platform.hh"
#include "env.hh"

namespace ssc::runtime::io {
  inline void write (const String& input, bool isErrorOutput) {
    static const auto GITHUB_ACTIONS_CI = env::get("GITHUB_ACTIONS_CI");
    static const auto isGitHubActionsCI = GITHUB_ACTIONS_CI.size() > 0;
    auto& stream = isErrorOutput ? std::cerr : std::cout;

    stream << input;

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    if (isGitHubActionsCI) {
      CLI::notify();
      // skip writing newline if running on Windows GHA CI
      return;
    }

  #endif
    stream << std::endl;

    cli::notify();
  }
}
#endif
