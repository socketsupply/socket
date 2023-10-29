#include "../android.hh"
#include "../apple.hh"
#include "../cli.hh"
#include "../io.hh"

namespace SSC::CLI::commands {
  InstallApp::InstallApp (Program& program) : Command("install-app", gHelpTextInstallApp) {
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

    this->option("--device", "-d", Option::Type::Single);
    this->option("--target", "-T", Option::Type::Multiple);

    this->callback = [&program](const Command& command, const Options& options) {
      Vector<String> targetPlatforms;
      String targetDevice;
      String targetPath;

      for (const auto& option : options) {
        if (option.isPositional() && targetPath.size() == 0) {
          targetPath = option.value();
        }

        if (option == "--device") {
          targetDevice = option.value();
        }

        if (option == "--target") {
          for (const auto& targetPlatform : option.values()) {
            if (targetPlatform == "desktop") {
              targetPlatforms.push_back(platform.os);
            } else {
              targetPlatforms.push_back(targetPlatform);
            }
          }
        }
      }

      if (targetDevice.size() > 0 && targetPlatforms.size() > 1) {
        program.logger.error(
          "Cannot specify multiple platforms and with '--device'"
        );
        program.exit(1);
        return false;
      }

      if (targetPlatforms.size() == 0) {
        if (!platform.mac && !platform.linux) {
          program.logger.error(
            "Unsupported host desktop platform: Only 'macOS' and 'Linux' is supported"
          );
          program.exit(1);
          return false;
        }

        targetPlatforms.push_back(platform.os);
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


      for (const auto& targetPlatform : targetPlatforms) {
        if (targetPlatform == "ios") {
        } else if (targetPlatform == "ios-simulator") {
        } else if (targetPlatform == "android") {
          Android android;
          if (targetDevice.size() > 0) {
            const auto installed = android.application.install(
              targetPath,
              targetDevice,
              program.isDebug()
                ? Android::Artifacts::Type::DevDebug
                : Android::Artifacts::Type::LiveDebug
            );
          } else {
            const auto installed = android.application.install(
              targetPath,
              Android::Application::Target::Any,
              program.isDebug()
                ? Android::Artifacts::Type::DevDebug
                : Android::Artifacts::Type::LiveDebug
            );
          }
        } else if (targetPlatform == "android-emulator") {
          Android android;
          if (targetDevice.size() > 0) {
            const auto installed = android.application.install(
              targetPath,
              targetDevice,
              program.isDebug()
                ? Android::Artifacts::Type::DevDebug
                : Android::Artifacts::Type::LiveDebug
            );
          } else {
            const auto installed = android.application.install(
              targetPath,
              Android::Application::Target::Emulator,
              program.isDebug()
                ? Android::Artifacts::Type::DevDebug
                : Android::Artifacts::Type::LiveDebug
            );
          }
        } else if (platform.mac) {
        } else if (platform.linux) {
        } else {
          program.logger.error(
            "Unsupported target platform: '" + targetPlatform + "'"
          );
          return false;
        }
      }

      return true;
    };
  }
}
