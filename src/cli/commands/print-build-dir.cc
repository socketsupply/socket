#include "../android.hh"
#include "../apple.hh"
#include "../cli.hh"
#include "../io.hh"

namespace SSC::CLI::commands {
  PrintBuildDir::PrintBuildDir (Program& program) :
    Command("print-build-dir", gHelpTextPrintBuildDir)
  {
    this->option(Option {
      "--platform",
      "-P",
      Option::Type::Multiple,
      Option::Requirement::Optional,
      Option::Expected {{
        "android",
        "android-emulator",
        "desktop",
        "ios",
        "ios-simulator"
      }}
    });

    this->option("--root");

    this->callback = [&program](const Command& command, const Options& options) {
      String targetPlatform = platform.os;
      String targetPath;
      bool printRootPath = false;

      for (const auto& option : options) {
        if (option.isPositional() && targetPath.size() == 0) {
          targetPath = option.value();
        }

        if (option == "--platform") {
          targetPlatform = option.value();
        }

        if (option == "--root") {
          printRootPath = true;
        }
      }

      if (targetPlatform == "desktop") {
        targetPlatform = platform.os;
      }

      if (targetPath.size() == 0) {
        targetPath = fs::current_path().string();
      } else {
        targetPath = fs::absolute(targetPath).lexically_normal().string();
      }

      if (!fs::exists(targetPath)) {
        program.logger.error(
          format("Target path does not exist: '{}'", targetPath)
        );
        return false;
      }

      fs::current_path(targetPath);

      if (!program.preloadUserConfig(targetPath)) {
        return false;
      }

      const auto paths = program.getPaths(targetPath, targetPlatform);

      if (printRootPath) {
        IO::write(paths.output);
      } else {
        IO::write(paths.resources);
      }

      return true;
    };
  }
}
