#include <iostream>

#include "../cli/cli.hh"
#include "env.hh"
#include "io.hh"

namespace SSC::IO {
  void write (const String& input, bool isErrorOutput) {
    static const auto GITHUB_ACTIONS_CI = Env::get("GITHUB_ACTIONS_CI");
    static const auto isGitHubActionsCI = GITHUB_ACTIONS_CI.size() > 0;
    auto& stream = isErrorOutput ? std::cerr : std::cout;

    stream << input;

    // skip writing newline if running on Windows GHA CI
  #if defined(_WIN32)
    if (isGitHubActionsCI) {
      CLI::notify();
      return;
    }

  #endif
    stream << std::endl;

    CLI::notify();
  }
}
