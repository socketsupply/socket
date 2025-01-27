#include "../../cli.hh"

#include "../io.hh"

namespace ssc::runtime::io {
  void write (const String& input, bool isErrorOutput) {
    static const auto GITHUB_ACTIONS_CI = env::get("GITHUB_ACTIONS_CI");
    static const auto isGitHubActionsCI = GITHUB_ACTIONS_CI.size() > 0;
    auto& stream = isErrorOutput ? std::cerr : std::cout;

    stream << input;

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    if (isGitHubActionsCI) {
      ssc::cli::notify();
      // skip writing newline if running on Windows GHA CI
      return;
    }

  #endif
    stream << std::endl;

    ssc::cli::notify();
  }
}
