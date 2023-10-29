#include "android.hh"
#include "cli.hh"

namespace SSC::CLI {
  Android::Application::Application (Android& android) : android(android) {}
  Android::Artifacts::Artifacts (Android& android) : android(android) {}
  Android::Emulator::Emulator (Android& android) : android(android) {}
  Android::SDK::SDK (Android& android) : android(android) {}

  Android::Android () :
    application(*this),
    artifacts(*this),
    emulator(*this),
    sdk(*this)
  {}

  Android::Device::Device (const String& name) : name(name) {}

  const String Android::Device::str () const {
    return this->name;
  }

  const JSON::Object Android::Device::json () const {
    return JSON::Object::Entries {
      {"name", this->name},
      {"status", this->status},
      {"product", this->product},
      {"model", this->model},
      {"device", this->device},
      {"type",
        this->type == Type::Emulator
          ? "Emulator"
          : this->type == Type::Phone
            ? "Android"
            : "TV"
      }
    };
  }

  bool Android::Device::isEmulator () const {
    return this->type == Type::Emulator;
  }

  bool Android::Device::isTablet () const {
    return this->type == Type::Tablet;
  }

  bool Android::Device::isPhone () const {
    return this->type == Type::Phone;
  }

  bool Android::Device::isTV () const {
    return this->type == Type::TV;
  }

  Vector<Android::Device> Android::devices () {
    static auto program = Program::instance();
    const auto result = this->sdk.adb("devices -l | tail -n +2");

    Vector<Device> devices;

    if (result.exitCode != 0) {
      if (program->isVerbose()) {
        program->logger.warn("adb: Failed to list devices");
        IO::write(result.output);
      }
      return devices;
    }

    auto output = result.output;
    std::regex regex(R"(\s+|(?=:))");
    std::smatch matches;

    if (std::regex_search(output, matches, std::regex(".+"))) {
      for (const auto& match : matches) {
        const auto entry = match.str();
        std::sregex_token_iterator it(entry.begin(), entry.end(), regex, -1);
        std::sregex_token_iterator end;

        Device device;
        while (it != end) {
          const auto key = it->str();

          if (device.name.size() == 0) {
            device.name = key;
            if (key.starts_with("emulator-")) {
              device.type = Device::Type::Emulator;
            }
          } else if (device.status.size() == 0) {
            device.status = key;
          } else if (key == "product") {
            device.product = (++it)->str();
            if (
              contains(device.product, "Tablet") ||
              contains(device.product, "Tab") ||
              contains(device.product, "tablet") ||
              contains(device.product, "tab")
            ) {
              device.type = Device::Type::Tablet;
            }
          } else if (key == "model") {
            device.model = (++it)->str();
          } else if (key == "device") {
            device.device = (++it)->str();
            if (contains(device.model, "atv")) {
              device.type = Device::Type::TV;
            } else if (contains(device.model, "emu64")) {
              device.type = Device::Type::Emulator;
            }
          }

          ++it;
        }

        devices.push_back(device);
      }
    }

    return devices;
  }

  const ExecOutput Android::SDK::adb (
    const String& command,
    const Vector<String> arguments,
    bool useSystem
  ) const {
    static const auto program = Program::instance();
    static const auto ANDROID_HOME = program->env.ANDROID_HOME();
    static const auto adb = program->userConfig.contains("android.adb_path")
      ? program->userConfig.get("android.adb_path")
      : ANDROID_HOME + (
        // path prefix in `ANDROID_HOME`
        String(platform.win ? "\\platform-tools\\" : "/platform-tools/") +
        // `adb` commmand name
        "adb" +
        // `adb` command postfix
        (platform.win ? ".exe" : "")
      );

    if (!fs::exists(adb)) {
      Program::instance()->logger.warn(
        "adb: Failed to locate 'adb' command at " + adb
      );

      return ExecOutput { "", 1 };
    }

    // run adb from ANDROID_HOME to prevent file lock
    // issues in application build folder on Windows
    const auto cwd = fs::current_path();
    fs::current_path(ANDROID_HOME);
    auto deviceQuery = exec(adb + " devices");
    fs::current_path(cwd);

    const auto commandString = join(concat(Vector<String> { adb, command }, arguments), " ");

    if (useSystem) {
      const auto result = std::system(commandString.c_str());
      return ExecOutput { .output = "", .exitCode = WEXITSTATUS(result) };
    }

    return exec(commandString);
  }

  const ExecOutput Android::SDK::avdmanager (
    const String& command,
    const Vector<String> arguments,
    bool useSystem
  ) const {
    static const auto program = Program::instance();
    static const auto ANDROID_HOME = Program::instance()->env.ANDROID_HOME();
    static const auto ANDROID_AVD_MANAGER = Env::get("ANDROID_AVD_MANAGER");
    static const auto ANDROID_SDK_MANAGER = Env::get("ANDROID_SDK_MANAGER");
    static const auto avdmanager = program->userConfig.contains("android.avdmanager_path")
      ? program->userConfig.get("android.avdmanager_path")
      : ANDROID_HOME + (platform.win ? "\\" : "/") + (
        ANDROID_AVD_MANAGER.size()
         ? ANDROID_AVD_MANAGER
         : ANDROID_SDK_MANAGER.size() > 0
           ? replace(ANDROID_SDK_MANAGER, "sdkmanager", "avdmanager")
           : platform.win
             ? "\\cmdline-tools\\latest\\bin\\avdmanager"
             : "/cmdline-tools/latest/bin/avdmanager"
      );

    if (!fs::exists(avdmanager)) {
      Program::instance()->logger.warn(
        "avdmanager: Failed to locate 'avdmanager' command at " + avdmanager
      );

      return ExecOutput { "", 1 };
    }

    const auto commandString = join(concat(Vector<String> { avdmanager, command }, arguments), " ");

    if (useSystem) {
      const auto result = std::system(commandString.c_str());
      return ExecOutput { .output = "", .exitCode = WEXITSTATUS(result) };
    }

    return exec(commandString);
  }

  const ExecOutput Android::SDK::sdkmanager (
    const String& command,
    const Vector<String> arguments,
    bool useSystem
  ) const {
    static const auto program = Program::instance();
    static const auto ANDROID_HOME = program->env.ANDROID_HOME();
    static const auto ANDROID_SDK_MANAGER = Env::get("ANDROID_SDK_MANAGER");
    static const auto sdkmanager = program->userConfig.contains("android.sdkmanager_path")
      ? program->userConfig.get("android.sdkmanager_path")
      : ANDROID_HOME + (platform.win ? "\\" : "/") + (
        ANDROID_SDK_MANAGER.size() > 0
          ? ANDROID_SDK_MANAGER
          : platform.win
             ? "\\cmdline-tools\\latest\\bin\\sdkmanager"
             : "/cmdline-tools/latest/bin/sdkmanager"
      );

    if (!fs::exists(sdkmanager)) {
      Program::instance()->logger.warn(
        "sdkmanager: Failed to locate 'sdkmanager' command at " + sdkmanager
      );

      return ExecOutput { "", 1 };
    }

    const auto commandString = join(concat(Vector<String> { sdkmanager, command }, arguments), " ");

    if (useSystem) {
      const auto result = std::system(commandString.c_str());
      return ExecOutput { .output = "", .exitCode = WEXITSTATUS(result) };
    }

    return exec(commandString);
  }

  bool Android::SDK::setup () const {
    static const auto program = Program::instance();
    static const auto quote = String(!platform.win ? "'" : "\"");

    static const auto SKIP_ANDROID_AVD_MANAGER = (
      Env::get("SSC_SKIP_ANDROID_AVD_MANAGER").size() > 0 ||
      program->userConfig.get("android.skip_avd_manager") == "true"
    );

    static const auto SKIP_ANDROID_SDK_MANAGER = (
      Env::get("SSC_SKIP_ANDROID_SDK_MANAGER").size() > 0 ||
      program->userConfig.get("android.skip_sdk_manager") == "true"
    );

    Vector<String> packages;

    packages.push_back(quote + "ndk;" + ANDROID_NDK_VERSION + quote);
    packages.push_back(quote + "emulator" + quote);
    packages.push_back(quote + "patcher;v4" + quote);
    packages.push_back(quote + "platforms;" + ANDROID_PLATFORM + quote);
    packages.push_back(quote + "platform-tools" + quote);

    packages.push_back(
      quote + "system-images;" + ANDROID_PLATFORM + ";" + "google_apis;x86_64" + quote
    );

    packages.push_back(
      quote + "system-images;" + ANDROID_PLATFORM + ";" + "google_apis;arm64-v8a" + quote
    );

    if (!SKIP_ANDROID_SDK_MANAGER) {
      const auto command = join(packages, " ");

      if (program->isVerbose()) {
        program->logger.info("sdkmanager " + command);
      }

      if (this->sdkmanager(command, Vector<String>{}, true).exitCode != 0) {
        program->logger.error("sdkmanager: Failed to initialize Android SDK");
        return false;
      }
    }

    if (!SKIP_ANDROID_AVD_MANAGER) {
      const auto devices = this->avdmanager("list").output;

      if (program->isVerbose()) {
        program->logger.info(
          "Checking if the " + ANDROID_AVD_DEVICE_NAME + "Android virtual device exists"
        );
      }

      const auto avdDeviceExists = (
        devices.find(ANDROID_AVD_DEVICE_NAME) != String::npos
      );

      if (!avdDeviceExists) {
        const auto arch = replace(platform.arch, "arm64", "arm64-v8a");
        const auto command = String("create avd");
        const auto arguments = Vector<String> {
          "--device 30",
          "--force",
          String("--name ") + ANDROID_AVD_DEVICE_NAME,
          String("--abi google_apis/") + arch,
          "--package", quote + "system-images;" + ANDROID_PLATFORM + ";google_apis;" + arch + quote
        };

        const auto result = this->avdmanager(command, arguments);

        if (result.exitCode != 0) {
          program->logger.error(
            "avdmanager: Failed to create device '" + ANDROID_AVD_DEVICE_NAME + "': " + result.output
          );
          return false;
        }
      }
    }

    return true;
  }

  bool Android::Emulator::boot () {
    const auto program = Program::instance();

    static const auto slash = String(!platform.win ? "/" : "\\");
    static const auto ANDROID_HOME = program->env.ANDROID_HOME();
    static const auto emulator = (
      ANDROID_HOME + slash + "emulator" + slash + "emulator" + (platform.win ? ".exe" : "")
    );

    if (!fs::exists(emulator)) {
      program->logger.error("emulator: Failed to locate Android Emulator");
      return false;
    }

    int totalTimeWaitingForBoot = 0;
    StringStream stream;

    // platform-33: swiftshader not supported below platform-32
    const auto arguments = String("@SSCAVD -gpu swiftshader_indirect");
    const auto onOutput = [&program, &stream](const auto& output) {
      if (program->isVerbose()) {
        IO::write(output);
      } else {
        stream << output << std::endl;
      }
    };

    const auto onExit = [&program, this](const auto& output) {
      this->isRunning = false;
    };

    if (!this->android.sdk.setup()) {
      return false;
    }

    auto process = Process(
      emulator,
      // TODO(jwerle): fix how `SSC::Process` joins `command` and `argv`
      // so a space prefix is not required below
      " " + arguments,
      ANDROID_HOME,
      onOutput,
      onOutput,
      onExit
    );

    program->logger.info("emulator: Waiting for Android Emulator to boot...");
    process.open();
    this->isBooting = true;

    while (true) {
      msleep(ANDROID_TASK_SLEEP_TIME);
      totalTimeWaitingForBoot += ANDROID_TASK_SLEEP_TIME;

      const auto arguments = Vector<String> {
        "getprop",
        "sys.boot_completed" + String(SSC_NULL_DEVICE_REDIRECT)
      };

      const auto result = this->android.sdk.adb("shell", arguments, true);

      if (result.exitCode == 0) {
        this->isRunning = true;
        program->logger.info("emulator: OK. Android Emulator booted");
        break;
      }

      if (process.closed) {
        auto const status = std::to_string(process.status);
        program->logger.warn(
          "emulator: Android Emulator exited with code: " + status
        );
        break;
      }

      if (totalTimeWaitingForBoot >= ANDROID_TASK_TIMEOUT) {
        program->logger.error(
          "emulator: Android Emulator failed to start before time out"
        );
        break;
      }
    }

    this->isBooting = false;

    if (process.status != 0 && !program->isVerbose()) {
      IO::write(stream.str(), true);
    }

    return true;
  }

  const String Android::Artifacts::apk (const Path& path, const Type type) const {
    const auto outputs = path / "build" / "outputs";
    StringStream stream;

    stream << "app";
    if (type <= Type::DevRelease) {
      stream << "-dev";
      if (type == Type::DevDebug) {
        stream << "-debug";
      } else if (type == Type::DevReleaseSigned) {
        stream << "-release-signed";
      } else if (type == Type::DevReleaseUnsigned) {
        stream << "-release-unsigned";
      }
    } else if (type <= Type::LiveRelease) {
      stream << "-live";
      if (type == Type::LiveDebug) {
        stream << "-debug";
      } else if (type == Type::LiveReleaseSigned) {
        stream << "-release-signed";
      } else if (type == Type::LiveReleaseUnsigned) {
        stream << "-release-unsigned";
      }
    }

    stream << ".apk";
    const auto filename = stream.str();
    return (
      outputs /
      "apk" /
      (type <= Type::DevRelease ? "dev" : "live") /
      (type == Type::DevDebug || type == Type::LiveDebug ? "debug" : "release") /
      filename
    );
  }

  const String Android::Artifacts::bundle (const Path& path, const Type type) const {
    const auto outputs = path / "build" / "outputs";
    String directory;
    StringStream stream;

    stream << "app";
    if (type <= Type::DevRelease) {
      stream << "-dev";
      if (type == Type::DevDebug) {
        stream << "-debug";
        directory = "devDebug";
      } else {
        stream << "-release";
        directory = "devRelease";
      }
    } else if (type <= Type::LiveRelease) {
      stream << "-live";
      if (type == Type::LiveDebug) {
        stream << "-debug";
        directory = "liveDebug";
      } else {
        stream << "-release";
        directory = "liveRelease";
      }
    }

    stream << ".aab";
    const auto filename = stream.str();

    stream.clear();
    return outputs / "bundle" / directory / filename;
  }

  bool Android::Application::install (
    const Path& path,
    const Target target,
    const Artifacts::Type type
  ) {
    static const auto devices = this->android.devices();
    static auto program = Program::instance();

    Device targetDevice;

    if (target == Target::Emulator) {
      for (const auto& device : devices) {
        if (device.status == "device" && device.isEmulator()) {
          targetDevice = device;
          break;
        }
      }
    } else if (target == Target::Device) {
      for (const auto& device : devices) {
        if (device.status == "device" && !device.isEmulator()) {
          targetDevice = device;
          break;
        }
      }
    }

    if (target != Target::Any && targetDevice.name.size() == 0) {
      program->logger.error(
        "android: Failed to select target device"
      );

      program->logger.error(
        "android: Ensure your target device serial number appears (without UNAUTHORIZED) when running"
      );

      program->logger.error(
        "android: Please run 'ssc list-device --platform android' (or 'adb devices')"
      );

      return false;
    }

    return this->install(path, targetDevice, type);
  }

  bool Android::Application::install (
    const Path& path,
    const Device& device,
    const Artifacts::Type type
  ) {
    static const auto devices = this->android.devices();
    static auto program = Program::instance();
    const auto apk = this->android.artifacts.apk(path, type);

    Vector<String> arguments;
    Device targetDevice;
    bool success = false;
    int totalTimeWaitingForInstall = 0;

    for (const auto& d: devices) {
      if (d.status == "device" && device.name == d.name) {
        targetDevice = device;
        arguments.push_back("-s");
        arguments.push_back(device.name);
        break;
      }
    }

    if (arguments.size() == 0) {
      program->logger.error(
        "adb: Target device could not be found"
      );

      program->logger.error(
        "adb: Ensure your target device serial number appears (without UNAUTHORIZED) when running"
      );

      program->logger.error(
        "adb: Please run 'ssc list-device --platform android' (or 'adb devices')"
      );

      return false;
    }

    arguments.push_back(apk);

    // handle emulator boot issue: cmd: Can't find service: package
    // no reliable way of detecting when emulator is ready without a blocking
    // logcat call. Note that there are several different errors that can
    // occur here based on the state of the emulator, just keep trying to
    // install with a timeout
    while (true) {
      const auto result = this->android.sdk.adb("install", arguments);
      if (result.exitCode == 0) {
        if (program->isVerbose()) {
          IO::write(result.output);
        }
        success = true;
        break;
      }

      msleep(ANDROID_TASK_SLEEP_TIME);
      totalTimeWaitingForInstall += ANDROID_TASK_SLEEP_TIME;

      if (totalTimeWaitingForInstall >= ANDROID_TASK_TIMEOUT) {
        program->logger.error(
          "adb: Failed to install APK to device before time out"
        );

        program->logger.error(
          "adb: Ensure your target device serial number appears (without UNAUTHORIZED) when running"
        );

        program->logger.error(
          "adb: Please run 'ssc list-device --platform android' (or 'adb devices')"
        );

        if (program->isVerbose()) {
          IO::write(result.output, true);
        }
        break;
      }
    }

    return success;
  }
}
