#include <iostream>

#include "../platform/platform.hh"
#include "../cli/cli.hh"
#include "env.hh"
#include "io.hh"

namespace SSC::IO {
  void write (const String& input, bool isErrorOutput) {
    static const auto GITHUB_ACTIONS_CI = Env::get("GITHUB_ACTIONS_CI");
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

    CLI::notify();
  }
}
