#include <ranges>

#include "../android.hh"
#include "../apple.hh"
#include "../cli.hh"
#include "../io.hh"

namespace SSC::CLI::commands {
  ListDevices::ListDevices (Program& program)
    : Command("list-devices", gHelpTextListDevices)
  {
    this->option("--ecid");
    this->option("--udid");
    this->option("--only");
    this->option(Option {
      "--platform",
      "-P",
      Option::Type::Single,
      Option::Requirement::Required,
      Option::Expected {{
        "android",
        "android-emulator",
        "ios",
        "ios-simulator"
      }}
    });

    this->option(Option {
      "--format",
      "-f",
      Option::Type::Single,
      Option::Requirement::Optional,
      Option::Expected {{
        "default",
        "json"
      }}
    });

    this->option(Option {
      "--type",
      "-t",
      Option::Type::Single,
      Option::Requirement::Optional,
      Option::Expected {{
        "simulator",
        "emulator",
        "phone"
      }}
    });

    this->callback = [](const Command& command, const Options& options) {
      String targetPlatform;
      String outputFormat;
      String deviceType;
      Apple apple;

      auto onlyFirstDevice = false;
      auto showECID = false;
      auto showUDID = false;
      auto program = Program::instance();

      for (const auto& option : options) {
        if (option.name == "--ecid") {
          showECID = true;
        }

        if (option.name == "--udid") {
          showUDID = true;
        }

        if (option.name == "--only") {
          onlyFirstDevice = true;
        }

        if (option.name == "--platform") {
          targetPlatform = option.value();
        }

        if (option.name == "--format") {
          outputFormat = option.value();
        }

        if (option.name == "--type") {
          deviceType = option.value();
        }
      }

      // default to 'simulator' as device type when platform is 'ios-simulator'
      if (targetPlatform == "ios-simulator") {
        if (deviceType.size() != 0) {
          program->logger.error(
            "'--type' cannot be set when '--platform' is 'ios-simulator'"
          );
          command.printHelp(std::cerr);
          return false;
        }

        deviceType = "simulator";
      }

      // default to 'emulator' as device type when platform is 'ios-emulator'
      if (targetPlatform == "android-emulator") {
        if (deviceType.size() != 0) {
          program->logger.error(
            "'--type' cannot be set when '--platform' is 'android-emulator'"
          );
          command.printHelp(std::cerr);
          return false;
        }

        deviceType = "emulator";
      }

      // validate '--ecid', '--udid' and '--type simulator' not given when
      // target platform is not 'ios' or 'ios-simulator'
      if (targetPlatform != "ios" && targetPlatform != "ios-simulator") {
        if (showUDID || showECID) {
          program->logger.error(
            "'--udid' and '--ecid' can only be used when '--platform' is 'ios' or 'ios-simulator'"
          );
          command.printHelp(std::cerr);
          return false;
        }

        if (deviceType == "simulator") {
          program->logger.error(
            "'--type' cannot be set to 'simulator' when '--platform' is not 'ios' or 'ios-simulator'"
          );
          command.printHelp(std::cerr);
          return false;
        }
      }

      if (targetPlatform != "android" && targetPlatform != "android-emulator") {
        if (deviceType == "emulator") {
          program->logger.error(
            "'--type' cannot be set to 'emulator' when '--platform' is not 'android' or 'android-emulator'"
          );
          command.printHelp(std::cerr);
          return false;
        }
      }

      if ((targetPlatform == "ios" || targetPlatform == "ios-simulator")) {
        if (!platform.mac) {
          program->logger.error(
            format("'--platform {}' is only supported on macOS", targetPlatform)
          );
          command.printHelp(std::cerr);
          return false;
        }

        if (showUDID && showECID) {
          program->logger.error("'--udid' and '--ecid' are mutually exclusive");
          command.printHelp(std::cerr);
          return false;
        }

        if (onlyFirstDevice && !showUDID && !showECID) {
          program->logger.error("'--only' requires '--udid' or '--ecid'");
          command.printHelp(std::cerr);
          return false;
        }

        // filter devices based on queried device type
        auto filtered = apple.devices() | std::views::filter([&](auto device) {
          if (deviceType.size() == 0) {
            return true;
          } else if (deviceType == "simulator" && device.isSimulator()) {
            return true;
          } else if (deviceType == "phone" && device.isPhone()) {
            return true;
          }

          return false;
        });

        if (filtered.empty()) {
          return true;
        }

        const auto devices = Vector<Apple::Device>(filtered.begin(), filtered.end());

        if (onlyFirstDevice) {
          const auto device = devices[0];
          if (showUDID) {
            IO::write(device.identifier);
          } else if (showECID) {
            IO::write(device.ecid);
          } else {
            if (outputFormat == "json") {
              IO::write(device.json().str());
            } else {
              IO::write(device.name);
              IO::write(format("ECID: {} UDID: {}", device.ecid, device.udid));
            }
          }

          return true;
        }

        JSON::Array outputs;
        for (const auto& device : devices) {
          if (showUDID) {
            if (outputFormat == "json") {
              outputs.push(JSON::Object::Entries {
                {device.name, device.identifier}
              });
            } else {
              IO::write(format("{}: {}", device.name, device.identifier));
            }
          } else if (showECID) {
            if (outputFormat == "json") {
              outputs.push(JSON::Object::Entries {
                {device.name, device.ecid}
              });
            } else {
              IO::write(format("{}: {}", device.name, device.ecid));
            }
          } else {
            outputs.push(device.json());
          }
        }

        if (outputFormat == "json") {
          IO::write(outputs.str());
        }
      } else if (targetPlatform == "android") {
        Android android;
        // filter devices based on queried device type
        auto filtered = android.devices() | std::views::filter([&](auto device) {
          if (deviceType.size() == 0) {
            return true;
          } else if (deviceType == "emulator" && device.isEmulator()) {
            return true;
          } else if (deviceType == "phone" && device.isPhone()) {
            return true;
          }

          return false;
        });

        if (filtered.empty()) {
          return true;
        }

        const auto devices = Vector<Android::Device>(filtered.begin(), filtered.end());
        if (onlyFirstDevice) {
          if (outputFormat == "json") {
            IO::write(devices[0].json().str());
          } else {
            IO::write(devices[0].name);
          }

          return true;
        }

        JSON::Array outputs;
        for (const auto& device : devices) {
          if (outputFormat == "json") {
            outputs.push(device.json());
          } else {
            IO::write(device.name);
          }
        }

        if (outputFormat == "json") {
          IO::write(outputs.str());
        }
      } else {
        program->logger.error("'ssc list-devices' is not supported for the target paltform");
        return false;
      }

      return true;
    };
  }
}
