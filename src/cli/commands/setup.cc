#include "../android.hh"
#include "../cli.hh"
#include "../io.hh"

namespace SSC::CLI::commands {
  Setup::Setup (Program& program) : Command("setup", gHelpTextSetup) {
    this->option(Option {
      "--platform",
      "-P",
      Option::Type::Multiple,
      Option::Requirement::Optional,
      Option::Expected {
        {"android", "darwin", "desktop", "ios", "linux", "mac", "windows", "win"}
      }
    });

    this->option("--yes", "-y");
    this->option("--debug", "-D");
    this->option("--quiet", "-q");
    this->option("--verbose", "-V");

    this->callback = [](const Command& command, const Options& options) {
      Vector<String> targetPlatforms;

      auto program = Program::instance();
      auto quiet = false;
      auto yes = program->rc.get("build.quiet") == "true";

      for (const auto& option : options) {
        if (option == "--platform") {
          for (const auto& targetPlatform : option.values()) {
            if (targetPlatform == "desktop") {
              if (platform.win) {
                targetPlatforms.push_back("windows");
              } else if (platform.mac) {
                targetPlatforms.push_back("darwin");
              } else {
                targetPlatforms.push_back(platform.os);
              }
            } else if (targetPlatform == "mac") {
              targetPlatforms.push_back("darwin");
            } else {
              targetPlatforms.push_back(targetPlatform);
            }
          }
        }

        if (option == "--quiet") {
          quiet = true;
        }

        if (option == "--yes") {
          yes = true;
        }
      }

      if (targetPlatforms.size() == 0) {
        if (platform.win) {
          targetPlatforms.push_back("windows");
        } else if (platform.linux) {
          targetPlatforms.push_back("linux");
        } else if (platform.mac) {
          targetPlatforms.push_back("darwin");
        }
      }

      // validate target platforms
      for (const auto& targetPlatform : targetPlatforms) {
        String scriptHost;
        String yesArgument;
        String argument;
        Path script;

        if (targetPlatform == "windows" && !platform.win) {
          program->logger.error("Windows build dependencies can only be installed on Windows");
          return false;
        }

        if (targetPlatform == "linux" && !platform.linux) {
          program->logger.error("Linux build dependencies can only be installed on Linux");
          return false;
        }

        if (platform.win) {
          scriptHost = "powershell.exe";
          script = program->env.SOCKET_HOME() + "bin\\install.ps1";

          if (targetPlatform.size() > 0) {
            argument = "-fte:" + targetPlatform;
          } else {
            argument = "-fte:windows";
          }

          yesArgument = yes ? "-yesdeps" : "";
        } else if (platform.linux || platform.mac) {
          scriptHost = "bash";
          script = program->env.SOCKET_HOME() + "bin/functions.sh";
          argument = "--fte " + targetPlatform;
          yesArgument = yes ? "--yes-deps" : "";
        } else {
          argument = "--" + targetPlatform + "-fte";

          if (targetPlatform == "android") {
            script = program->env.SOCKET_HOME() + "bin/android-functions.sh";
          }

          yesArgument = yes ? "--yes-deps" : "";
        }

        if (!fs::exists(script)) {
          program->logger.error("Setup script not found: '" + script.string() + "'");
          return false;
        }

        fs::current_path(program->env.SOCKET_HOME());

        program->logger.info(
          "Running setup for platform '" + targetPlatform + "' in "
          "SOCKET_HOME (" + program->env.SOCKET_HOME() + ")"
        );

        const auto command = (
          scriptHost +
          " \"" + script.string() + "\" " +
          argument + " " +
          yesArgument
        );

        const auto result = std::system(command.c_str());
        const auto status = WEXITSTATUS(result);

        if (status != 0) {
          return false;
        }

        Android android;
        if (!android.sdk.setup()) {
          return false;
        }
      }

      return true;
    };
  }
}
