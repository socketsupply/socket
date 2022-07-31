#include "common.hh"
#include "cli.hh"
#include "process.hh"
#include <filesystem>

#ifdef __linux__
#include <cstring>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <shlwapi.h>
#include <strsafe.h>
#include <comdef.h>
#include <AppxPackaging.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Urlmon.lib")
#define _stat stat
#else
#include <unistd.h>
#endif

#ifndef CMD_RUNNER
#define CMD_RUNNER
#endif

#ifndef BUILD_TIME
#define BUILD_TIME 0
#endif

using namespace SSC;
using namespace std::chrono;

auto start = std::chrono::system_clock::now();

bool flagDebugMode = true;
bool flagQuietMode = false;

void log (const std::string s) {
  if (flagQuietMode) return;
  if (s.size() == 0) return;

  #ifdef _WIN32 // unicode console support
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000);
  #endif

  auto now = system_clock::now();
  auto delta = duration_cast<milliseconds>(now - start).count();
  std::cout << "• " << s << " \033[0;32m+" << delta << "ms\033[0m" << std::endl;
  start = std::chrono::system_clock::now();
}

int runApp (const fs::path& path, const std::string& args, bool headless) {
  auto cmd = path.string();
  int status = 0;

  if (!fs::exists(path)) {
    log("executable not found: " + cmd);
    return 1;
  }

  auto runner = trim(std::string(STR_VALUE(CMD_RUNNER)));
  auto prefix = runner.size() > 0 ? runner + std::string(" ") : runner;

  if (headless) {
    auto runner = appData["headless_runner"];
    auto runnerFlags = appData["headless_runner_flags"];

    std::string headlessCommand = "";

    if (runner.size() == 0) {
      runner = appData["headless_" + platform.os + "_runner"];
    }

    if (runnerFlags.size() == 0) {
      runnerFlags = appData["headless_" + platform.os + "_runner_flags"];
    }

    if (platform.linux) {
      // use xvfb for linux as a default
      if (runner.size() == 0) {
        runner = "xvfb-run";
        status = std::system((runner + " --help >/dev/null").c_str());
        if (WEXITSTATUS(status) != 0) {
          runner = "";
        }
      }

      if (runnerFlags.size() == 0) {
        // use sane defaults if 'xvfb-run' is used
        if (runner == "xvfb-run") {
          runnerFlags = "--server-args='-screen 0 1920x1080x24'";
        }
      }

      if (runner != "false") {
        headlessCommand = runner + " " + runnerFlags + " ";
      }
    }

    status = std::system((headlessCommand + prefix + cmd + " " + args).c_str());
  } else {
    status = std::system((prefix + cmd + " " + args).c_str());
  }

  return WEXITSTATUS(status);
}

int runApp (const fs::path& path, const std::string& args) {
  return runApp(path, args, false);
}

void runIOSSimulator (const fs::path& path, Map& settings) {
  std::string deviceType;
  std::stringstream listDeviceTypesCommand;
  listDeviceTypesCommand
    << "xcrun"
    << " simctl"
    << " list devicetypes";

  auto rListDeviceTypes = exec(listDeviceTypesCommand.str().c_str());
  if (rListDeviceTypes.exitCode != 0) {
    log("failed to list device types using \"" + listDeviceTypesCommand.str() + "\"");
    if (rListDeviceTypes.output.size() > 0) {
      log(rListDeviceTypes.output);
    }
    exit(rListDeviceTypes.exitCode);
  }

  std::regex reDeviceType(settings["ios_simulator_device"] + "\\s\\((com.apple.CoreSimulator.SimDeviceType.(?:.+))\\)");
  std::smatch match;

  if (std::regex_search(rListDeviceTypes.output, match, reDeviceType)) {
    deviceType = match.str(1);
    log("simulator device type: " + deviceType);
  } else {
    auto rListDevices = exec("xcrun simctl list devicetypes | grep iPhone");
    log(
      "failed to find device type: " + settings["ios_simulator_device"] + ". " +
      "Please provide correct device name for the \"ios_simulator_device\". " +
      "The list of available devices:\n" + rListDevices.output
    );
    if (rListDevices.output.size() > 0) {
      log(rListDevices.output);
    }
    exit(rListDevices.exitCode);
  }

  std::stringstream listDevicesCommand;
  listDevicesCommand
    << "xcrun"
    << " simctl"
    << " list devices available";
  auto rListDevices = exec(listDevicesCommand.str().c_str());
  if (rListDevices.exitCode != 0) {
    log("failed to list available devices using \"" + listDevicesCommand.str() + "\"");
    if (rListDevices.output.size() > 0) {
      log(rListDevices.output);
    }
    exit(rListDevices.exitCode);
  }

  auto iosSimulatorDeviceSuffix = settings["ios_simulator_device"];
  std::replace(iosSimulatorDeviceSuffix.begin(), iosSimulatorDeviceSuffix.end(), ' ', '_');
  std::regex reSocketSDKDevice("SocketSDKSimulator_" + iosSimulatorDeviceSuffix + "\\s\\((.+)\\)\\s\\((.+)\\)");

  std::string uuid;
  bool booted = false;

  if (std::regex_search(rListDevices.output, match, reSocketSDKDevice)) {
    uuid = match.str(1);
    booted = match.str(2).find("Booted") != std::string::npos;

    log("found SocketSDK simulator VM for " + settings["ios_simulator_device"] + " with uuid: " + uuid);
    if (booted) {
      log("SocketSDK simulator VM is booted");
    } else {
      log("SocketSDK simulator VM is not booted");
    }
  } else {
    log("creating a new iOS simulator VM for " + settings["ios_simulator_device"]);

    std::stringstream listRuntimesCommand;
    listRuntimesCommand
      << "xcrun"
      << " simctl"
      << " list runtimes available";
    auto rListRuntimes = exec(listRuntimesCommand.str().c_str());
    if (rListRuntimes.exitCode != 0) {
      log("failed to list available runtimes using \"" + listRuntimesCommand.str() + "\"");
      if (rListRuntimes.output.size() > 0) {
        log(rListRuntimes.output);
      }
      exit(rListRuntimes.exitCode);
    }
    auto const runtimes = split(rListRuntimes.output, '\n');
    std::string runtime;
    // TODO: improve iOS version detection
    for (auto it = runtimes.rbegin(); it != runtimes.rend(); ++it) {
      if (it->find("iOS") != std::string::npos) {
        runtime = trim(*it);
        log("found runtime: " + runtime);
        break;
      }
    }

    std::regex reRuntime(R"(com.apple.CoreSimulator.SimRuntime.iOS(?:.*))");
    std::smatch matchRuntime;
    std::string runtimeId;

    if (std::regex_search(runtime, matchRuntime, reRuntime)) {
      runtimeId = matchRuntime.str(0);
    }

    std::stringstream createSimulatorCommand;
    createSimulatorCommand
      << "xcrun simctl"
      << " create SocketSDKSimulator_" + iosSimulatorDeviceSuffix
      << " " << deviceType
      << " " << runtimeId;

    auto rCreateSimulator = exec(createSimulatorCommand.str().c_str());
    if (rCreateSimulator.exitCode != 0) {
      log("unable to create simulator VM");
      if (rCreateSimulator.output.size() > 0) {
        log(rCreateSimulator.output);
      }
      exit(rCreateSimulator.exitCode);
    }
    uuid = rCreateSimulator.output;
  }

  if (!booted) {
    log("booting VM " + uuid);
    std::stringstream bootSimulatorCommand;
    bootSimulatorCommand
      << "xcrun"
      << " simctl boot " << uuid;
    auto rBootSimulator = exec(bootSimulatorCommand.str().c_str());
    if (rBootSimulator.exitCode != 0) {
      log("unable to boot simulator VM with command: " + bootSimulatorCommand.str());
      if (rBootSimulator.output.size() > 0) {
        log(rBootSimulator.output);
      }
      exit(rBootSimulator.exitCode);
    }
  }

  log("run simulator");

  auto pathToXCode = fs::path("/Applications") / "Xcode.app" / "Contents";
  auto pathToSimulator = pathToXCode / "Developer" / "Applications" / "Simulator.app";

  std::stringstream simulatorCommand;
  simulatorCommand
    << "open "
    << pathToSimulator.string()
    << " --args -CurrentDeviceUDID " << uuid;

  auto rOpenSimulator = exec(simulatorCommand.str().c_str());
  if (rOpenSimulator.exitCode != 0) {
    log("unable to run simulator");
    if (rOpenSimulator.output.size() > 0) {
      log(rOpenSimulator.output);
    }
    exit(rOpenSimulator.exitCode);
  }

  std::stringstream installAppCommand;

  installAppCommand
    << "xcrun"
    << " simctl install booted "
    << path.string();
  // log(installAppCommand.str());
  log("installed booted VM into simulator");

  auto rInstallApp = exec(installAppCommand.str().c_str());
  if (rInstallApp.exitCode != 0) {
    log("unable to install the app into simulator VM with command: " + installAppCommand.str());
    if (rInstallApp.output.size() > 0) {
      log(rInstallApp.output);
    }
    exit(rInstallApp.exitCode);
  }

  std::stringstream launchAppCommand;
  launchAppCommand
    << "xcrun"
    << " simctl launch --console-pty booted"
    << " " + settings["bundle_identifier"];

  // log(launchAppCommand.str());
  log("launching the app in simulator");

  auto rlaunchApp = exec(launchAppCommand.str().c_str());
  if (rlaunchApp.exitCode != 0) {
    log("unable to launch the app: " + launchAppCommand.str());
    if (rlaunchApp.output.size() > 0) {
      log(rlaunchApp.output);
    }
    exit(rlaunchApp.exitCode);
  }
};

static std::string getCxxFlags() {
  auto flags = getEnv("CXX_FLAGS");
  return flags.size() > 0 ? " " + flags : "";
}

void printHelp (const std::string& command, Map& attrs) {
  if (command == "ssc") {
    std::cout << tmpl(gHelpText, attrs) << std::endl;
  } else if (command == "compile") {
    std::cout << tmpl(gHelpTextCompile, attrs) << std::endl;
  } else if (command == "list-devices") {
    std::cout << tmpl(gHelpTextListDevices, attrs) << std::endl;
  } else if (command == "init") {
    std::cout << tmpl(gHelpTextInit, attrs) << std::endl;
  } else if (command == "install-app") {
    std::cout << tmpl(gHelpTextInstallApp, attrs) << std::endl;
  } else if (command == "print-build-dir") {
    std::cout << tmpl(gHelpTextPrintBuildDir, attrs) << std::endl;
  } else if (command == "run") {
    std::cout << tmpl(gHelpTextRun, attrs) << std::endl;
  }
}

inline std::string getCfgUtilPath() {
  const bool hasCfgUtilInPath = exec("command -v cfgutil").exitCode == 0;
  if (hasCfgUtilInPath) {
    return "cfgutil";
  }
  const bool hasCfgUtil = fs::exists("/Applications/Apple Configurator.app/Contents/MacOS/cfgutil");
  if (hasCfgUtil) {
    log("We highly recommend to install Automation Tools from the Apple Configurator main menu.");
    return "/Applications/Apple\\ Configurator.app/Contents/MacOS/cfgutil";
  }
  log("Please install Apple Configurator from https://apps.apple.com/us/app/apple-configurator/id1037126344");
  exit(1);
}

int main (const int argc, const char* argv[]) {
  Map attrs;
  attrs["ssc_version"] = SSC::full_version;

  if (argc < 2) {
    printHelp("ssc", attrs);
    exit(0);
  }

  auto is = [](const std::string& s, const auto& p) -> bool {
    return s.compare(p) == 0;
  };

  auto optionValue = [](const std::string& s, const auto& p) -> std::string {
    auto string = p + std::string("=");
    if (std::string(s).find(string) == 0) {
      auto value = s.substr(string.size());
      if (value.size() == 0) {
        log("missing value for option " + std::string(p));
        exit(1);
      }
      return value;
    }
    return "";
  };

  auto const subcommand = argv[1];

  if (is(subcommand, "-v") || is(subcommand, "--version")) {
    std::cout << SSC::full_version << std::endl;
    exit(0);
  }

  if (is(subcommand, "-h") || is(subcommand, "--help")) {
    printHelp("ssc", attrs);
    exit(0);
  }

  if (subcommand[0] == '-') {
    log("unknown option: " + std::string(subcommand));
    printHelp("ssc", attrs);
    exit(0);
  }

  auto const lastOption = argv[argc-1];
  fs::path targetPath;
  int numberOfOptions = argc - 3;

  // if no path provided, use current directory
  if (argc == 2 || lastOption[0] == '-') {
    numberOfOptions = argc - 2;
    targetPath = fs::absolute(".");
  } else if (lastOption[0] == '.') {
    targetPath = fs::absolute(lastOption);
  } else {
    targetPath = fs::path(lastOption);
  }

  std::string _settings;
  Map settings = {{}};

  struct Paths {
    fs::path pathBin;
    fs::path pathPackage;
    fs::path pathResourcesRelativeToUserBuild;
    fs::path platformSpecificOutputPath;
  };

  auto getPaths = [&](std::string platform) -> Paths {
    Paths paths;
    std::string platformPath = platform == "win32"
      ? "win"
      : platform;
    paths.platformSpecificOutputPath = {
      targetPath /
      settings["output"] /
      platformPath
    };
    if (platform == "mac") {
      fs::path pathBase = "Contents";
      fs::path packageName = fs::path(settings["name"] + ".app");
      paths.pathPackage = { paths.platformSpecificOutputPath  / packageName };
      paths.pathBin = { paths.pathPackage / pathBase / "MacOS" };
      paths.pathResourcesRelativeToUserBuild = {
        paths.platformSpecificOutputPath  /
        packageName /
        pathBase /
        "Resources"
      };
      return paths;
    } else if (platform == "linux") {
      // this follows the .deb file naming convention
      fs::path packageName = (
        settings["executable"] + "_" +
        settings["version"] + "-" +
        settings["revision"] + "_" +
        "amd64"
      );
      paths.pathPackage = { paths.platformSpecificOutputPath  / packageName };
      paths.pathBin = {
        paths.pathPackage /
        "opt" /
        settings["name"]
      };;
      paths.pathResourcesRelativeToUserBuild = paths.pathBin;
      return paths;
    } else if (platform == "win32") {
      paths.pathPackage = { 
        paths.platformSpecificOutputPath  /
        fs::path(settings["executable"] + "-" +settings["version"])
      };

      paths.pathBin = paths.pathPackage;
      paths.pathResourcesRelativeToUserBuild = paths.pathPackage;
      return paths;
    } else if (platform == "ios" || platform == "ios-simulator") {
      fs::path pathBase = "Contents";
      fs::path packageName = settings["name"] + ".app";
      paths.pathPackage = { paths.platformSpecificOutputPath  / packageName };
      paths.pathBin = { paths.platformSpecificOutputPath  / pathBase / "MacOS" };
      paths.pathResourcesRelativeToUserBuild = paths.platformSpecificOutputPath / "ui";
      return paths;
    } else if (platform == "android" || platform == "android-simulator") {
      auto relativeOutput = paths.platformSpecificOutputPath  / "android";
      auto output = fs::absolute(targetPath) / relativeOutput;
      paths.pathResourcesRelativeToUserBuild = {
        relativeOutput / "app" / "src" / "main" / "assets"
      };
      return paths;
    }
    log("unknown platform: " + platform);
    exit(1);
  };

  auto createSubcommand = [&](
    const std::string& subcommand,
    const std::vector<std::string>& options,
    const bool& needsConfig,
    std::function<void(std::span<const char *>)> subcommandHandler
  ) -> void {
    if (argv[1] == subcommand) {
      if (argc > 2 && (is(argv[2], "-h") || is(argv[2], "--help"))) {
        printHelp(subcommand, attrs);
        exit(0);
      }
      auto commandlineOptions = std::span(argv, argc).subspan(2, numberOfOptions);
      for (auto const& arg : commandlineOptions) {
        auto isAcceptableOption = false;
        for (auto const& option : options) {
          if (is(arg, option) || optionValue(arg, option).size() > 0) {
            isAcceptableOption = true;
            break;
          }
        }
        if (!isAcceptableOption) {
          log("unrecognized option: " + std::string(arg));
          printHelp(subcommand, attrs);
          exit(1);
        }
      }

      if (needsConfig) {
        auto configPath = targetPath / "ssc.config";
        if (!fs::exists(configPath) && !is(subcommand, "init")) {
          log("ssc.config not found in " + targetPath.string());
          exit(1);
        }
        _settings = WStringToString(readFile(configPath));
        settings = parseConfig(_settings);

        settings["output"] = settings["output"].size() > 0 ? settings["output"] : "dist";

        for (auto const arg : std::span(argv, argc).subspan(2, numberOfOptions)) {
          if (is(arg, "--prod")) {
            flagDebugMode = false;
            break;
          }
        }

        std::string suffix = "";

        if (flagDebugMode) {
          suffix += "-dev";
        }

        std::replace(settings["name"].begin(), settings["name"].end(), ' ', '_');
        settings["name"] += suffix;
        settings["title"] += suffix;
        settings["executable"] += suffix;
      }
      subcommandHandler(commandlineOptions);
    }
  };

  createSubcommand("init", {}, false, [&](const std::span<const char *>& options) -> void {
    attrs["node_platform"] = platform.arch == "arm64" ? "arm64" : "x64";
    fs::create_directories(targetPath / "src");
    SSC::writeFile(targetPath / "src" / "index.html", gHelloWorld);
    SSC::writeFile(targetPath / "ssc.config", tmpl(gDefaultConfig, attrs));
    SSC::writeFile(targetPath / ".gitignore", tmpl(gDefaultGitignore, attrs));
    exit(0);
  });

  createSubcommand("list-devices", { "--platform", "--ecid", "--udid", "--only" }, false, [&](const std::span<const char *>& options) -> void {
    std::string targetPlatform = "";
    bool isUdid = false;
    bool isEcid = false;
    bool isOnly = false;
    for (auto const& option : options) {
      auto platform = optionValue(options[0], "--platform");
      if (platform.size() > 0) {
        targetPlatform = platform;
      }
      if (is(option, "--udid")) {
        isUdid = true;
      }
      if (is(option, "--ecid")) {
        isEcid = true;
      }
      if (is(option, "--only")) {
        isOnly = true;
      }
    }
    if (targetPlatform.size() == 0) {
      log("error: --platfrom option is required");
      exit(1);
    }
    if (targetPlatform == "ios" && platform.mac) {
      if (isUdid && isEcid) {
        log("--udid and --ecid are mutually exclusive");
        printHelp("list-devices", attrs);
        exit(1);
      }
      if (isOnly && !isUdid && !isEcid) {
        log("--only requires --udid or --ecid");
        printHelp("list-devices", attrs);
        exit(1);
      }
      std::string cfgUtilPath = getCfgUtilPath();
      std::string command = cfgUtilPath + " list-devices";
      auto r = exec(command);

      if (r.exitCode == 0) {
        std::regex re(R"(ECID:\s(\S*)\s*UDID:\s(\S*).*Name:\s(.*))");
        std::smatch matches;
        std::string uuid;

        while (std::regex_search(r.output, matches, re)) {
          auto ecid = matches[1];
          auto udid = matches[2];
          auto name = matches[3];
          if (isUdid) {
            if (isOnly) {
              std::cout << udid << std::endl;
              break;
            } else {
              std::cout << name << ": " << udid << std::endl;
            }
          } else if (isEcid) {
            if (isOnly) {
              std::cout << ecid << std::endl;
              break;
            } else {
              std::cout << name << ": " << ecid << std::endl;
            }
          } else {
            std::cout << name << std::endl;
            std::cout << "ECID: " << ecid << " UDID: " << udid << std::endl;
            if (isOnly) break;
          }
          r.output = matches.suffix();
        }
        exit(0);
      } else {
        log("Could not list devices using " + command);
        exit(1);
      }
    } else {
      log("list-devices is only supported for iOS devices on macOS.");
      exit(0);
    }
  });

  createSubcommand("install-app", { "--platform", "--device" }, true, [&](const std::span<const char *>& options) -> void {
    if (platform.os != "mac") {
      log("install-app is only supported on macOS.");
      exit(0);
    } else {
      const std::string cfgUtilPath = getCfgUtilPath();
      std::string commandOptions = "";
      std::string targetPlatform = "";
      // we need to find platform first
      for (auto const option : options) {
        targetPlatform = optionValue(option, "--platform");
        if (targetPlatform.size() > 0) {
          // TODO: add Android support
          if (targetPlatform != "ios") {
            std::cout << "Unsupported platform: " << targetPlatform << std::endl;
            exit(1);
          }
        }
      }
      if (targetPlatform.size() == 0) {
        log("--platform option is required.");
        printHelp("install-app", attrs);
        exit(1);
      }
      // then we need to find device
      for (auto const option : options) {
        auto device = optionValue(option, "--device");
        if (device.size() > 0) {
          if (targetPlatform == "ios") {
            commandOptions += " --ecid " + device + " ";
          }
        }
      }

      auto ipaPath = (
        getPaths(targetPlatform).platformSpecificOutputPath /
        "build" /
        std::string(settings["name"] + ".ipa") /
        std::string(settings["name"] + ".ipa")
      );
      if (!fs::exists(ipaPath)) {
        log("Could not find " + ipaPath.string());
        exit(1);
      }
      // this command will install the app to first connected device which were
      // added to the provisioning profile if no --device is provided.
      auto command = cfgUtilPath + " " + commandOptions + "install-app " + ipaPath.string();
      auto r = exec(command);
      if (r.exitCode != 0) {
        log("failed to install the app. Is the device plugged in?");
        exit(1);
      }
      log("successfully installed the app on your device(s).");
      exit(0);
    }
  });

  createSubcommand("print-build-dir", { "--platform", "--prod" }, true, [&](const std::span<const char *>& options) -> void {
    // if --platform is specified, use the build path for the specified platform
    for (auto const option : options) {
      auto targetPlatform = optionValue(option, "--platform");
      if (targetPlatform.size() > 0) {
        fs::path path = getPaths(targetPlatform).pathResourcesRelativeToUserBuild;
        std::cout << path.string() << std::endl;
        exit(0);
      }
    }
    // if no --platform option provided, print current platform build path
    std::cout << getPaths(platform.os).pathResourcesRelativeToUserBuild << std::endl;
    exit(0);
  });

  createSubcommand("compile", { "--platform", "--port", "--quiet", "-o", "-r", "--prod", "-p", "-c", "-s", "-e", "-n", "--test=1", "--headless" }, true, [&](const std::span<const char *>& options) -> void {
    bool flagRunUserBuildOnly = false;
    bool flagAppStore = false;
    bool flagCodeSign = false;
    bool flagHeadless = false;
    bool flagShouldRun = false;
    bool flagEntitlements = false;
    bool flagShouldNotarize = false;
    bool flagShouldPackage = false;
    bool flagBuildForIOS = false;
    bool flagBuildForAndroid = false;
    bool flagBuildForAndroidEmulator = false;
    bool flagBuildForSimulator = false;
    std::string argvForward = "";
    std::string targetPlatform = "";

    std::string devPort("0");
    auto cnt = 0;

    for (auto const arg : options) {
      if (is(arg, "-h") || is(arg, "--help")) {
        printHelp("compile", attrs);
        exit(0);
      }

      if (is(arg, "-c")) {
        flagCodeSign = true;
      }

      if (is(arg, "-e")) {
        if (platform.os != "mac") {
          log("-e is only supported on macOS. Ignoring.");
        } else {
          // TODO: we don't use it
          flagEntitlements = true;
        }
      }

      if (is(arg, "-n")) {
        if (platform.os != "mac") {
          log("-n is only supported on macOS. Ignoring.");
        } else {
          flagShouldNotarize = true;
        }
      }

      if (is(arg, "-o")) {
        flagRunUserBuildOnly = true;
      }

      if (is(arg, "-p")) {
        flagShouldPackage = true;
      }

      if (is(arg, "-r") || is(arg, "--run")) {
        flagShouldRun = true;
      }

      if (is(arg, "-s")) {
        flagAppStore = true;
      }

      if (is(arg, "--test=1") || is(arg, "--test")) {
        argvForward += "--test=1";
      }

      if (is(arg, "--headless")) {
        argvForward += "--headless";
        flagHeadless = true;
      }

      if (is(arg, "--quiet")) {
        flagQuietMode = true;
      }

      targetPlatform = optionValue(arg, "--platform");
      if (targetPlatform.size() > 0) {
        if (targetPlatform == "ios") {
          flagBuildForIOS = true;
        } else if (targetPlatform == "android") {
          flagBuildForAndroid = true;
        } else if (targetPlatform == "android-emulator") {
          flagBuildForAndroid = true;
          flagBuildForAndroidEmulator = true;
        } else if (targetPlatform == "ios-simulator") {
          flagBuildForIOS = true;
          flagBuildForSimulator = true;
        }
      }

      auto port = optionValue(arg, "--port");
      if (port.size() > 0) {
        devPort = port;
      }
    }

    const std::vector<std::string> required = {
      "name",
      "title",
      "executable",
      "version"
    };

    for (const auto &str : required) {
      if (settings.count(str) == 0) {
        log("'" + str + "' value is required in ssc.config");
        exit(1);
      }
    }

    if (settings.count("revision") == 0) {
      settings["revision"] = "1";
    }

    if (getEnv("CXX").size() == 0) {
      log("warning! $CXX env var not set, assuming defaults");

      if (platform.win) {
        setEnv("CXX=clang++");
      } else {
        setEnv("CXX=/usr/bin/g++");
      }
    }

    targetPlatform = targetPlatform.size() > 0 ? targetPlatform : platform.os;
    Paths paths = getPaths(targetPlatform);

    auto executable = fs::path(settings["executable"] + (platform.win ? ".exe" : ""));
    auto binaryPath = paths.pathBin / executable;
    auto configPath = targetPath / "ssc.config";

    if (!fs::exists(binaryPath)) {
      flagRunUserBuildOnly = false;
    } else {
      struct stat stats;
      if (stat(binaryPath.c_str(), &stats) == 0) {
        if (BUILD_TIME > stats.st_mtime) {
          flagRunUserBuildOnly = false;
        }
      }
    }

    if (flagRunUserBuildOnly) {
      struct stat binaryPathStats;
      struct stat configPathStats;

      if (stat(binaryPath.c_str(), &binaryPathStats) == 0) {
        if (stat(configPath.c_str(), &configPathStats) == 0) {
          if (configPathStats.st_mtime > binaryPathStats.st_mtime) {
            flagRunUserBuildOnly = false;
          }
        }
      }
    }

    if (flagRunUserBuildOnly == false && fs::exists(paths.platformSpecificOutputPath)) {
      auto p = fs::current_path() / fs::path(paths.platformSpecificOutputPath);
      try {
        fs::remove_all(p);
        log(std::string("cleaned: " + p.string()));
      } catch (fs::filesystem_error const& ex) {
        log("could not clean path (binary could be busy)");
        log(std::string("ex: ") + ex.what());
        log(std::string("ex code: ") + std::to_string(ex.code().value()));
        exit(1);
      }
    }

    std::string flags;
    std::string files;

    fs::path pathResources;
    fs::path pathToArchive;

    //
    // Darwin Package Prep
    // ---
    //
    if (platform.mac && !flagBuildForIOS && !flagBuildForAndroid) {
      log("preparing build for mac");

      flags = "-std=c++2a -ObjC++";
      flags += " -framework UniformTypeIdentifiers";
      flags += " -framework CoreBluetooth";
      flags += " -framework UserNotifications";
      flags += " -framework WebKit";
      flags += " -framework Cocoa";
      flags += " -DMACOS=1";
      flags += " -Wno-nullability-completeness"; // ignore swift compat
      flags += " -I" + prefixFile();
      flags += " -I" + prefixFile("include");
      flags += " -L" + prefixFile("lib");
      flags += " -luv";
      flags += " " + getCxxFlags();

      files += prefixFile("src/main.cc");
      files += prefixFile("src/process_unix.cc");

      fs::path pathBase = "Contents";
      pathResources = { paths.pathPackage / pathBase / "Resources" };

      fs::create_directories(paths.pathBin);
      fs::create_directories(pathResources);

      auto plistInfo = tmpl(gPListInfo, settings);

      writeFile(paths.pathPackage / pathBase / "Info.plist", plistInfo);

      auto credits = tmpl(gCredits, attrs);

      writeFile(paths.pathResourcesRelativeToUserBuild / "Credits.html", credits);
    }

    if (flagBuildForAndroid) {
      auto bundle_identifier = settings["bundle_identifier"];
      auto bundle_path = fs::path(replace(bundle_identifier, "\\.", "/")).make_preferred();
      auto bundle_path_underscored = replace(bundle_identifier, "\\.", "_");

      auto output = paths.platformSpecificOutputPath;
      auto app = output / "app";
      auto src = app / "src";
      auto jni = src / "main" / "jni";
      auto res = src / "main" / "res";
      auto pkg = src / "main" / "java" / bundle_path;

      if (settings["android_main_activity"].size() == 0) {
        settings["android_main_activity"] = std::string(DEFAULT_ANDROID_ACTIVITY_NAME);
      }

      // clean and create output directories
      //fs::remove_all(output);
      fs::create_directories(output);
      fs::create_directories(src);
      fs::create_directories(jni);
      fs::create_directories(pkg);

      fs::create_directories(res);
      fs::create_directories(res / "layout");
      fs::create_directories(res / "values");

      pathResources = { src / "main" / "assets" };

      // set current path to output directory
      fs::current_path(output);

      std::stringstream gradleInitCommand;
      gradleInitCommand
        << "echo 1 |"
        << "gradle "
        << "--no-configuration-cache "
        << "--no-build-cache "
        << "--no-scan "
        << "--offline "
        << "--quiet "
        << "init";

      if (std::system(gradleInitCommand.str().c_str()) != 0) {
        log("error: failed to invoke `gradle init` command");
        exit(1);
      }

      //writeFile();

      // Core
      fs::copy(trim(prefixFile("src/core.hh")), jni, fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/common.hh")), jni, fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/preload.hh")), jni, fs::copy_options::overwrite_existing);

      // libuv
      fs::copy(
        trim(prefixFile("uv")),
        jni / "uv",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      auto aaptNoCompressOptionsNormalized = std::vector<std::string>();
      auto aaptNoCompressDefaultOptions = split(R"OPTIONS("htm","html","txt","js","jsx","mjs","ts","css","xml")OPTIONS", ',');
      auto aaptNoCompressOptions = split(settings["android_aapt_no_compress"], ',');

      settings["android_aapt_no_compress"] = "";

      for (auto const &option: aaptNoCompressOptions) {
        auto value = trim(option);

        if ('"' != value[0]) {
          value = "\"" + value + "\"";
        }

        if (settings["android_aapt_no_compress"].size() > 0) {
          settings["android_aapt_no_compress"] += ", ";
        }

        settings["android_aapt_no_compress"] = value;
        aaptNoCompressOptionsNormalized.push_back(value);
      }

      for (auto const &option: aaptNoCompressDefaultOptions) {
        auto value = trim(option);


        auto cursor = std::find(
          aaptNoCompressOptionsNormalized.begin(),
          aaptNoCompressOptionsNormalized.end(),
          trim(option)
        );

        if (cursor == aaptNoCompressOptionsNormalized.end()) {
          if (settings["android_aapt_no_compress"].size() > 0) {
            settings["android_aapt_no_compress"] += ", ";
          }

          settings["android_aapt_no_compress"] += value;
        }
      }

      if (settings["android_aapt"].size() == 0) {
        settings["android_aapt"] = "";
      }

      if (settings["android_native_abis"].size() == 0) {
        settings["android_native_abis"] = "";
      }

      if (settings["android_ndk_abi_filters"].size() == 0) {
        settings["android_ndk_abi_filters"] = settings["android_native_abis"];
      }

      if (settings["android_ndk_abi_filters"].size() == 0) {
        settings["android_ndk_abi_filters"] = "'arm64-v8a', 'x86_64'";
      } else {
        auto filters = settings["android_ndk_abi_filters"];
        settings["android_ndk_abi_filters"] = "";

        for (auto const &filter : split(filters, ' ')) {
          auto value = trim(replace(filter, "\"", "'"));

          if (value.size() == 0) {
            continue;
          }

          if (value[0] != '\'') {
            value = "'" + value;
          }

          if (value[value.size() - 1] != '\'') {
            value += "'";
          }

          if (settings["android_ndk_abi_filters"].size() > 0) {
            settings["android_ndk_abi_filters"] += ", ";
          }

          settings["android_ndk_abi_filters"] += value;
        }
      }

      Map manifestContext;

      manifestContext["android_manifest_xml_permissions"] = "";

      if (settings["android_manifest_permissions"].size() > 0) {
        settings["android_manifest_permissions"] = replace(settings["android_manifest_permissions"], ",", " ");
        for (auto const &value: split(settings["android_manifest_permissions"], ' ')) {
          auto permission = replace(trim(value), "\"", "");
          std::stringstream xml;

          std::transform(permission.begin(), permission.end(), permission.begin(), ::toupper);

          xml
            << "<uses-permission android:name="
            << "\"android.permission." << permission << "\""
            << " />";

          manifestContext["android_manifest_xml_permissions"] += xml.str() + "\n";
        }
      }

      // Android Project
      writeFile(
        src / "main" / "AndroidManifest.xml",
        trim(tmpl(tmpl(gAndroidManifest, settings), manifestContext))
      );

      writeFile(app / "proguard-rules.pro", trim(tmpl(gProGuardRules, settings)));
      writeFile(app / "build.gradle", trim(tmpl(gGradleBuildForSource, settings)));

      writeFile(output / "settings.gradle", trim(tmpl(gGradleSettings, settings)));
      writeFile(output / "build.gradle", trim(tmpl(gGradleBuild, settings)));
      writeFile(output / "gradle.properties", trim(tmpl(gGradleProperties, settings)));

      writeFile(res / "layout" / "web_view_activity.xml", trim(tmpl(gAndroidLayoutWebviewActivity, settings)));
      writeFile(res / "values" / "strings.xml", trim(tmpl(gAndroidValuesStrings, settings)));
      writeFile(src / "main" / "assets" / "vital_check_ok.txt", "OK");

      // JNI/NDK
      fs::copy(trim(prefixFile("src/android.cc")), jni, fs::copy_options::overwrite_existing);

      writeFile(
        jni / "android.hh",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android.hh")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_path_underscored
        )
      );

      auto cflags = flagDebugMode
        ? settings.count("debug_flags") ? settings["debug_flags"] : ""
        : settings.count("flags") ? settings["flags"] : "";

      std::stringstream pp;
      pp
        << "-DDEBUG=" << (flagDebugMode ? 1 : 0) << " "
        << "-DANDROID=1" << " "
        << "-DSETTINGS=\"" << encodeURIComponent(_settings) << "\" "
        << "-DVERSION=" << SSC::version << " "
        << "-DVERSION_HASH=" << SSC::version_hash << " ";

      Map makefileContext;

      makefileContext["cflags"] = cflags + " " + pp.str();
      makefileContext["cflags"] += " " + settings["android_native_cflags"];

      if (settings["android_native_abis"].size() > 0) {
        makefileContext["android_native_abis"] = settings["android_native_abis"];
      } else {
        makefileContext["android_native_abis"] = "all";
      }

      // custom native sources
      for (auto const &file : split(settings["android_native_sources"], ' ')) {
        auto filename = fs::path(file).filename();
        // log(std::string("Android NDK source: " + std::string(target / file)).c_str());
        appendFile(
          jni / "android.cc",
          tmpl(std::regex_replace(
            WStringToString(readFile(targetPath / file )),
            std::regex("__BUNDLE_IDENTIFIER__"),
            bundle_identifier
          ), settings)
        );
      }

      if (settings["android_native_makefile"].size() > 0) {
        makefileContext["android_native_make_context"] =
          trim(tmpl(tmpl(WStringToString(readFile(targetPath / settings["android_native_makefile"])), settings), makefileContext));
      } else {
        makefileContext["android_native_make_context"] = "";
      }

      writeFile(
        jni / "Application.mk",
        trim(tmpl(tmpl(gAndroidApplicationMakefile, makefileContext), settings))
      );

      writeFile(
        jni / "Android.mk",
        trim(tmpl(tmpl(gAndroidMakefile, makefileContext), settings))
      );

      // Android Source
      writeFile(
        pkg / "main.kt",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      // custom source files
      for (auto const &file : split(settings["android_sources"], ' ')) {
        // log(std::string("Android source: " + std::string(target / file)).c_str());
        writeFile(
          pkg / fs::path(file).filename(),
          tmpl(std::regex_replace(
            WStringToString(readFile(targetPath / file )),
            std::regex("__BUNDLE_IDENTIFIER__"),
            bundle_identifier
          ), settings)
        );
      }
    }

    if (flagBuildForIOS && !platform.mac) {
      log("Building for iOS on a non-mac platform is unsupported");
      exit(1);
    }

    if (platform.mac && flagBuildForIOS) {
      fs::remove_all(paths.platformSpecificOutputPath);

      auto projectName = (settings["name"] + ".xcodeproj");
      auto schemeName = (settings["name"] + ".xcscheme");
      auto pathToProject = paths.platformSpecificOutputPath / projectName;
      auto pathToScheme = pathToProject / "xcshareddata" / "xcschemes";
      auto pathToProfile = targetPath / settings["ios_provisioning_profile"];

      fs::create_directories(pathToProject);
      fs::create_directories(pathToScheme);

      if (!flagBuildForSimulator) {
        if (!fs::exists(pathToProject)) {
          log("provisioning profile not found: " + pathToProfile.string() + ". " +
              "Please specify a valid provisioning profile in the " +
              "ios_provisioning_profile field in your ssc.config");
          exit(1);
        }
        std::string command = (
          "security cms -D -i " + pathToProfile.string()
        );

        auto r = exec(command);
        std::regex re(R"(<key>UUID<\/key>\n\s*<string>(.*)<\/string>)");
        std::smatch match;
        std::string uuid;

        if (!std::regex_search(r.output, match, re)) {
          log("failed to extract uuid from provisioning profile using \"" + command + "\"");
          exit(1);
        }

        uuid = match.str(1);

        auto pathToInstalledProfile = fs::path(getEnv("HOME")) /
          "Library" /
          "MobileDevice" /
          "Provisioning Profiles" /
          (uuid + ".mobileprovision");

        if (!fs::exists(pathToInstalledProfile)) {
          fs::copy(pathToProfile, pathToInstalledProfile);
        }

        settings["ios_provisioning_profile"] = uuid;
      }

      fs::copy(
        fs::path(prefixFile()) / "lib",
        paths.platformSpecificOutputPath / "lib",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      fs::copy(
        fs::path(prefixFile()) / "include",
        paths.platformSpecificOutputPath / "include",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      writeFile(paths.platformSpecificOutputPath / "exportOptions.plist", tmpl(gXCodeExportOptions, settings));
      writeFile(paths.platformSpecificOutputPath / "Info.plist", tmpl(gXCodePlist, settings));
      writeFile(pathToProject / "project.pbxproj", tmpl(gXCodeProject, settings));
      writeFile(pathToScheme / schemeName, tmpl(gXCodeScheme, settings));

      pathResources = paths.platformSpecificOutputPath / "ui";
      fs::create_directories(pathResources);
    }

    //
    // Linux Package Prep
    // ---
    //
    if (platform.linux && !flagBuildForAndroid && !flagBuildForIOS) {
      log("preparing build for linux");
      flags = " -std=c++2a `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0`";
      flags += " " + getCxxFlags();
      flags += " -I" + prefixFile();
      flags += " -I" + prefixFile("include");
      flags += " -L" + prefixFile("lib");
      flags += " -luv";

      files += prefixFile("src/main.cc");
      files += prefixFile("src/process_unix.cc");

      pathResources = paths.pathBin;

      // @TODO(jwerle): support other Linux based OS
      fs::path pathControlFile = {
        paths.pathPackage /
        "DEBIAN"
      };

      fs::path pathManifestFile = {
        paths.pathPackage /
        "usr" /
        "share" /
        "applications"
      };

      fs::path pathIcons = {
        paths.pathPackage /
        "usr" /
        "share" /
        "icons" /
        "hicolor" /
        "256x256" /
        "apps"
      };

      fs::create_directories(pathIcons);
      fs::create_directories(pathResources);
      fs::create_directories(pathManifestFile);
      fs::create_directories(pathControlFile);

      auto linuxExecPath =
        fs::path("/opt") /
        settings["name"] /
        settings["executable"];

      settings["linux_executable_path"] = linuxExecPath.string();
      settings["linux_icon_path"] = (
        fs::path("/usr") /
        "share" /
        "icons" /
        "hicolor" /
        "256x256" /
        "apps" /
        (settings["executable"] + ".png")
      ).string();

      writeFile(pathManifestFile / (settings["name"] + ".desktop"), tmpl(gDestkopManifest, settings));

      if (settings.count("deb_name") == 0) {
        settings["deb_name"] = settings["name"];
      }

      writeFile(pathControlFile / "control", tmpl(gDebianManifest, settings));

      auto pathToIconSrc = (targetPath / settings["linux_icon"]).string();
      auto pathToIconDest = (pathIcons / (settings["executable"] + ".png")).string();

      if (!fs::exists(pathToIconDest)) {
        fs::copy(pathToIconSrc, pathToIconDest);
      }
    }

    //
    // Windows Package Prep
    // ---
    //
    if (platform.win) {
      log("preparing build for win");
      auto prefix = prefixFile();

      flags = " -std=c++20"
        " -I" + prefix +
        " -I" + prefix + "\\src\\win64"
        " -L" + prefix + "\\src\\win64"
      ;

      files += prefixFile("src\\main.cc");
      files += prefixFile("src\\process_win.cc");

      fs::create_directories(paths.pathPackage);

      auto p = fs::path {
        paths.pathResourcesRelativeToUserBuild /
        "AppxManifest.xml"
      };

      if (settings["version_short"].size() > 0) {
        auto versionShort = settings["version_short"];
        auto winversion = split(versionShort, '-')[0];

        settings["win_version"] = winversion + ".0";
      } else {
        settings["win_version"] = "0.0.0.0";
      }

      settings["exe"] = executable.string();

      writeFile(p, tmpl(gWindowsAppManifest, settings));

      // TODO Copy the files into place
    }

    log("package prepared");

    auto pathResourcesRelativeToUserBuild = paths.pathResourcesRelativeToUserBuild;
    if (settings.count("build") != 0) {
      //
      // cd into the targetPath and run the user's build command,
      // pass it the platform specific directory where they
      // should send their build artifacts.
      //
      std::stringstream buildCommand;
      auto oldCwd = fs::current_path();
      fs::current_path(oldCwd / targetPath);

      {
        char prefix[4096] = {0};
        std::memcpy(
          prefix,
          pathResourcesRelativeToUserBuild.string().c_str(),
          pathResourcesRelativeToUserBuild.string().size()
        );

        // @TODO(jwerle): use `setEnv()` if #148 is closed
        #if _WIN32
          setEnv("PREFIX=prefix");
        #else
          setenv("PREFIX", prefix, 1);
        #endif
      }

      buildCommand
        << settings["build"]
        << " "
        << pathResourcesRelativeToUserBuild.string()
        << " --debug=" << flagDebugMode;

      // log(buildCommand.str());
      auto r = exec(buildCommand.str().c_str());

      if (r.exitCode != 0) {
        log("Unable to run user build command");
        log(r.output);
        exit(r.exitCode);
      }

      log("ran user build command");

      fs::current_path(oldCwd);
    } else {
      fs::path pathInput = settings["input"].size() > 0
        ? targetPath / settings["input"]
        : targetPath / "src";
      fs::copy(
        pathInput,
        pathResourcesRelativeToUserBuild
      );
    }

    if (flagBuildForIOS) {
      log("building for iOS");

      auto oldCwd = fs::current_path();
      auto pathToDist = oldCwd / paths.platformSpecificOutputPath;

      fs::create_directories(pathToDist);
      fs::current_path(pathToDist);

      //
      // Copy and or create the source files we need for the build.
      //
      fs::copy(trim(prefixFile("src/ios.mm")), pathToDist);
      fs::copy(trim(prefixFile("src/apple.mm")), pathToDist);
      fs::copy(trim(prefixFile("src/common.hh")), pathToDist);
      fs::copy(trim(prefixFile("src/core.hh")), pathToDist);
      fs::copy(trim(prefixFile("src/preload.hh")), pathToDist);

      auto pathBase = pathToDist / "Base.lproj";
      fs::create_directories(pathBase);

      writeFile(pathBase / "LaunchScreen.storyboard", gStoryboardLaunchScreen);
      // TODO allow the user to copy their own if they have one
      writeFile(pathToDist / "socket.entitlements", gXcodeEntitlements);

      //
      // For iOS we're going to bail early and let XCode infrastructure handle
      // building, signing, bundling, archiving, noterizing, and uploading.
      //
      std::stringstream archiveCommand;
      std::string destination = flagBuildForSimulator
        ? "platform=iOS Simulator,OS=latest,name=" + settings["ios_simulator_device"]
        : "generic/platform=iOS";
      std::string deviceType;

      if (settings["ios_codesign_identity"].size() == 0) {
        settings["ios_codesign_identity"] = "iPhone Distribution";
      }

      auto sup = std::string("archive");
      auto configuration = std::string(flagDebugMode ? "Debug" : "Release");

      if (!flagShouldPackage) {
        sup = "CONFIGURATION_BUILD_DIR=" + pathToDist.string();
      }

      archiveCommand
        << "xcodebuild"
        << " build " << sup
        << " -configuration " << configuration
        << " -scheme " << settings["name"]
        << " -destination '" << destination << "'";

      if (flagShouldPackage) {
        archiveCommand << " -archivePath build/" << settings["name"];
      }

      if (!flagCodeSign) {
        archiveCommand
          << " CODE_SIGN_IDENTITY=\"\""
          << " CODE_SIGNING_REQUIRED=\"NO\""
          << " CODE_SIGN_ENTITLEMENTS=\"\""
          << " CODE_SIGNING_ALLOWED=\"NO\"";
      }

      log(archiveCommand.str().c_str());
      auto rArchive = exec(archiveCommand.str().c_str());

      if (rArchive.exitCode != 0) {
        log("error: failed to archive project");
        log(rArchive.output);
        fs::current_path(oldCwd);
        exit(1);
      }

      log("created archive");

      if (flagBuildForSimulator && flagShouldRun) {
        std::string app = (settings["name"] + ".app");
        auto pathToApp = paths.platformSpecificOutputPath / app;
        runIOSSimulator(pathToApp, settings);
      }

      if (flagShouldPackage) {
        std::stringstream exportCommand;

        exportCommand
          << "xcodebuild"
          << " -exportArchive "
          << " -archivePath build/" << settings["name"] << ".xcarchive"
          << " -exportPath build/" << settings["name"] << ".ipa"
          << " -exportOptionsPlist " << (pathToDist / "exportOptions.plist").string();

        log(exportCommand.str());
        auto rExport = exec(exportCommand.str().c_str());

        if (rExport.exitCode != 0) {
          log("error: failed to export project");
          fs::current_path(oldCwd);
          exit(1);
        }

        log("exported archive");
      }

      log("completed");
      fs::current_path(oldCwd);
      exit(0);
    }

    if (flagBuildForAndroid) {
      auto app = paths.platformSpecificOutputPath / "app";
      auto androidHome = getEnv("ANDROID_HOME");

      if (androidHome.size() == 0) {
        if (platform.os == "linux") {
          androidHome = getEnv("HOME") + "/android";
        } else if (platform.os == "mac") {
          androidHome = getEnv("HOME") + "/Library/Android/sdk";
        } else if (platform.os == "win32") {
          // TODO
        }
      }

      #ifdef _WIN32
        setEnv((std::string("ANDROID_HOME=") + androidHome).c_str());
      #else
        setenv("ANDROID_HOME", androidHome.c_str(), 1);
      #endif

      std::stringstream sdkmanager;
      std::stringstream packages;
      std::stringstream gradlew;

      packages
        << "ndk-bundle "
        << "'system-images;android-32;google_apis;x86_64' "
        << "'system-images;android-32;google_apis;arm64-v8a' ";

      sdkmanager
        << androidHome << "/cmdline-tools/latest/bin/"
        << "sdkmanager "
        << packages.str();

      if (std::system(sdkmanager.str().c_str()) != 0) {
        log("error: failed to initialize Android SDK (sdkmanager)");
        exit(1);
      }

      if (flagDebugMode) {
        gradlew
          << "./gradlew :app:bundleDebug "
          << "--warning-mode all ";

        if (std::system(gradlew.str().c_str()) != 0) {
          log("error: failed to invoke `gradlew :app:bundleDebug` command");
          exit(1);
        }
      } else {
        gradlew
          << "./gradlew :app:bundle";

        if (std::system(gradlew.str().c_str()) != 0) {
          log("error: failed to invoke `gradlew :app:bundle` command");
          exit(1);
        }
      }

      // clear stream
      gradlew.str("");
      gradlew << "./gradlew assemble";

      if (std::system(gradlew.str().c_str()) != 0) {
        log("error: failed to invoke `gradlew assemble` command");
        exit(1);
      }

      if (flagBuildForAndroidEmulator) {
        std::stringstream avdmanager;
        std::string package = "'system-images;android-32;google_apis;x86_64' ";

        avdmanager
          << androidHome << "/cmdline-tools/latest/bin/"
          << "avdmanager create avd "
          << "--device 3 "
          << "--force "
          << "--name SSCAVD "
          << "--abi google_apis/x86_64 "
          << "--package " << package;

        if (std::system(avdmanager.str().c_str()) != 0) {
          log("error: failed to Android Virtual Device (avdmanager)");
          exit(1);
        }
      }

      if (flagShouldRun) {
        // the emulator must be running on device SSCAVD for now
        std::stringstream adb;

        adb
          << androidHome << "/platform-tools/"
          << "adb "
          << "install ";

        if (flagDebugMode) {
          adb << (app / "build" / "outputs" / "apk" / "dev" / "debug" / "app-dev-debug.apk").string();
        } else {
          adb << (app / "build" / "outputs" / "apk" / "dev" / "live" / "app-dev-release-unsigned.apk").string();
        }

        if (std::system(adb.str().c_str()) != 0) {
          log("error: failed to install APK to Android Emulator (adb)");
          exit(1);
        }
      }

      exit(0);
    }

    if (flagRunUserBuildOnly == false) {
      std::stringstream compileCommand;

      auto extraFlags = flagDebugMode
        ? settings.count("debug_flags") ? settings["debug_flags"] : ""
        : settings.count("flags") ? settings["flags"] : "";

      compileCommand
        << getEnv("CXX")
        << " " << files
        << " " << flags
        << " " << extraFlags
        << " -o " << binaryPath.string()
        << " -DIOS=" << (flagBuildForIOS ? 1 : 0)
        << " -DANDROID=" << (flagBuildForAndroid ? 1 : 0)
        << " -DDEBUG=" << (flagDebugMode ? 1 : 0)
        << " -DPORT=" << devPort
        << " -DSETTINGS=\"" << encodeURIComponent(_settings) << "\""
        << " -DVERSION=" << SSC::version
        << " -DVERSION_HASH=" << SSC::version_hash
      ;

      // log(compileCommand.str());

      auto r = exec(compileCommand.str());

      if (r.exitCode != 0) {
        log("Unable to build");
        log(r.output);
        exit(r.exitCode);
      }

      log("compiled native binary");
    }

    //
    // Linux Packaging
    // ---
    //
    if (flagShouldPackage && platform.linux) {
      fs::path pathSymLinks = {
        paths.pathPackage /
        "usr" /
        "local" /
        "bin"
      };

      auto linuxExecPath = fs::path {
        fs::path("/opt") /
        settings["name"] /
        settings["executable"]
      };

      fs::create_directories(pathSymLinks);
      fs::create_symlink(
        linuxExecPath,
        pathSymLinks / settings["executable"]
      );

      std::stringstream archiveCommand;

      archiveCommand
        << "dpkg-deb --build --root-owner-group "
        << paths.pathPackage.string()
        << " "
        << (paths.platformSpecificOutputPath).string();

      auto r = std::system(archiveCommand.str().c_str());

      if (r != 0) {
        log("error: failed to create deb package");
        exit(1);
      }
    }

    //
    // MacOS Stripping
    //
    if (platform.mac) {
      std::stringstream stripCommand;

      stripCommand
        << "strip"
        << " " << binaryPath
        << " -u";

      auto r = exec(stripCommand.str().c_str());
      if (r.exitCode != 0) {
        log(r.output);
        exit(r.exitCode);
      }
    }

    //
    // MacOS Code Signing
    // ---
    //
    if (flagCodeSign && platform.mac) {
      //
      // https://www.digicert.com/kb/code-signing/mac-os-codesign-tool.htm
      // https://developer.apple.com/forums/thread/128166
      // https://wiki.lazarus.freepascal.org/Code_Signing_for_macOS
      //
      std::stringstream signCommand;
      std::string entitlements = "";

      if (settings.count("entitlements") == 1) {
        // entitlements = " --entitlements " + (targetPath / settings["entitlements"]).string();
      }

      if (settings.count("mac_sign") == 0) {
        log("'mac_sign' key/value is required");
        exit(1);
      }

      std::stringstream commonFlags;

      commonFlags
        << " --force"
        << " --options runtime"
        << " --timestamp"
        << entitlements
        << " --sign '" + settings["mac_sign"] + "'"
        << " "
      ;

      if (settings["mac_sign_paths"].size() > 0) {
        auto paths = split(settings["mac_sign_paths"], ';');

        for (int i = 0; i < paths.size(); i++) {
          std::string prefix = (i > 0) ? ";" : "";

          signCommand
            << prefix
            << "codesign "
            << commonFlags.str()
            << (pathResources / paths[i]).string()
          ;
        }
        signCommand << ";";
      }

      signCommand
        << "codesign"
        << commonFlags.str()
        << binaryPath.string()

        << "; codesign"
        << commonFlags.str()
        << paths.pathPackage.string();

      log(signCommand.str());
      auto r = exec(signCommand.str());

      if (r.output.size() > 0) {
        if (r.exitCode != 0) {
          log("Unable to sign");
          std::cerr << r.output << std::endl;
          exit(r.exitCode);
        } else {
          std::cout << r.output << std::endl;
        }
      }

      log("finished code signing");
    }

    //
    // MacOS Packaging
    // ---
    //
    if (flagShouldPackage && platform.mac) {
      std::stringstream zipCommand;
      auto ext = ".zip";
      auto pathToBuild = paths.platformSpecificOutputPath / "build";

      pathToArchive = paths.platformSpecificOutputPath / (settings["executable"] + ext);

      zipCommand
        << "ditto"
        << " -c"
        << " -k"
        << " --sequesterRsrc"
        << " --keepParent"
        << " "
        << paths.pathPackage.string()
        << " "
        << pathToArchive.string();

      auto r = std::system(zipCommand.str().c_str());

      if (r != 0) {
        log("error: failed to create zip for notarization");
        exit(1);
      }

      log("created package artifact");
    }

    //
    // MacOS Notorization
    // ---
    //
    if (flagShouldNotarize && platform.mac) {
      std::stringstream notarizeCommand;
      std::string username = getEnv("APPLE_ID");
      std::string password = getEnv("APPLE_ID_PASSWORD");

      notarizeCommand
        << "xcrun"
        << " altool"
        << " --notarize-app"
        << " --username \"" << username << "\""
        << " --password \"" << password << "\""
        << " --primary-bundle-id \"" << settings["bundle_identifier"] << "\""
        << " --file \"" << pathToArchive.string() << "\""
      ;

      auto r = exec(notarizeCommand.str().c_str());

      if (r.exitCode != 0) {
        log("Unable to notarize");
        exit(r.exitCode);
      }

      std::regex re(R"(\nRequestUUID = (.+?)\n)");
      std::smatch match;
      std::string uuid;

      if (std::regex_search(r.output, match, re)) {
        uuid = match.str(1);
      }

      int requests = 0;

      log("polling for notarization");

      while (!uuid.empty()) {
        if (++requests > 1024) {
          log("apple did not respond to the request for notarization");
          exit(1);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1024 * 6));
        std::stringstream notarizeStatusCommand;

        notarizeStatusCommand
          << "xcrun"
          << " altool"
          << " --notarization-info " << uuid
          << " -u " << username
          << " -p " << password;

        auto r = exec(notarizeStatusCommand.str().c_str());

        std::regex re(R"(\n *Status: (.+?)\n)");
        std::smatch match;
        std::string status;

        if (std::regex_search(r.output, match, re)) {
          status = match.str(1);
        }

        if (status.find("in progress") != -1) {
          log("Checking for updates from apple");
          continue;
        }

        auto lastStatus = r.output;

        if (status.find("invalid") != -1) {
          log("apple rejected the request for notarization");

          log(lastStatus);

          std::stringstream notarizeHistoryCommand;

          notarizeHistoryCommand
            << "xcrun"
            << " altool"
            << " --notarization-history 0"
            << " -u " << username
            << " -p " << password;

          auto r = exec(notarizeHistoryCommand.str().c_str());

          if (r.exitCode != 0) {
            log("Unable to get notarization history");
            exit(r.exitCode);
          }

          log(r.output);

          exit(1);
        }

        if (status.find("success") != -1) {
          log("successfully notarized");
          break;
        }

        if (status.find("success") == -1) {
          log("apple was unable to notarize");
          break;
        }
      }

      log("finished notarization");
    }

    //
    // Windows Packaging
    // ---
    //
    if (flagShouldPackage && platform.win) {
      #ifdef _WIN32

      auto GetPackageWriter = [&](_In_ LPCWSTR outputFileName, _Outptr_ IAppxPackageWriter** writer) {
        HRESULT hr = S_OK;
        IStream* outputStream = NULL;
        IUri* hashMethod = NULL;
        APPX_PACKAGE_SETTINGS packageSettings = {0};
        IAppxFactory* appxFactory = NULL;

        hr = SHCreateStreamOnFileEx(
          outputFileName,
          STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE,
          0, // default file attributes
          TRUE, // create file if it does not exist
          NULL, // no template
          &outputStream
        );

        if (SUCCEEDED(hr)) {
          hr = CreateUri(
            L"http://www.w3.org/2001/04/xmlenc#sha256",
            Uri_CREATE_CANONICALIZE,
            0, // reserved parameter
            &hashMethod
          );
        }

        if (SUCCEEDED(hr)) {
            packageSettings.forceZip32 = TRUE;
            packageSettings.hashMethod = hashMethod;
        }

        // Create a new Appx factory
        if (SUCCEEDED(hr)) {
          hr = CoCreateInstance(
            __uuidof(AppxFactory),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(IAppxFactory),
            (LPVOID*)(&appxFactory)
          );
        }

        // Create a new package writer using the factory
        if (SUCCEEDED(hr)) {
          hr = appxFactory->CreatePackageWriter(
            outputStream,
            &packageSettings,
            writer
          );
        }

        // Clean up allocated resources
        if (appxFactory != NULL) {
          appxFactory->Release();
          appxFactory = NULL;
        }

        if (hashMethod != NULL) {
          hashMethod->Release();
          hashMethod = NULL;
        }

        if (outputStream != NULL) {
          outputStream->Release();
          outputStream = NULL;
        }

        return hr;
      };

      std::wstring appx(StringToWString(paths.pathPackage.string()) + L".appx");

      HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

      if (SUCCEEDED(hr)) {
        IAppxPackageWriter* packageWriter = NULL;

        hr = GetPackageWriter(appx.c_str(), &packageWriter);

        std::function<void(LPCWSTR, fs::path)> addFiles = [&](auto basePath, auto last) {
          for (const auto & entry : fs::directory_iterator(basePath)) {
            auto p = entry.path().filename().string();

            LPWSTR mime = 0;
            FindMimeFromData(NULL, entry.path().c_str(), NULL, 0, NULL, 0, &mime, 0);

            if (p.find("AppxManifest.xml") == 0) {
              continue;
            }

            if (fs::is_directory(entry.path())) {
              addFiles(entry.path().c_str(), fs::path { last / entry.path().filename() });
              continue;
            }

            auto composite = (fs::path { last / entry.path().filename() });

            if (!mime) {
              mime = (LPWSTR) L"application/octet-stream";
            }

            IStream* fileStream = NULL;
            hr = SHCreateStreamOnFileEx(
              entry.path().c_str(),
              STGM_READ | STGM_SHARE_EXCLUSIVE,
              0,
              FALSE,
              NULL,
              &fileStream
            );

            if (SUCCEEDED(hr)) {
              auto hr2 = packageWriter->AddPayloadFile(
                composite.c_str(),
                mime,
                APPX_COMPRESSION_OPTION_NONE,
                fileStream
              );

              if (SUCCEEDED(hr2)) {
              } else {
                log("mimetype?: " + WStringToString(mime));
                log("Could not add payload file: " + entry.path().string());
              }
            } else {
              log("Could not add file: " + entry.path().string());
            }

            if (fileStream != NULL) {
              fileStream->Release();
              fileStream = NULL;
            }
          }
        };

        addFiles(paths.pathPackage.c_str(), fs::path {});

        IStream* manifestStream = NULL;

        if (SUCCEEDED(hr)) {
          auto p = fs::path({ paths.pathPackage / "AppxManifest.xml" });

          hr = SHCreateStreamOnFileEx(
            p.c_str(),
            STGM_READ | STGM_SHARE_EXCLUSIVE,
            0,
            FALSE,
            NULL,
            &manifestStream
          );
        } else {
          log("Could not get package writer or add files");
        }

        if (SUCCEEDED(hr)) {
          hr = packageWriter->Close(manifestStream);
        } else {
          log("Could not generate AppxManifest.xml");
          // log("Run MakeAppx.exe manually to see more errors");
        }

        if (manifestStream != NULL) {
          manifestStream->Release();
          manifestStream = NULL;
        }
        if (packageWriter != NULL) {
          packageWriter->Release();
          packageWriter = NULL;
        }

        CoUninitialize();
      } else {
        log("Unable to initialize package writer");
      }

      if (SUCCEEDED(hr)) {
        log("Package saved");
      }
      else {
        _com_error err(hr);
        std::string msg = std::string( err.ErrorMessage() );
        log("Unable to save package; " + msg);

        exit(1);
      }

      #endif
    }


    //
    // Windows Code Signing
    //
    if (flagCodeSign && platform.win) {
      //
      // https://www.digicert.com/kb/code-signing/signcode-signtool-command-line.htm
      //
      auto sdkRoot = fs::path("C:\\Program Files (x86)\\Windows Kits\\10\\bin");
      auto pathToSignTool = getEnv("SIGNTOOL");

      if (pathToSignTool.size() == 0) {
        // TODO assumes last dir that contains dot. posix doesnt guarantee
        // order, maybe windows does, but this should probably be smarter.
        for (const auto& entry : fs::directory_iterator(sdkRoot)) {
          auto p = entry.path().string();
          if (p.find(".") != -1) pathToSignTool = p;
        }

        pathToSignTool = fs::path {
          fs::path(pathToSignTool) /
          "x64" /
          "signtool.exe"
        }.string();
      }

      if (pathToSignTool.size() == 0) {
        log("WARNING! Can't find windows 10 SDK, assuming signtool.exe is in the path.");
        pathToSignTool = "signtool.exe";
      }

      std::stringstream signCommand;
      std::string password = getEnv("CSC_KEY_PASSWORD");

      if (password.size() == 0) {
        log("ERROR! Environment variable 'CSC_KEY_PASSWORD' is empty!");
        exit(1);
      }

      signCommand
        << "\""
        << "\"" << pathToSignTool << "\""
        << " sign"
        << " /debug"
        << " /f " << settings["win_pfx"]
        << " /p \"" << password << "\""
        << " /v"
        << " /tr http://timestamp.digicert.com"
        << " /td sha256"
        << " /fd sha256"
        << " " << paths.pathPackage.string() << ".appx"
        << "\"";

      log(signCommand.str());
      auto r = exec(signCommand.str().c_str());

      if (r.exitCode != 0) {
        log("Unable to sign");
        log(paths.pathPackage.string());
        log(r.output);
        exit(r.exitCode);
      }
    }

    int exitCode = 0;
    if (flagShouldRun) {
      exitCode = runApp(binaryPath, argvForward, flagHeadless);
    }

    exit(exitCode);
  });

  createSubcommand("run", { "--platform", "--prod", "--test=1", "--headless" }, true, [&](const std::span<const char *>& options) -> void {
    std::string argvForward = "";
    bool isIosSimulator = false;
    bool flagHeadless = false;
    Paths paths = getPaths(platform.os);
    for (auto const& option : options) {
      auto targetPlatform = optionValue(option, "--platform");
      if (targetPlatform.size() != 0) {
        if (targetPlatform == "ios-simulator") {
          isIosSimulator = true;
        } else {
          log("Unknown platform: " + targetPlatform);
          exit(1);
        }
      }

      if (is(option, "--test=1") || is(option, "--test")) {
        argvForward += "--test=1";
      }

      if (is(option, "--headless")) {
        argvForward += "--headless";
        flagHeadless = true;
      }
    }

    if (isIosSimulator) {
      std::string app = (settings["name"] + ".app");
      auto pathToApp = paths.platformSpecificOutputPath / app;
      runIOSSimulator(pathToApp, settings);
    } else {
      auto executable = fs::path(settings["executable"] + (platform.win ? ".exe" : ""));
      auto exitCode = runApp(paths.pathBin / executable, argvForward, flagHeadless);
      exit(exitCode);
    }
  });

  log("subcommand 'ssc " + std::string(subcommand) + "' is not supported.");
  printHelp("ssc", attrs);
  exit(1);
}
