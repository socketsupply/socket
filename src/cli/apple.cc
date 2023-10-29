#include "cli.hh"
#include "apple.hh"

namespace SSC::CLI {
  static constexpr auto APPLE_CONFIGURATOR_CFGUTIL_PATH =  "/Applications/Apple Configurator.app/Contents/MacOS/cfgutil";

  Apple::Device::Device (const String& name) : name(name) {}

  const String Apple::Device::str () const {
    return this->name;
  }

  const JSON::Object Apple::Device::json () const {
    return JSON::Object::Entries {
      {"name", this->name},
      {"identifier", this->identifier},
      {"edid", this->ecid},
      {"udid", this->udid},
      {"type",
        this->type == Type::Desktop
          ? "MacOS"
          : this->type == Type::Simulator
            ? "Simulator"
            : this->type == Type::Tablet
              ? "iPad"
              : "iPhone"
      }
    };
  }

  bool Apple::Device::isSimulator () const {
    return this->type == Type::Simulator;
  }

  bool Apple::Device::isDesktop () const {
    return this->type == Type::Desktop;
  }

  bool Apple::Device::isTablet () const {
    return this->type == Type::Tablet;
  }

  bool Apple::Device::isPhone () const {
    return this->type == Type::Phone;
  }

  bool Apple::Device::isTV () const {
    return this->type == Type::TV;
  }

  Apple::Application::Application (Apple& apple) : apple(apple) {}
  Apple::Simulator::Simulator (Apple& apple) : apple(apple) {}
  Apple::SDK::SDK (Apple& apple) : apple(apple) {}

  Apple::Apple () :
    application(*this),
    simulator(*this),
    sdk(*this)
  {}

  const ExecOutput Apple::SDK::cfgutil (
    const String& command,
    const Vector<String> arguments,
    bool useSystem
  ) const {
    static auto program = Program::instance();
    static const auto cfgutilInPath = trim(exec("which cfgutil").output);
    // The `cfgutil` command was not accessible in the user `$PATH` and we'll
    // possiblly fallback to the static `APPLE_CONFIGURATOR_CFGUTIL_PATH` path
    // to the `cfgutil` command
    static const auto cfgutil = cfgutilInPath.size() > 0
      ? cfgutilInPath
      : APPLE_CONFIGURATOR_CFGUTIL_PATH;

    // warn the user that they should install 'Automation Tootls' if the
    // 'Apple Configurator' application is installed, otherwise recommend
    // they install that first
    if (cfgutilInPath.size() == 0) {
      if (fs::exists(APPLE_CONFIGURATOR_CFGUTIL_PATH)) {
        program->logger.warn(
          "It is highly recommend to install 'Automation Tools' from the 'Apple Configurator' main menu"
        );
      } else {
        program->logger.warn(
          "Please install 'Apple Configurator' from 'https://apps.apple.com/us/app/apple-configurator/id1037126344'"
        );

        return ExecOutput { "", 1 };
      }
    }

    const auto commandString = join(concat(Vector<String> { cfgutil, command }, arguments), " ");

    if (useSystem) {
      const auto result = std::system(commandString.c_str());
      return ExecOutput { .output = "", .exitCode = WEXITSTATUS(result) };
    }

    return exec(commandString);
  }

  const ExecOutput Apple::SDK::simctl (
    const String& command,
    const Vector<String> arguments,
    bool useSystem
  ) const {
    static auto program = Program::instance();
    static const auto simctl = String("xcrun simctl");

    const auto commandString = join(concat(Vector<String> { simctl, command }, arguments), " ");

    if (useSystem) {
      const auto result = std::system(commandString.c_str());
      return ExecOutput { .output = "", .exitCode = WEXITSTATUS(result) };
    }

    return exec(commandString);
  }

  const ExecOutput Apple::SDK::installer (
    const String& command,
    const Vector<String> arguments,
    bool useSystem
  ) const {
    static auto program = Program::instance();
    static const auto installer = String("installer");
    const auto maybeSudo = contains(arguments, "/") ? "sudo" : "";
    const auto maybeVerbose = program->isVerbose() ? "-verbose" : "";

    const auto commandString = join(concat(
      Vector<String> {
        maybeSudo,
        installer,
        command,
        maybeVerbose
      },
      arguments
    ), " ");

    if (useSystem) {
      const auto result = std::system(commandString.c_str());
      return ExecOutput { .output = "", .exitCode = WEXITSTATUS(result) };
    }

    return exec(commandString);
  }

  bool Apple::SDK::setup () const {
    static const auto program = Program::instance();
    return true;
  }

  bool Apple::Application::start (const Target target) {
    return true;
  }

  bool Apple::Application::install (
    const Path& path,
    const Target target,
    const InstallOptions& options
  ) {
    static const auto devices = this->apple.devices();
    static auto program = Program::instance();
    Device targetDevice;

    if (target == Target::Desktop) {
      for (const auto& device : devices) {
        if (device.isDesktop()) {
          targetDevice = device;
          break;
        }
      }
    } else if (target == Target::iPhone) {
      for (const auto& device : devices) {
        if (device.isPhone()) {
          targetDevice = device;
          break;
        }
      }
    } else if (target == Target::iPhoneSimulator) {
      for (const auto& device : devices) {
        if (device.isSimulator()) {
          targetDevice = device;
          break;
        }
      }
    }

    if (targetDevice.name.size() == 0) {
      program->logger.error(
        "apple: Failed to select target device"
      );

      if (target == Target::iPhone) {
        program->logger.error(
          "apple: Please run 'ssc list-device --platform ios' (or 'cfgutil list-devices')"
        );
      } else if (target == Target::iPhoneSimulator) {
        program->logger.error(
          "apple: Please run 'ssc list-device --platform ios-simulator' (or 'xcrun simctl list')"
        );
      } else {
        program->logger.error(
          "apple: Unable to determine target device for application install)"
        );
      }

      return false;
    }

    return this->install(path, targetDevice, options);
  }

  bool Apple::Application::install (
    const Path& path,
    const Device& device,
    const InstallOptions& options
  ) {
    static const auto devices = this->apple.devices();
    static auto program = Program::instance();

    if (device.isPhone() || device.isSimulator()) {
      const auto paths = program->getPaths(path, device.isPhone() ? "ios" : "ios-simulator");
      const auto result = this->apple.sdk.cfgutil("install-app", { paths.archive });

      if (result.exitCode != 0) {
        if (program->isVerbose()) {
          IO::write(result.output);
        }
        return false;
      }
    } else if (device.isDesktop()) {
      const auto name = program->userConfig.get("build.name");
      const auto paths = program->getPaths(path, platform.os);
      const auto destination = options.destination.size() > 0
        ? options.destination
        : "/";

      const auto pkg = paths.output / (name + ".pkg");
      const auto zip = paths.output / (name + ".zip");
      const auto app = paths.output / (name + ".app");

      if (fs::exists(pkg)) {
        const auto result = this->apple.sdk.installer(format(
          "-pkg {} -target {}",
          pkg.string(),
          destination
        ));
      }
    } else {
      return false;
    }

    return true;
  }

  Vector<Apple::Device> Apple::devices () {
    static auto program = Program::instance();
    Vector<Apple::Device> devices;

    do {
      auto result = this->sdk.cfgutil("list-devices");

      if (result.exitCode != 0) {
        if (program->isVerbose()) {
          IO::write(result.output);
        }

        return devices;
      }

      std::regex regex(R"(Type:\s(\S*)\s*ECID:\s(\S*)\s*UDID:\s(\S*).*Name:\s(.*))");
      std::smatch matches;
      String output = result.output;

      while (std::regex_search(output, matches, regex)) {
        const auto type = trim(matches[1]);
        const auto ecid = trim(matches[2]);
        const auto udid = trim(matches[3]);
        const auto name = trim(matches[4]);
        Apple::Device device;

        if (type.starts_with("iPhone")) {
          device.type = Device::Type::Phone;
        } else if (type.starts_with("iPad")) {
          device.type = Device::Type::Tablet;
        } else if (type.starts_with("AppleTV")) {
          device.type = Device::Type::TV;
        }

        device.ecid = ecid;
        device.udid = udid;
        device.name = name;
        device.identifier = udid;
        devices.push_back(device);

        output = matches.suffix();

      }
    } while (0);

    do {
      const auto result = this->sdk.simctl("list | grep Booted");

      if (result.exitCode != 0) {
        if (program->isVerbose()) {
          IO::write(result.output);
        }

        return devices;
      }

      std::regex regex(R"((.*)\s \((.*)\)\s\(Booted\))");
      std::smatch matches;
      String output = trim(result.output);

      while (std::regex_search(output, matches, regex)) {
        const auto name = trim(matches[1]);
        const auto identifier = trim(matches[2]);

        Apple::Device device;
        device.name = name;
        device.identifier = identifier;
        device.type = Device::Type::Simulator;
        devices.push_back(device);

        output = matches.suffix();
      }
    } while (0);

    return devices;
  }
}
