#include "../common.hh"
#include "../process/process.hh"
#include "templates.hh"
#include "../core/core.hh"

#include <filesystem>

#ifdef __linux__
#include <cstring>
#endif

#if defined(__APPLE__)
#include <Foundation/Foundation.h>
#include <Cocoa/Cocoa.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <array>
#include <AppxPackaging.h>
#include <comdef.h>
#include <functional>
#include <shlwapi.h>
#include <strsafe.h>
#include <tchar.h>
#include <wrl.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "msvcrt.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "uv_a.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "ws2_32.lib")

#else
#include <unistd.h>
#endif

#ifndef CMD_RUNNER
#define CMD_RUNNER
#endif

#ifndef SSC_BUILD_TIME
#define SSC_BUILD_TIME 0
#endif

using namespace SSC;
using namespace std::chrono;

String _settings;
Map settings;
Map rc;

auto start = system_clock::now();

bool flagDebugMode = true;
bool flagQuietMode = false;
Map defaultTemplateAttrs;

#if defined(__APPLE__)
std::atomic<bool> checkLogStore = true;
static dispatch_queue_t queue = dispatch_queue_create(
  "socket.runtime.cli.queue",
  dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_CONCURRENT,
    QOS_CLASS_USER_INITIATED,
    -1
  )
);
#endif

const Map SSC::getUserConfig () {
  return settings;
}

bool SSC::isDebugEnabled () {
  return DEBUG == 1;
}

void log (const String s) {
  if (flagQuietMode) return;
  if (s.size() == 0) return;

  #ifdef _WIN32 // unicode console support
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stderr, nullptr, _IOFBF, 1000);
  #endif

  auto now = system_clock::now();
  auto delta = duration_cast<milliseconds>(now - start).count();
  #ifdef _WIN32
  std::cerr << "• " << s << " " << delta << "ms" << std::endl;
  #else
  std::cerr << "• " << s << " \033[0;32m+" << delta << "ms\033[0m" << std::endl;
  #endif
  start = std::chrono::system_clock::now();
}

String getSocketHome (bool verbose) {
  static String XDG_DATA_HOME = getEnv("XDG_DATA_HOME");
  static String LOCALAPPDATA = getEnv("LOCALAPPDATA");
  static String SOCKET_HOME = getEnv("SOCKET_HOME");
  static String HOME = getEnv("HOME");
  static const bool SSC_DEBUG = (
    getEnv("SSC_DEBUG").size() > 0 ||
    getEnv("DEBUG").size() > 0
  );

  static String socketHome = "";
  static String sep = platform.win ? "\\" : "/";

  if (socketHome.size() == 0) {
    if (SOCKET_HOME.size() > 0) {
      if (SOCKET_HOME.back() != sep[0]) {
        socketHome = SOCKET_HOME + sep;
      } else {
        socketHome = SOCKET_HOME;
      }
    } else if (platform.mac || platform.linux) {
      if (XDG_DATA_HOME.size() == 0) {
        socketHome = HOME + "/.local/share/socket/";
      } else {
        if (XDG_DATA_HOME.back() != sep[0]) {
          socketHome = XDG_DATA_HOME + "/socket/";
        } else {
          socketHome = XDG_DATA_HOME + "socket/";
        }
      }
    } else if (platform.win) {
      socketHome = LOCALAPPDATA + "\\Programs\\socketsupply\\";
    }
  }

  if (socketHome.size() > 0) {
    #ifdef _WIN32
    setEnv((String("SOCKET_HOME=") + socketHome).c_str());
    #else
    setenv("SOCKET_HOME", socketHome.c_str(), 1);
    #endif

    if (SSC_DEBUG) {
      log("WARNING: 'SOCKET_HOME' is set to '" + socketHome + "'");
    }
  }

  return socketHome;
}

String getSocketHome () {
  return getSocketHome(true);
}

String getAndroidHome() {
  static auto androidHome = getEnv("ANDROID_HOME");
  if (androidHome.size() > 0) {
    return androidHome;
  }

  if (!platform.win) {
    auto cmd = String(
      "dirname $(dirname $(readlink $(which sdkmanager 2>/dev/null) 2>/dev/null) 2>/dev/null) 2>/dev/null"
    );

    auto r = exec(cmd);

    if (r.exitCode == 0) {
      androidHome = trim(r.output);
    }
  }

  if (androidHome.size() == 0) {
    if (platform.mac) {
      androidHome = getEnv("HOME") + "/Library/Android/sdk";
    } else if (platform.unix) {
      androidHome = getEnv("HOME") + "/android";
    } else if (platform.win) {
      // TODO
    }
  }

  if (androidHome.size() > 0) {
    #ifdef _WIN32
    setEnv((String("ANDROID_HOME=") + androidHome).c_str());
    #else
    setenv("ANDROID_HOME", androidHome.c_str(), 1);
    #endif

    static const bool SSC_DEBUG = (
      getEnv("SSC_DEBUG").size() > 0 ||
      getEnv("DEBUG").size() > 0
    );

    if (SSC_DEBUG) {
      log("WARNING: 'ANDROID_HOME' is set to '" + androidHome + "'");
    }
  }

  return androidHome;
}

inline String prefixFile (String s) {
  static String socketHome = getSocketHome();
  return socketHome + s + " ";
}

inline String prefixFile () {
  static String socketHome = getSocketHome();
  return socketHome;
}

static Process::id_type appPid = 0;
static Process* appProcess = nullptr;
static std::atomic<int> appStatus = -1;
static std::mutex appMutex;

void signalHandler (int signal) {
#if defined(__APPLE__)
  if (signal == SIGUSR1) {
    checkLogStore = true;
    return;
  }
#endif

  if (appProcess != nullptr) {
    auto pid = appProcess->getPID();
    appProcess->kill(pid);
  } else if (appPid > 0) {
  #if !defined(_WIN32)
    kill(appPid, signal);
  #endif
  }

  appProcess = nullptr;
  appStatus = signal;
  appPid = 0;

  appMutex.unlock();
}

int runApp (const Path& path, const String& args, bool headless) {
  auto cmd = path.string();

  if (!fs::exists(path)) {
    log("executable not found: " + cmd);
    return 1;
  }

  auto runner = trim(String(STR_VALUE(CMD_RUNNER)));
  auto prefix = runner.size() > 0 ? runner + String(" ") : runner;
  String headlessCommand = "";

  if (headless) {
    auto headlessRunner = settings["headless_runner"];
    auto headlessRunnerFlags = settings["headless_runner_flags"];

    if (headlessRunner.size() == 0) {
      headlessRunner = settings["headless_" + platform.os + "_runner"];
    }

    if (headlessRunnerFlags.size() == 0) {
      headlessRunnerFlags = settings["headless_" + platform.os + "_runner_flags"];
    }

    if (platform.linux) {
      // use xvfb for linux as a default
      if (headlessRunner.size() == 0) {
        headlessRunner = "xvfb-run";
        int status = std::system((headlessRunner + " --help >/dev/null").c_str());
        if (WEXITSTATUS(status) != 0) {
          headlessRunner = "";
        }
      }

      if (headlessRunnerFlags.size() == 0) {
        // use sane defaults if 'xvfb-run' is used
        if (headlessRunner == "xvfb-run") {
          headlessRunnerFlags = " --server-args='-screen 0 1920x1080x24' ";
        }
      }
    }

    if (headlessRunner != "false") {
      headlessCommand = headlessRunner + headlessRunnerFlags;
    }
  }

#if defined(__APPLE__)
  if (platform.mac) {
    auto sharedWorkspace = [NSWorkspace sharedWorkspace];
    auto configuration = [NSWorkspaceOpenConfiguration configuration];
    auto string = path.string();
    auto slice = string.substr(0, string.find(".app") + 4);
    auto url = [NSURL
      fileURLWithPath: [NSString stringWithUTF8String: slice.c_str()]
    ];

    auto bundle = [NSBundle bundleWithURL: url];
    auto env = [[NSMutableDictionary alloc] init];

    env[@"SSC_CLI_PID"] = [NSString stringWithFormat: @"%d", getpid()];

    for (auto const &envKey : parseStringList(settings["build_env"])) {
      auto cleanKey = trim(envKey);
      auto envValue = getEnv(cleanKey.c_str());
      auto key = [NSString stringWithUTF8String: cleanKey.c_str()];
      auto value = [NSString stringWithUTF8String: envValue.c_str()];

      env[key] = value;
    }

    auto splitArgs = split(args, ' ');
    auto arguments = [[NSMutableArray alloc] init];

    for (auto arg : splitArgs) {
      [arguments addObject: [NSString stringWithUTF8String: arg.c_str()]];
    }

    [arguments addObject: @"--from-ssc"];

    configuration.createsNewApplicationInstance = YES;
    configuration.promptsUserIfNeeded = YES;
    configuration.environment = env;
    configuration.arguments = arguments;
    configuration.activates = headless ? NO : YES;

    log(String("Running App: " + String(bundle.bundlePath.UTF8String)));

    appMutex.lock();

    [sharedWorkspace
      openApplicationAtURL: bundle.bundleURL
             configuration: configuration
         completionHandler: ^(NSRunningApplication* app, NSError* error)
    {
      if (error) {
        appMutex.unlock();
        appStatus = 1;
        debug(
          "ERROR: NSWorkspace: (code=%lu, domain=%@) %@",
          error.code,
          error.domain,
          error.localizedDescription
        );
        return;
      }

      appPid = app.processIdentifier;

      // It appears there is a bug with `:predicateWithFormat:` as the
      // following does not appear to work:
      //
      // [NSPredicate
      //   predicateWithFormat: @"processIdentifier == %d AND subsystem == '%s'",
      //   app.processIdentifier,
      //   bundle.bundleIdentifier // or even a literal string "co.socketsupply.socket.tests"
      // ];
      //
      // We can build the predicate query string manually, instead.
      auto queryStream = StringStream {};
      auto pid = std::to_string(app.processIdentifier);
      auto bid = bundle.bundleIdentifier.UTF8String;
      queryStream
        << "("
        << "  category == 'socket.runtime.desktop' OR "
        << "  category == 'socket.runtime.debug'"
        << ") AND "
        << "processIdentifier == " << pid << " AND "
        << "subsystem == '" << bid << "'";
      // log store query and predicate for filtering logs based on the currently
      // running application that was just launched and those of a subsystem
      // directly related to the application's bundle identifier which allows us
      // to just get logs that came from the application (not foundation/cocoa/webkit)
      const auto query = [NSString stringWithUTF8String: queryStream.str().c_str()];
      const auto predicate = [NSPredicate predicateWithFormat: query];

      // use the launch date as the initial marker
      const auto now = app.launchDate;
      // and offset it by 1 second in the past as the initial position in the eumeration
      auto offset = [now dateByAddingTimeInterval: -1];

      // tracks the latest log entry date so we ignore older ones
      NSDate* latest = nil;

      int pollsAfterTermination = 4;
      int backoffIndex = 0;

      // lucas series of backoffs
      int backoffs[] = {
        16*2,
        16*1,
        16*3,
        16*4,
        16*7,
        16*11,
        16*18,
        16*29,
        32*2,
        32*1,
        32*3,
        32*4,
        32*7,
        32*11,
        32*18,
        32*29,
        64*2,
        64*1,
        64*3,
        64*4,
        64*7,
        64*11,
        64*18,
        64*29,
      };

      dispatch_async(queue, ^{
        while (kill(app.processIdentifier, 0) == 0) {
          msleep(256);
        }
      });

      while (appStatus < 0 || pollsAfterTermination > 0) {
        if (appStatus >= 0) {
          pollsAfterTermination = pollsAfterTermination - 1;
          checkLogStore = true;
        }

        if (!checkLogStore) {
          auto backoff = backoffs[backoffIndex];
          backoffIndex = (
            (backoffIndex + 1) %
            (sizeof(backoffs) / sizeof(backoffs[0]))
          );

          msleep(backoff);
          continue;
        }

        // this is set to `true` from a `SIGUSR1` signal
        checkLogStore = false;
        @autoreleasepool {
          // We need a new `OSLogStore` in each so we can keep enumeratoring
          // the logs until the application terminates. This is required because
          // each `OSLogStore` instance is a snapshot of the system's universal
          // log archive at the time of instantiation.
          auto logs = [OSLogStore
            storeWithScope: OSLogStoreSystem
                     error: &error
          ];

          if (error) {
            appStatus = 1;
            debug(
              "ERROR: OSLogStore: (code=%lu, domain=%@) %@",
              error.code,
              error.domain,
              error.localizedDescription
            );
            break;
          }

          auto position = [logs positionWithDate: offset];
          auto enumerator = [logs
            entriesEnumeratorWithOptions: 0
                                position: position
                               predicate: predicate
                                   error: &error
          ];

          if (error) {
            appStatus = 1;
            debug(
              "ERROR: OSLogEnumerator: (code=%lu, domain=%@) %@",
              error.code,
              error.domain,
              error.localizedDescription
            );
            break;
          }

          // Enumerate all the logs in this loop and print unredacted and most
          // recently log entries to stdout
          for (OSLogEntryLog* entry in enumerator) {
            if (
              entry.composedMessage &&
              entry.processIdentifier == app.processIdentifier
            ) {
              // visit latest log
              if (!latest || [latest compare: entry.date] == NSOrderedAscending) {
                auto message = entry.composedMessage.UTF8String;

                // the OSLogStore may redact log messages the user does not
                // have access to, filter them out
                if (String(message) != "<private>") {
                  if (String(message).starts_with("__EXIT_SIGNAL__")) {
                    appStatus = std::stoi(replace(String(message), "__EXIT_SIGNAL__=", ""));
                  } else if (
                    entry.level == OSLogEntryLogLevelDebug ||
                    entry.level == OSLogEntryLogLevelError ||
                    entry.level == OSLogEntryLogLevelFault
                  ) {
                    std::cerr << message << std::endl;
                  } else {
                    std::cout << message << std::endl;
                  }
                }

                backoffIndex = 0;
                latest = entry.date;
                offset = latest;
              }
            }
          }
        }
      }

      appMutex.unlock();
    }];

    // wait for `NSRunningApplication` to terminate
    std::lock_guard<std::mutex> lock(appMutex);
    log("App result: " + std::to_string(appStatus.load()));
    return appStatus.load();
  }
#endif

  log(String("Running App: " + headlessCommand + prefix + cmd +  args + " --from-ssc"));

  appProcess = new SSC::Process(
     headlessCommand + prefix + cmd,
    args + " --from-ssc",
    fs::current_path().string(),
    [](SSC::String const &out) { std::cout << out << std::endl; },
    [](SSC::String const &out) { std::cerr << out << std::endl; }
  );

  appPid = appProcess->open();
  appProcess->wait();
  auto status = appProcess->status.load();

  if (status > -1) {
    appStatus = status;
  }

  delete appProcess;
  appProcess = nullptr;

  log("App result: " + std::to_string(appStatus));
  return appStatus;
}

int runApp (const Path& path, const String& args) {
  return runApp(path, args, false);
}

void runIOSSimulator (const Path& path, Map& settings) {
  #ifndef _WIN32
  if (settings["ios_simulator_device"].size() == 0) {
    log("ERROR: 'ios_simulator_device' option is empty");
    exit(1);
  }
  String deviceType;
  StringStream listDeviceTypesCommand;
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
      "failed to find device type: " + settings["ios_simulator_device"] + ". "
      "Please provide correct device name for the \"ios_simulator_device\". "
      "The list of available devices:\n" + rListDevices.output
    );
    if (rListDevices.output.size() > 0) {
      log(rListDevices.output);
    }
    exit(rListDevices.exitCode);
  }

  StringStream listDevicesCommand;
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
  std::regex reSocketSDKDevice("SocketSimulator_" + iosSimulatorDeviceSuffix + "\\s\\((.+)\\)\\s\\((.+)\\)");

  String uuid;
  bool booted = false;

  if (std::regex_search(rListDevices.output, match, reSocketSDKDevice)) {
    uuid = match.str(1);
    booted = match.str(2).find("Booted") != String::npos;

    log("found Socket simulator VM for " + settings["ios_simulator_device"] + " with uuid: " + uuid);
    if (booted) {
      log("Socket simulator VM is booted");
    } else {
      log("Socket simulator VM is not booted");
    }
  } else {
    log("creating a new iOS simulator VM for " + settings["ios_simulator_device"]);

    StringStream listRuntimesCommand;
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
    String runtime;
    // TODO: improve iOS version detection
    for (auto it = runtimes.rbegin(); it != runtimes.rend(); ++it) {
      if (it->find("iOS") != String::npos) {
        runtime = trim(*it);
        log("found runtime: " + runtime);
        break;
      }
    }

    std::regex reRuntime(R"(com.apple.CoreSimulator.SimRuntime.iOS(?:.*))");
    std::smatch matchRuntime;
    String runtimeId;

    if (std::regex_search(runtime, matchRuntime, reRuntime)) {
      runtimeId = matchRuntime.str(0);
    }

    StringStream createSimulatorCommand;
    createSimulatorCommand
      << "xcrun simctl"
      << " create SocketSimulator_" + iosSimulatorDeviceSuffix
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
    StringStream bootSimulatorCommand;
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

  auto pathToXCode = Path("/Applications") / "Xcode.app" / "Contents";
  auto pathToSimulator = pathToXCode / "Developer" / "Applications" / "Simulator.app";

  StringStream simulatorCommand;
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

  StringStream installAppCommand;

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

  StringStream launchAppCommand;
  launchAppCommand
    << "xcrun"
    << " simctl launch --console-pty booted"
    << " " + settings["meta_bundle_identifier"];

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
  #endif
};

static String getCxxFlags() {
  auto flags = getEnv("CXXFLAGS");
  return flags.size() > 0 ? " " + flags : "";
}

void printHelp (const String& command) {
  if (command == "ssc") {
    std::cout << tmpl(gHelpText, defaultTemplateAttrs) << std::endl;
  } else if (command == "build") {
    std::cout << tmpl(gHelpTextBuild, defaultTemplateAttrs) << std::endl;
  } else if (command == "list-devices") {
    std::cout << tmpl(gHelpTextListDevices, defaultTemplateAttrs) << std::endl;
  } else if (command == "init") {
    std::cout << tmpl(gHelpTextInit, defaultTemplateAttrs) << std::endl;
  } else if (command == "install-app") {
    std::cout << tmpl(gHelpTextInstallApp, defaultTemplateAttrs) << std::endl;
  } else if (command == "print-build-dir") {
    std::cout << tmpl(gHelpTextPrintBuildDir, defaultTemplateAttrs) << std::endl;
  } else if (command == "run") {
    std::cout << tmpl(gHelpTextRun, defaultTemplateAttrs) << std::endl;
  } else if (command == "setup") {
    std::cout << tmpl(gHelpTextSetup, defaultTemplateAttrs) << std::endl;
  }
}

inline String getCfgUtilPath() {
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

void initializeEnv (Path targetPath) {
  static auto SSC_ENV_FILENAME = getEnv("SSC_ENV_FILENAME");
  static auto filename = SSC_ENV_FILENAME.size() > 0
    ? SSC_ENV_FILENAME
    : DEFAULT_SSC_ENV_FILENAME;

  auto path = targetPath / filename;

  // just set to `targetPath` if resolved `path` doesn't exist
  if (!fs::exists(path) && fs::is_regular_file(path)) {
    path = targetPath;
  }

  if (fs::exists(path) && fs::is_regular_file(path)) {
    auto env = parseINI(readFile(path));
    for (const auto& tuple : env) {
      auto key = tuple.first;
      auto value = tuple.second;
      auto valueAsPath = Path(value).make_preferred();

      // convert env value to normalized path if it exists
      if (fs::exists(fs::status(valueAsPath))) {
        value = valueAsPath.string();
      }

      setEnv(key, value);
    }
  }
}

void initializeRC (Path targetPath) {
  static auto SSC_RC_FILENAME = getEnv("SSC_RC_FILENAME");
  static auto SSC_RC = getEnv("SSC_RC");
  static auto filename = SSC_RC_FILENAME.size() > 0
    ? SSC_RC_FILENAME
    : DEFAULT_SSC_RC_FILENAME;

  auto path = SSC_RC.size() > 0
    ? Path(SSC_RC)
    : targetPath / filename;

  // just set to `targetPath` if resolved `path` doesn't exist
  if (!fs::exists(path) && fs::is_regular_file(path)) {
    path = targetPath;
  }

  if (fs::exists(path) && fs::is_regular_file(path)) {
    extendMap(rc, parseINI(readFile(path)));

    for (const auto& tuple : rc) {
      auto key = tuple.first;
      auto value = tuple.second;
      auto valueAsPath = Path(value).make_preferred();

      // convert env value to normalized path if it exists
      if (fs::exists(fs::status(valueAsPath))) {
        value = valueAsPath.string();
      }

      // auto set environment variables
      if (key.starts_with("env_")) {
        key = key.substr(4, key.size() - 4);
        setEnv(key, value);
      }
    }
  }
}

bool isSetupCompleteAndroid() {
  auto androidHome = getAndroidHome();
  if (androidHome.size() == 0) {
    return false;
  }

  if (!fs::exists(androidHome)) {
    return false;
  }

  Path sdkManager = androidHome + "/" + getEnv("ANDROID_SDK_MANAGER");

  if (!fs::exists(sdkManager)) {
    return false;
  }

  if (!fs::exists(getEnv("JAVA_HOME"))) {
    return false;
  }

  return true;
}

bool isSetupCompleteWindows() {
  if (getEnv("CXX").size() == 0) {
    return false;
  }

  return fs::exists(getEnv("CXX"));
}


bool isSetupComplete(SSC::String platform) {
  std::map<SSC::String, bool(*)()> funcs;

  funcs["android"] = isSetupCompleteAndroid;
  funcs["windows"] = isSetupCompleteWindows;

  if (funcs.count(platform) == 0) return true;

  return funcs[platform]();
}

int main (const int argc, const char* argv[]) {  
  defaultTemplateAttrs = {{ "ssc_version", SSC::VERSION_FULL_STRING }};
  if (argc < 2) {
    printHelp("ssc");
    exit(0);
  }

  auto is = [](const String& s, const auto& p) -> bool {
    return s.compare(p) == 0;
  };

  auto optionValue = [](
    const String& c, // command
    const String& s,
    const String& p
  ) -> String {
    auto string = p + String("=");
    if (String(s).find(string) == 0) {
      auto value = s.substr(string.size());
      if (value.size() == 0) {
        value = rc[c + "_" + p];
      }

      if (value.size() == 0) {
        log("missing value for option " + String(p));
        exit(1);
      }
      return value;
    }
    return "";
  };

  auto const subcommand = argv[1];

#ifndef _WIN32
  signal(SIGHUP, signalHandler);
  signal(SIGUSR1, signalHandler);
#endif

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  if (is(subcommand, "-v") || is(subcommand, "--version")) {
    std::cout << SSC::VERSION_FULL_STRING << std::endl;
    exit(0);
  }

  if (is(subcommand, "-h") || is(subcommand, "--help")) {
    printHelp("ssc");
    exit(0);
  }

  if (is(subcommand, "--prefix")) {
    std::cout << getSocketHome() << std::endl;
    exit(0);
  }

  if (subcommand[0] == '-') {
    log("unknown option: " + String(subcommand));
    printHelp("ssc");
    exit(0);
  }

  auto const lastOption = argv[argc-1];
  Path targetPath;
  int numberOfOptions = argc - 3;

  // if no path provided, use current directory
  if (argc == 2 || lastOption[0] == '-') {
    numberOfOptions = argc - 2;
    targetPath = fs::current_path();
  } else {
    targetPath = fs::absolute(lastOption).lexically_normal();
  }

#if defined(_WIN32)
  static String HOME = getEnv("HOMEPATH");
#else
  static String HOME = getEnv("HOME");
#endif

  // `/etc/sscrc` (global)
  initializeRC(Path("etc")  / "sscrc" );
  // `/etc/ssc.rc` (global)
  initializeRC(Path("etc")  / "ssc.rc" );
  // `/etc/ssc/config` (global)
  initializeRC(Path("etc")  / "ssc" / "config");
  // `$HOME/.ssc/config` (user)
  initializeRC(Path(HOME)  / ".ssc" / "config");
  // `$HOME/.config/ssc` (user)
  initializeRC(Path(HOME)  / ".config" / "ssc");
  // `$HOME/.sscrc` (user)
  initializeRC(HOME);
  // `$PWD/.sscrc` (local)
  initializeRC(targetPath);

  // `$SOCKET_HOME/.ssc.env` (global/user)
  initializeEnv(prefixFile());
  // `$PWD/.ssc.env` (local)
  initializeEnv(targetPath); // overrides global config with local

  struct Paths {
    Path pathBin;
    Path pathPackage;
    Path pathResourcesRelativeToUserBuild;
    Path platformSpecificOutputPath;
  };

  auto getPaths = [&](String platform) -> Paths {
    Paths paths;
    String platformPath = platform == "win32"
      ? "win"
      : platform;
    paths.platformSpecificOutputPath = {
      targetPath /
      settings["build_output"] /
      platformPath
    };
    if (platform == "mac") {
      Path pathBase = "Contents";
      Path packageName = Path(settings["build_name"] + ".app");
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
      settings["meta_revision"] = "1";

      // this follows the .deb file naming convention
      Path packageName = (
        settings["build_name"] + "_" +
        "v" + settings["meta_version"] + "-" +
        settings["meta_revision"] + "_" +
        "amd64"
      );
      paths.pathPackage = { paths.platformSpecificOutputPath  / packageName };
      paths.pathBin = {
        paths.pathPackage /
        "opt" /
        settings["build_name"]
      };
      paths.pathResourcesRelativeToUserBuild = paths.pathBin;
      return paths;
    } else if (platform == "win32") {
      paths.pathPackage = {
        paths.platformSpecificOutputPath  /
        Path(settings["build_name"] + "-v" + settings["meta_version"])
      };

      paths.pathBin = paths.pathPackage;
      paths.pathResourcesRelativeToUserBuild = paths.pathPackage;
      return paths;
    } else if (platform == "ios" || platform == "ios-simulator") {
      Path pathBase = "Contents";
      Path packageName = settings["build_name"] + ".app";
      paths.pathPackage = { paths.platformSpecificOutputPath  / packageName };
      paths.pathBin = { paths.platformSpecificOutputPath / pathBase / "MacOS" };
      paths.pathResourcesRelativeToUserBuild = paths.platformSpecificOutputPath / "ui";
      return paths;
    } else if (platform == "android" || platform == "android-emulator") {
      auto output = fs::absolute(targetPath) / paths.platformSpecificOutputPath;
      paths.pathResourcesRelativeToUserBuild = {
        paths.platformSpecificOutputPath / "app" / "src" / "main" / "assets"
      };
      return paths;
    }
    log("unknown platform: " + platform);
    exit(1);
  };

  auto createSubcommand = [&](
    const String& subcommand,
    const std::vector<String>& options,
    const bool& needsConfig,
    std::function<void(std::span<const char *>)> subcommandHandler
  ) -> void {
    if (argv[1] == subcommand) {
      if (argc > 2 && (is(argv[2], "-h") || is(argv[2], "--help"))) {
        printHelp(subcommand);
        exit(0);
      }

      auto commandlineOptions = std::span(argv, argc).subspan(2, numberOfOptions);
      auto envs = Vector<String>();

      for (auto const& arg : commandlineOptions) {
        auto isAcceptableOption = false;
        if (String(arg).starts_with("--env")) {
          auto value = optionValue(subcommand, arg, "--env");
          if (value.size() > 0) {
            auto parts = parseStringList(value);
            for (const auto& part : parts) {
              envs.push_back(part);
            }
            continue;
          }
        }

        if (is(arg, "--verbose")) {
          setEnv("SSC_VERBOSE", "1");
          continue;
        }

        if (is(arg, "--debug")) {
          setEnv("SSC_DEBUG", "1");
          continue;
        }

        for (auto const& option : options) {
          if (is(arg, option) || optionValue(subcommand, arg, option).size() > 0) {
            isAcceptableOption = true;
            break;
          }
        }

        if (!isAcceptableOption) {
          log("unrecognized option: " + String(arg));
          printHelp(subcommand);
          exit(1);
        }
      }

      if (needsConfig) {
        auto configExists = false;
        auto configPath = targetPath / "socket.ini";
        auto code = String("");
        auto ini = String("");

        if (fs::exists(configPath)) {
          ini = readFile(configPath);
          configExists = true;
        } else if (!is(subcommand, "init") && !is(subcommand, "env")) {
          log("socket.ini not found in " + targetPath.string());
          exit(1);
        }

        if (envs.size() > 0) {
          auto stream = StringStream();
          stream << "\n";
          stream << "[env]" << "\n";
          for (const auto& value : envs) {
            auto parts = split(value, '=');
            if (parts.size() == 2) {
              stream << parts[0] << " = " << parts[1] << "\n";
            } else if (parts.size() == 1) {
              stream << parts[0] << " = " << getEnv(parts[0].c_str()) << "\n";
            }
          }

          ini += stream.str();
        }

        if (configExists) {
          auto hex = stringToHex(ini);
          auto bytes = StringStream();
          auto length = hex.size() - 1;
          int size = 0;

          for (int i = 0; i < length; i += 2) {
            size++;

            if (i != 0 && (i & 3) == 0) {
              bytes << "\n";
            }

            bytes << "  ";
            if (i + 2 < length) {
              bytes << "0x" << hex.substr(i, 2) << ",";
            } else if (i + 1 < length) {
              bytes << "0x0" << hex.substr(i, 1);
            } else {
              bytes << "0x" << hex.substr(i, 2);
            }
          }

          code = String(
            "constexpr unsigned char __ssc_config_bytes["+ std::to_string(size) +"] = {\n" + bytes.str() + "\n};"
          );
        }

        settings = parseINI(ini);

        // allow for local `.sscrc` '[settings] ...' entries to overload the
        // project's settings in `socket.ini`:
        // [settings.ios]
        // simulator_device = "My local device"
        Map tmp;
        for (const auto& tuple : rc) {
          if (tuple.first.starts_with("settings_")) {
            auto key = replace(tuple.first, "settings_", "");
            tmp[key] = tuple.second;
          }
        }

        extendMap(settings, tmp);

        if (code.size() > 0) {
          settings["ini_code"] = code;
        }

        // only validate if config exists and we didn't exit early above
        if (configExists) {
          // Check if build_name is set
          if (settings.count("build_name") == 0) {
            log("ERROR: 'name' value is required in socket.ini in the [build] section");
            exit(1);
          }

          // Define regular expression to match spaces, and special characters except dash and underscore
          std::regex name_pattern("[^a-zA-Z0-9_\\-]");
          // Check if the name matches the pattern
          if (std::regex_search(settings["build_name"], name_pattern)) {
            log("ERROR: 'name' in socket.ini [build] section can only contain alphanumeric characters, dashes, and underscores");
            exit(1);
          }

          // Define regular expression to match semver format
          // The semver specification is available at https://semver.org/
          // The pre-release and build metadata are not supported
          std::regex semver_pattern("^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)$");
          // Check if the version matches the pattern
          if (!std::regex_match(settings["meta_version"], semver_pattern)) {
            log("ERROR: 'version' in [meta] section of socket.ini must be in semver format");
            exit(1);
          }

          // default values
          settings["build_output"] = settings["build_output"].size() > 0 ? settings["build_output"] : "build";
          settings["meta_lang"] = settings["meta_lang"].size() > 0 ? settings["meta_lang"] : "en-us";
          settings["meta_version"] = settings["meta_version"].size() > 0 ? settings["meta_version"] : "1.0.0";
          settings["meta_title"] = settings["meta_title"].size() > 0 ? settings["meta_title"] : settings["build_name"];

          for (auto const arg : std::span(argv, argc).subspan(2, numberOfOptions)) {
            if (is(arg, "--prod")) {
              flagDebugMode = false;
              break;
            }
          }

          String suffix = "";

          // internal
          if (flagDebugMode) {
            settings["apple_instruments"] = "true";
            suffix += "-dev";
          } else {
            settings["apple_instruments"] = "false";
          }

          settings["debug"] = flagDebugMode;

          std::replace(settings["build_name"].begin(), settings["build_name"].end(), ' ', '_');
          settings["build_name"] += suffix;
        }
      }

      subcommandHandler(commandlineOptions);
    }
  };

  createSubcommand("init", { "--config" }, false, [&](const std::span<const char *>& options) -> void {
    auto configOnly = false;
    for (auto const& option : options) {
      if (is(option, "--config")) {
        configOnly = true;
      }
    }
    if (fs::exists(targetPath / "socket.ini")) {
      log("socket.ini already exists in " + targetPath.string());
      exit(0);
    } else {
      SSC::writeFile(targetPath / "socket.ini", tmpl(gDefaultConfig, defaultTemplateAttrs));
      log("socket.ini created in " + targetPath.string());
    }
    if (!configOnly) {
      if (fs::exists(targetPath / "src")) {
        log("src directory already exists in " + targetPath.string());
      } else {
        fs::create_directories(targetPath / "src");
        SSC::writeFile(targetPath / "src" / "index.html", gHelloWorld);
        log("src/index.html created in " + targetPath.string());
      }
      if (fs::exists(targetPath / ".gitignore")) {
        log(".gitignore already exists in " + targetPath.string());
      } else {
        SSC::writeFile(targetPath / ".gitignore", gDefaultGitignore);
        log(".gitignore created in " + targetPath.string());
      }
    }
    exit(0);
  });

  createSubcommand("list-devices", { "--platform", "--ecid", "--udid", "--only" }, false, [&](const std::span<const char *>& options) -> void {
    String targetPlatform = "";
    bool isUdid = false;
    bool isEcid = false;
    bool isOnly = false;
    for (auto const& option : options) {
      auto platform = optionValue("list-devices", options[0], "--platform");
      if (platform.size() > 0) {
        targetPlatform = platform;
      }
      if (is(option, "--udid") || is(rc["list-devices_udid"], "true")) {
        isUdid = true;
      }
      if (is(option, "--ecid") || is(rc["list-devices_ecid"], "true")) {
        isEcid = true;
      }
      if (is(option, "--only") || is(rc["list-devices_only"], "true")) {
        isOnly = true;
      }
    }

    if (targetPlatform.size() == 0) {
      log("ERROR: --platform option is required");
      exit(1);
    }

    if (targetPlatform == "ios" && platform.mac) {
      if (isUdid && isEcid) {
        log("--udid and --ecid are mutually exclusive");
        printHelp("list-devices");
        exit(1);
      }

      if (isOnly && !isUdid && !isEcid) {
        log("--only requires --udid or --ecid");
        printHelp("list-devices");
        exit(1);
      }

      String cfgUtilPath = getCfgUtilPath();
      String command = cfgUtilPath + " list-devices";
      auto r = exec(command);

      if (r.exitCode == 0) {
        std::regex re(R"(ECID:\s(\S*)\s*UDID:\s(\S*).*Name:\s(.*))");
        std::smatch matches;
        String uuid;

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
    } else if (targetPlatform == "android") {
      auto androidHome = getAndroidHome();
      StringStream adb;

      if (!platform.win) {
        adb << androidHome << "/platform-tools/";
      } else {
        adb << androidHome << "\\platform-tools\\";
      }

      auto r = exec(adb.str() + "adb devices | tail -n +2");
      std::regex re(R"((.*)\s*device)");
      std::smatch matches;

      if (r.exitCode != 0) {
        exit(r.exitCode);
      }

      while (std::regex_search(r.output, matches, re)) {
        std::cout << matches[1] << std::endl;
        r.output = matches.suffix();
        if (isOnly) {
          break;
        }
      }

      exit(0);
    } else {
      log("list-devices is only supported for iOS devices on macOS.");
      exit(0);
    }
  });

  createSubcommand("install-app", { "--platform", "--device" }, true, [&](const std::span<const char *>& options) -> void {
    String commandOptions = "";
    String targetPlatform = "";
    // we need to find the platform first
    for (auto const option : options) {
      if (targetPlatform.size() == 0) {
        targetPlatform = optionValue("install-app", option, "--platform");
      }

      if (targetPlatform.size() > 0) {
        // just assume android when 'android-emulator' is given
        if (targetPlatform == "android-emulator") {
          targetPlatform = "android";
        }

        if (targetPlatform != "ios" && targetPlatform != "android") {
          std::cout << "Unsupported platform: " << targetPlatform << std::endl;
          exit(1);
        }
      }
    }

    if (targetPlatform.size() == 0) {
      log("--platform option is required.");
      printHelp("install-app");
      exit(1);
    }

    // then we need to find the device
    for (auto const option : options) {
      auto device = optionValue("install-app", option, "--device");
      if (device.size() > 0) {
        if (targetPlatform == "ios") {
          commandOptions += " --ecid " + device + " ";
        } else if (targetPlatform == "android") {
          commandOptions += " -s " + device;
        }
      }
    }

    if (targetPlatform == "ios") {
      auto cfgUtilPath = getCfgUtilPath();
      auto ipaPath = (
        getPaths(targetPlatform).platformSpecificOutputPath /
        "build" /
        String(settings["build_name"] + ".ipa") /
        String(settings["build_name"] + ".ipa")
      );

      if (!fs::exists(ipaPath)) {
        log("Could not find " + ipaPath.string());
        exit(1);
      }

      // this command will install the app to the first connected device which was
      // added to the provisioning profile if no --device is provided.
      auto command = cfgUtilPath + " " + commandOptions + "install-app " + ipaPath.string();
      auto r = exec(command);

      if (r.exitCode != 0) {
        log("ERROR: failed to install the app. Is the device plugged in?");
        exit(1);
      }
    }

    if (targetPlatform == "android") {
      auto androidHome = getAndroidHome();
      auto paths = getPaths(targetPlatform);
      auto output = paths.platformSpecificOutputPath;
      auto app = output / "app";
      StringStream adb;

      if (!platform.win) {
        adb << androidHome << "/platform-tools/";
      } else {
        adb << androidHome << "\\platform-tools\\";
      }

      adb << "adb" << commandOptions << " install ";

      if (flagDebugMode) {
        adb << (app / "build" / "outputs" / "apk" / "dev" / "debug" / "app-dev-debug.apk").string();
      } else {
        adb << (app / "build" / "outputs" / "apk" / "live" / "debug" / "app-live-debug.apk").string();
      }

      auto command = adb.str();
      auto r = exec(command);

      if (r.exitCode != 0) {
        log("ERROR: failed to install the app. Is the device plugged in?");
        exit(1);
      }
    }

    log("successfully installed the app on your device(s).");
    exit(0);
  });

  createSubcommand("print-build-dir", { "--platform", "--prod" }, true, [&](const std::span<const char *>& options) -> void {
    // if --platform is specified, use the build path for the specified platform
    for (auto const option : options) {
      auto targetPlatform = optionValue("print-build-dir", option, "--platform");
      if (targetPlatform.size() > 0) {
        Path path = getPaths(targetPlatform).pathResourcesRelativeToUserBuild;
        std::cout << path.string() << std::endl;
        exit(0);
      }
    }
    // if no --platform option is provided, print the current platform build path
    std::cout << getPaths(platform.os).pathResourcesRelativeToUserBuild.string() << std::endl;
    exit(0);
  });

  createSubcommand("build", { "--platform", "--host", "--port", "--quiet", "-o", "--only-build", "-r", "--run", "--prod", "-p", "-c", "-s", "-e", "-n", "--test", "--headless" }, true, [&](const std::span<const char *>& options) -> void {
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
    bool flagBuildTest = false;

    String argvForward = "";
    String targetPlatform = "";
    String testFile = "";

    String hostArg = "";
    String portArg = "";
    String devHost("localhost");
    String devPort("0");
    auto cnt = 0;

    const bool debugEnv = (
      getEnv("SSC_DEBUG").size() > 0 ||
      getEnv("DEBUG").size() > 0
    );

    const bool verboseEnv = (
      getEnv("SSC_VERBOSE").size() > 0 ||
      getEnv("VERBOSE").size() > 0
    );

    String localDirPrefix = !platform.win ? "./" : "";
    String quote = !platform.win ? "'" : "\"";
    String slash = !platform.win ? "/" : "\\";

    for (auto const arg : options) {
      if (is(arg, "-h") || is(arg, "--help")) {
        printHelp("build");
        exit(0);
      }

      if (is(arg, "-c") || is(rc["build_code_sign"], "true")) {
        flagCodeSign = true;
      }

      if (is(arg, "-e") || is(rc["build_entitlements"], "true")) {
        if (platform.os != "mac") {
          log("WARNING: Build entitlements (-e) are only supported on macOS. Ignoring option.");
        } else {
          // TODO: we don't use it
          flagEntitlements = true;
        }
      }

      if (is(arg, "-n") || is(rc["build_notarize"], "true")) {
        if (platform.os != "mac") {
          log("WARNING: Build notarization (-n) are only supported on macOS. Ignoring option.");
        } else {
          flagShouldNotarize = true;
        }
      }

      if (is(arg, "-o") || is(arg, "--only-build") || is(rc["build_only"], "true")) {
        flagRunUserBuildOnly = true;
      }

      if (is(arg, "-p") || is(rc["build_package"], "true")) {
        flagShouldPackage = true;
      }

      if (is(arg, "-r") || is(arg, "--run") || is(rc["build_run"], "true")) {
        flagShouldRun = true;
      }

      if (is(arg, "-s")|| is(rc["build_app_store"], "true")) {
        flagAppStore = true;
      }

      if (is(arg, "--test")) {
        flagBuildTest = true;
      }

      const auto testFileTmp = optionValue("build", arg, "--test");
      if (testFileTmp.size() > 0) {
        testFile = testFileTmp;
        flagBuildTest = true;
        argvForward += " " + String(arg);
      }

      if (is(arg, "--headless") || is(rc["build_headless"], "true")) {
        argvForward += " --headless";
        flagHeadless = true;
      }

      if (is(arg, "--quiet") || is(rc["build_quiet"], "true")) {
        flagQuietMode = true;
      }

      if (targetPlatform.size() == 0) {
        targetPlatform = optionValue("build", arg, "--platform");
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
      }

      if (hostArg.size() == 0) {
        hostArg = optionValue("build", arg, "--host");
        if (hostArg.size() > 0) {
          devHost = hostArg;
        } else {
          if (flagBuildForIOS || flagBuildForAndroid) {
            auto r = exec((!platform.win)
              ? "ifconfig | grep -w 'inet' | awk '!match($2, \"^127.\") {print $2; exit}' | tr -d '\n'"
              : "PowerShell -Command ((Get-NetIPAddress -AddressFamily IPV4).IPAddress ^| Select-Object -first 1)"
            );

            if (r.exitCode == 0) {
              devHost = r.output;
            }
          }
        }
      }

      if (portArg.size() == 0) {
        portArg = optionValue("build", arg, "--port");
        if (portArg.size() > 0) {
          devPort = portArg;
        }
      }
    }

    if (flagBuildTest && testFile.size() == 0) {
      log("ERROR: --test value is required.");
      exit(1);
    }

    if (flagBuildTest && fs::exists(testFile) == false) {
      log("ERROR: file " + testFile + " does not exist.");
      exit(1);
    }

    if (settings.count("meta_file_limit") == 0) {
      settings["meta_file_limit"] = "4096";
    }

    // set it automatically, hide it from the user
    settings["meta_revision"] = "1";

    if (getEnv("CXX").size() == 0) {
      log("WARNING: $CXX env var not set, assuming defaults");

      if (platform.win) {
        setEnv("CXX=clang++");
      } else {
        setEnv("CXX=/usr/bin/clang++");
      }
    }

    targetPlatform = targetPlatform.size() > 0 ? targetPlatform : platform.os;
    auto platformFriendlyName = targetPlatform == "win32" ? "windows" : targetPlatform;
    platformFriendlyName = platformFriendlyName == "android-emulator" ? "android" : platformFriendlyName;

    if (getEnv("CI").size() == 0 && !isSetupComplete(platformFriendlyName)) {
      log("Build dependency setup is incomplete for " + platformFriendlyName + ", Use 'setup' to resolve: ");
      std::cout << "ssc setup --platform=" << platformFriendlyName << std::endl;
      exit(1);
    }

    Paths paths = getPaths(targetPlatform);

    auto executable = Path(settings["build_name"] + (platform.win ? ".exe" : ""));
    auto binaryPath = paths.pathBin / executable;
    auto configPath = targetPath / "socket.ini";

    if (!fs::exists(binaryPath) && !flagBuildForAndroid && !flagBuildForAndroidEmulator) {
      flagRunUserBuildOnly = false;
    } else {
      struct stat stats;
      if (stat(WStringToString(binaryPath).c_str(), &stats) == 0) {
        if (SSC_BUILD_TIME > stats.st_mtime) {
          flagRunUserBuildOnly = false;
        }
      }
    }

    if (flagRunUserBuildOnly) {
      struct stat binaryPathStats;
      struct stat configPathStats;

      if (stat(WStringToString(binaryPath).c_str(), &binaryPathStats) == 0) {
        if (stat(WStringToString(configPath).c_str(), &configPathStats) == 0) {
          if (configPathStats.st_mtime > binaryPathStats.st_mtime) {
            flagRunUserBuildOnly = false;
          }
        }
      }
    }

    auto removePath = paths.platformSpecificOutputPath;
    if (targetPlatform == "android")
        removePath = settings["build_output"] + "/android/app/src";

    if (flagRunUserBuildOnly == false && fs::exists(removePath)) {
      auto p = fs::current_path() / Path(removePath);
      try {
        fs::remove_all(p);
        log(String("cleaned: " + p.string()));
      } catch (fs::filesystem_error const& ex) {
        log("could not clean path (binary could be busy)");
        log(String("ex: ") + ex.what());
        log(String("ex code: ") + std::to_string(ex.code().value()));

        if (flagBuildForAndroid) {
          log("check for gradle processes.");
        }

        exit(1);
      }
    }

    String flags;
    String files;

    Path pathResources;
    Path pathToArchive;

    if (platform.mac && exec("xcode-select -p").exitCode != 0) {
      log("Please install Xcode from https://apps.apple.com/us/app/xcode/id497799835");
      exit(1);
    }

    if (!flagBuildForAndroid && !flagBuildForIOS) {
      fs::create_directories(paths.platformSpecificOutputPath / "include");
      writeFile(paths.platformSpecificOutputPath / "include" / "user-config-bytes.hh", settings["ini_code"]);
    }

    //
    // Darwin Package Prep
    // ---
    //
    if (platform.mac && !flagBuildForIOS && !flagBuildForAndroid) {
      log("preparing build for mac");

      flags = "-std=c++2a -ObjC++ -v";
      flags += " -framework UniformTypeIdentifiers";
      flags += " -framework CoreBluetooth";
      flags += " -framework Network";
      flags += " -framework UserNotifications";
      flags += " -framework WebKit";
      flags += " -framework Cocoa";
      flags += " -framework OSLog";
      flags += " -DMACOS=1";
      flags += " -I" + prefixFile();
      flags += " -I" + prefixFile("include");
      flags += " -L" + prefixFile("lib/" + platform.arch + "-desktop");
      flags += " -lsocket-runtime";
      flags += " -luv";
      flags += " -I" + Path(paths.platformSpecificOutputPath / "include").string();
      files += prefixFile("objects/" + platform.arch + "-desktop/desktop/main.o");
      files += prefixFile("src/init.cc");
      flags += " " + getCxxFlags();

      Path pathBase = "Contents";
      pathResources = { paths.pathPackage / pathBase / "Resources" };

      fs::create_directories(paths.pathBin);
      fs::create_directories(pathResources);

      auto plistInfo = tmpl(gPListInfo, settings);

      writeFile(paths.pathPackage / pathBase / "Info.plist", plistInfo);

      auto credits = tmpl(gCredits, defaultTemplateAttrs);

      writeFile(paths.pathResourcesRelativeToUserBuild / "Credits.html", credits);
    }


    // used in multiple if blocks, need to declare here
    auto androidEnableStandardNdkBuild = settings["android_enable_standard_ndk_build"] == "true";

    if (flagBuildForAndroid) {
      auto bundle_identifier = settings["meta_bundle_identifier"];
      auto bundle_path = Path(replace(bundle_identifier, "\\.", "/")).make_preferred();
      auto bundle_path_underscored = replace(bundle_identifier, "\\.", "_");

      auto output = paths.platformSpecificOutputPath;
      auto app = output / "app";
      auto src = app / "src";
      auto jni = src / "main" / "jni";
      auto res = src / "main" / "res";
      auto pkg = src / "main" / "java" / bundle_path;

      if (settings["android_main_activity"].size() == 0) {
        settings["android_main_activity"] = String(DEFAULT_ANDROID_ACTIVITY_NAME);
      }

      // clean and create output directories
      //fs::remove_all(output);
      fs::create_directories(output);
      fs::create_directories(src);
      fs::create_directories(pkg);
      fs::create_directories(jni);
      fs::create_directories(jni / "android");
      fs::create_directories(jni / "app");
      fs::create_directories(jni / "core");
      fs::create_directories(jni / "include");
      fs::create_directories(jni / "ipc");
      fs::create_directories(jni / "src");
      fs::create_directories(jni / "window");

      fs::create_directories(res);
      fs::create_directories(res / "layout");
      fs::create_directories(res / "values");

      pathResources = { src / "main" / "assets" };

      // set current path to output directory
      if (debugEnv || verboseEnv) log("cd " + output.string());
      fs::current_path(output);

      auto androidHome = getAndroidHome();
      StringStream sdkmanager;

      if (debugEnv || verboseEnv) log("sdkmanager --version 2>&1 >/dev/null");

      sdkmanager << androidHome;
      if (getEnv("ANDROID_SDK_MANAGER").size() > 0)
      {
        sdkmanager << "/" << getEnv("ANDROID_SDK_MANAGER");
      } else if (std::system("  sdkmanager --version 2>&1 >/dev/null") != 0) {
        if (!platform.win) {
          sdkmanager << "/cmdline-tools/latest/bin/sdkmanager";
        } else {
          sdkmanager << "\\cmdline-tools\\latest\\bin\\sdkmanager.bat";
        }
      }

      // Create default output to advise user in case gradle init fails

      // TODO(mribbons): Check if we need this in CI - Upload licenses folder from workstation
      // https://developer.android.com/studio/intro/update#download-with-gradle


      String licenseAccept = 
       (getEnv("ANDROID_SDK_MANAGER_ACCEPT_LICENSES").size() > 0 ? getEnv("ANDROID_SDK_MANAGER_ACCEPT_LICENSES") : "echo") + " | " +
       sdkmanager.str() + " --licenses";

      if (std::system((licenseAccept).c_str()) != 0) {
        // Windows doesn't support 'yes'
        log(("Check licenses and run again: \n" + licenseAccept + "\n").c_str());
      }

      // TODO: internal, no need to save to settings
      settings["android_sdk_manager_path"] = sdkmanager.str();
      
      String gradlePath = getEnv("GRADLE_HOME").size() > 0 ? getEnv("GRADLE_HOME") + slash + "bin" + slash : "";

      StringStream gradleInitCommand;
      gradleInitCommand
        << "echo 1 |"
        << gradlePath
        << "gradle "
        << "--no-configuration-cache "
        << "--no-build-cache "
        << "--no-scan "
        << "--offline "
        << "--quiet "
        << "init";

      if (debugEnv || verboseEnv) log(gradleInitCommand.str());
      if (std::system(gradleInitCommand.str().c_str()) != 0) {
        log("ERROR: failed to invoke `gradle init` command");
        // In case user didn't accept licenses above
        log(
          String("Check licenses and run again: \n") +
          (platform.win ? "set" : "export") +
          (" JAVA_HOME=\"" + getEnv("JAVA_HOME") + "\" && ") +
          licenseAccept +
          "\n"
         );
        exit(1);
      }

      //writeFile();

      // Core
      fs::copy(trim(prefixFile("src/common.hh")), jni, fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/init.cc")), jni, fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/android/bridge.cc")), jni / "android", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/android/runtime.cc")), jni / "android", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/android/string_wrap.cc")), jni / "android", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/android/window.cc")), jni / "android", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/core.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/json.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/runtime-preload.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/ipc/ipc.hh")), jni / "ipc", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/window/options.hh")), jni / "window", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/window/window.hh")), jni / "window", fs::copy_options::overwrite_existing);

      // libuv
      fs::copy(
        trim(prefixFile("uv")),
        jni / "uv",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      fs::copy(
        trim(prefixFile("include")),
        jni / "include",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      writeFile(jni / "user-config-bytes.hh", settings["ini_code"]);

      auto aaptNoCompressOptionsNormalized = std::vector<String>();
      auto aaptNoCompressDefaultOptions = split(R"OPTIONS("htm","html","txt","js","jsx","mjs","ts","css","xml")OPTIONS", ',');
      auto aaptNoCompressOptions = parseStringList(settings["android_aapt_no_compress"]);

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
        settings["android_native_abis"] = getEnv("ANDROID_SUPPORTED_ABIS");
      }

      if (settings["android_ndk_abi_filters"].size() == 0) {
        settings["android_ndk_abi_filters"] = settings["android_native_abis"];
      }

      if (settings["android_ndk_abi_filters"].size() == 0) {
        settings["android_ndk_abi_filters"] = "'arm64-v8a', 'x86_64'";
      } else {
        auto filters = settings["android_ndk_abi_filters"];
        settings["android_ndk_abi_filters"] = "";

        for (auto const &filter : parseStringList(filters)) {
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
        for (auto const &value: parseStringList(settings["android_manifest_permissions"])) {
          auto permission = replace(trim(value), "\"", "");
          StringStream xml;

          std::transform(permission.begin(), permission.end(), permission.begin(), ::toupper);

          xml
            << "<uses-permission android:name="
            << "\"android.permission." << permission << "\""
            << " />";

          manifestContext["android_manifest_xml_permissions"] += xml.str() + "\n";
        }
      }

      if (settings["android_allow_cleartext"].size() == 0) {
        if (flagDebugMode)
        {
          settings["android_allow_cleartext"] = "android:usesCleartextTraffic=\"true\"\n";
        } else {
          settings["android_allow_cleartext"] = "";
        }
      }

      // set internal variables used by templating system to generate build.gradle
      if (androidEnableStandardNdkBuild) {
        settings["android_default_config_external_native_build"].assign(
          "    externalNativeBuild {\n"
          "      ndkBuild {\n"
          "        arguments \"NDK_APPLICATION_MK:=src/main/jni/Application.mk\"\n"
          "      }\n"
          "    }"
        );

        settings["android_external_native_build"].assign(
          "  externalNativeBuild {\n"
          "    ndkBuild {\n"
          "      path \"src/main/jni/Android.mk\"\n"
          "    }\n"
          "  }"
        );
      } else {
        settings["android_default_config_external_native_build"].assign("    // externalNativeBuild called manually for -j parallel support. Disable with [android]...enable_standard_ndk_build = true in socket.ini\n");
        settings["android_external_native_build"].assign("  // externalNativeBuild called manually for -j parallel support. Disable with [android]...enable_standard_ndk_build = true in socket.ini\n");
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
      writeFile(src / "main" / "assets" / "__ssc_vital_check_ok_file__.txt", "OK");

      auto cflags = flagDebugMode
        ? settings.count("debug_flags") ? settings["debug_flags"] : ""
        : settings.count("build_flags") ? settings["build_flags"] : "";

      StringStream pp;
      pp
        << "-DDEBUG=" << (flagDebugMode ? 1 : 0) << " "
        << "-DANDROID=1" << " "
        << "-DSSC_SETTINGS=\"" << encodeURIComponent(_settings) << "\" "
        << "-DSSC_VERSION=" << SSC::VERSION_STRING << " "
        << "-DSSC_VERSION_HASH=" << SSC::VERSION_HASH_STRING << " ";

      Map makefileContext;

      makefileContext["cflags"] = cflags + " " + pp.str();
      makefileContext["cflags"] += " " + settings["android_native_cflags"];

      if (settings["android_native_abis"].size() > 0) {
        makefileContext["android_native_abis"] = settings["android_native_abis"];
      } else {
        makefileContext["android_native_abis"] = "all";
      }

      // custom native sources
      for (
        auto const &file :
        parseStringList(settings["android_native_sources"])
      ) {
        auto filename = Path(file).filename();
        writeFile(
          jni / "src" / filename,
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
        jni  / "android" / "internal.hh",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android/internal.hh")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_path_underscored
        )
      );

      writeFile(
        pkg / "bridge.kt",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android/bridge.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      writeFile(
        pkg / "main.kt",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android/main.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      writeFile(
        pkg / "runtime.kt",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android/runtime.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      writeFile(
        pkg / "window.kt",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android/window.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      writeFile(
        pkg / "webview.kt",
        std::regex_replace(
          WStringToString(readFile(trim(prefixFile("src/android/webview.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      // custom source files
      for (auto const &file : parseStringList(settings["android_sources"])) {
        // log(String("Android source: " + String(target / file)).c_str());
        writeFile(
          pkg / Path(file).filename(),
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

      auto projectName = (settings["build_name"] + ".xcodeproj");
      auto schemeName = (settings["build_name"] + ".xcscheme");
      auto pathToProject = paths.platformSpecificOutputPath / projectName;
      auto pathToScheme = pathToProject / "xcshareddata" / "xcschemes";
      auto pathToProfile = targetPath / settings["ios_provisioning_profile"];

      fs::create_directories(pathToProject);
      fs::create_directories(pathToScheme);

      if (!flagBuildForSimulator) {
        if (!fs::exists(pathToProfile)) {
          log("provisioning profile not found: " + pathToProfile.string() + ". " +
              "Please specify a valid provisioning profile in the " +
              "provisioning_profile field in the [ios] section of your socket.ini");
          exit(1);
        }
        String command = (
          "security cms -D -i " + pathToProfile.string()
        );

        auto r = exec(command);
        std::regex reUuid(R"(<key>UUID<\/key>\n\s*<string>(.*)<\/string>)");
        std::smatch matchUuid;

        if (!std::regex_search(r.output, matchUuid, reUuid)) {
          log("failed to extract uuid from provisioning profile using \"" + command + "\"");
          exit(1);
        }

        String uuid = matchUuid.str(1);

        std::regex reProvSpec(R"(<key>Name<\/key>\n\s*<string>(.*)<\/string>)");
        std::smatch matchProvSpec;

        if (!std::regex_search(r.output, matchProvSpec, reProvSpec)) {
          log("failed to extract Provisioning Specifier from provisioning profile using \"" + command + "\"");
          exit(1);
        }

        String provSpec = matchProvSpec.str(1);

        std::regex reTeamId(R"(<key>com\.apple\.developer\.team-identifier<\/key>\n\s*<string>(.*)<\/string>)");
        std::smatch matchTeamId;

        if (!std::regex_search(r.output, matchTeamId, reTeamId)) {
          log("failed to extract Team Id from provisioning profile using \"" + command + "\"");
          exit(1);
        }

        String team = matchTeamId.str(1);

        auto pathToInstalledProfile = Path(getEnv("HOME")) /
          "Library" /
          "MobileDevice" /
          "Provisioning Profiles" /
          (uuid + ".mobileprovision");

        if (!fs::exists(pathToInstalledProfile)) {
          fs::copy(pathToProfile, pathToInstalledProfile);
        }

        settings["ios_provisioning_specifier"] = provSpec;
        settings["ios_provisioning_profile"] = uuid;
        settings["apple_team_id"] = team;
      }

      if (flagBuildForSimulator) {
        settings["ios_provisioning_specifier"] = "";
        settings["ios_provisioning_profile"] = "";
        settings["ios_codesign_identity"] = "";
        settings["apple_team_id"] = "";
      }

      auto deviceType = platform.arch + "-iPhone" + (flagBuildForSimulator ? "Simulator" : "OS");

      auto deviceLibs = Path(prefixFile()) / "lib" / deviceType;
      auto deviceObjects = Path(prefixFile()) / "objects" / deviceType;

      if (!fs::exists(deviceLibs)) {
        log("ERROR: libs folder for the target platform doesn't exist: " + deviceLibs.string());
      }

      if (!fs::exists(deviceObjects)) {
        log("ERROR: objects folder for the target platform doesn't exist: " + deviceObjects.string());
      }

      if (!fs::exists(deviceLibs) || !fs::exists(deviceObjects)) {
        exit(1);
      }

      fs::copy(
        deviceLibs,
        paths.platformSpecificOutputPath / "lib",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      fs::copy(
        deviceObjects,
        paths.platformSpecificOutputPath / "objects",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      fs::copy(
        Path(prefixFile()) / "include",
        paths.platformSpecificOutputPath / "include",
        fs::copy_options::overwrite_existing | fs::copy_options::recursive
      );

      writeFile(
        paths.platformSpecificOutputPath / "include" / "user-config-bytes.hh",
        settings["ini_code"]
      );

      settings.insert(std::make_pair("host", devHost));
      settings.insert(std::make_pair("port", devPort));

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
      flags = " -std=c++2a `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1`";
      flags += " -ldl " + getCxxFlags();
      flags += " -I" + Path(paths.platformSpecificOutputPath / "include").string();
      flags += " -I" + prefixFile();
      flags += " -I" + prefixFile("include");
      flags += " -L" + prefixFile("lib/" + platform.arch + "-desktop");

      files += prefixFile("objects/" + platform.arch + "-desktop/desktop/main.o");
      files += prefixFile("src/init.cc");
      files += prefixFile("lib/" + platform.arch + "-desktop/libsocket-runtime.a");
      files += prefixFile("lib/" + platform.arch + "-desktop/libuv.a");

      pathResources = paths.pathBin;

      // @TODO(jwerle): support other Linux based OS
      Path pathControlFile = {
        paths.pathPackage /
        "DEBIAN"
      };

      Path pathManifestFile = {
        paths.pathPackage /
        "usr" /
        "share" /
        "applications"
      };

      Path pathIcons = {
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
        Path("/opt") /
        settings["build_name"] /
        settings["build_name"];

      // internal settings
      settings["linux_executable_path"] = linuxExecPath.string();
      settings["linux_icon_path"] = (
        Path("/usr") /
        "share" /
        "icons" /
        "hicolor" /
        "256x256" /
        "apps" /
        (settings["build_name"] + ".png")
      ).string();

      writeFile(pathManifestFile / (settings["build_name"] + ".desktop"), tmpl(gDestkopManifest, settings));

      writeFile(pathControlFile / "control", tmpl(gDebianManifest, settings));

      auto pathToIconSrc = (targetPath / settings["linux_icon"]).string();
      auto pathToIconDest = (pathIcons / (settings["build_name"] + ".png")).string();

      if (!fs::exists(pathToIconDest)) {
        if (fs::exists(pathToIconSrc)) {
          fs::copy(pathToIconSrc, pathToIconDest);
        } else {
          log("WARNING: [linux] icon '" + pathToIconSrc +  "' does not exist");
        }
      }
    }

    //
    // Windows Package Prep
    // ---
    //
    if (platform.win && !flagBuildForAndroid && !flagBuildForIOS) {
      log("preparing build for win");
      auto prefix = prefixFile();

      flags = " -std=c++2a"
        " -D_MT"
        " -D_DLL"
        " -DWIN32"
        " -DWIN32_LEAN_AND_MEAN"
        " -Xlinker /NODEFAULTLIB:libcmt"
        " -Wno-nonportable-include-path"
        " -I\"" + Path(paths.platformSpecificOutputPath / "include").string() + "\""
        " -I\"" + prefix + "include\""
        " -I\"" + prefix + "src\""
        " -L\"" + prefix + "lib\""
      ;

      // See install.sh for more info on windows debug builds and d suffix
      auto missing_assets = false;
      auto debugBuild = debugEnv;
      if (debugBuild) {
        for (String libString : split(getEnv("WIN_DEBUG_LIBS"), ',')) {
          if (libString.size() > 0) {
            if (libString[0] == '\"' && libString[libString.size()-2] == '\"')
              libString = libString.substr(1, libString.size()-2);

            Path lib(libString);
            if (!fs::exists(lib))
            {
              log("WARNING: WIN_DEBUG_LIBS: File doesn't exist, aborting build: " + lib.string());
              missing_assets = true;
            } else {
              flags += " " + lib.string();
            }
          }
        }
      }

      if (debugBuild) {
        flags += " -D_DEBUG";
      }

      auto d = String(debugBuild ? "d" : "" );

      flags += " -I" + prefixFile("include");
      flags += " -L" + prefixFile("lib" + d + "/" + platform.arch + "-desktop");
      auto main_o = prefixFile("objects/" + platform.arch + "-desktop/desktop/main" + d + ".o");
      if (!fs::exists(main_o)) {
        log("WARNING: Can't find main obj, unable to build: " + main_o);
        missing_assets = true;
      } else {
        files += main_o;
      }
      files += prefixFile("src/init.cc");
      auto static_runtime = prefixFile("lib" + d + "/" + platform.arch + "-desktop/libsocket-runtime" + d + ".a");
      if (!fs::exists(static_runtime)) {
        log("Can't find static runtime, unable to build: " + static_runtime);
        missing_assets = true;
      } else {
        files += static_runtime;
      }

      if (missing_assets) {
        exit(1);
      }

      fs::create_directories(paths.pathPackage);

      pathResources = paths.pathResourcesRelativeToUserBuild;

      auto p = Path {
        paths.pathResourcesRelativeToUserBuild /
        "AppxManifest.xml"
      };

      // internal, used in the manifest
      if (settings["meta_version"].size() > 0) {
        auto version = settings["meta_version"];
        auto winversion = split(version, '-')[0];

        settings["win_version"] = winversion + ".0";
      } else {
        settings["win_version"] = "0.0.0.0";
      }

      // internal, a path to win executable, used in the manifest
      settings["win_exe"] = executable.string();

      writeFile(p, tmpl(gWindowsAppManifest, settings));

      // TODO Copy the files into place
    }

    log("package prepared");

    auto pathResourcesRelativeToUserBuild = paths.pathResourcesRelativeToUserBuild;
    if (settings.count("build_script") != 0) {
      //
      // cd into the targetPath and run the user's build command,
      // pass it to the platform-specific directory where they
      // should send their build artifacts.
      //
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
          std::string prefix_ = "PREFIX=";
          prefix_ += prefix;
          setEnv(prefix_.c_str());
        #else
          setenv("PREFIX", prefix, 1);
        #endif
      }

      StringStream buildArgs;
      buildArgs << " " << pathResourcesRelativeToUserBuild.string();

      if (flagDebugMode) {
        buildArgs << " --debug=true";
      }

      if (flagBuildTest) {
        buildArgs << " --test=true";
      }

      auto process = new SSC::Process(
        settings["build_script"],
        buildArgs.str(),
        fs::current_path().string(),
        [](SSC::String const &out) { stdWrite(out, false); },
        [](SSC::String const &out) { stdWrite(out, true); }
      );

      process->open();
      process->wait();

      if (process->status != 0) {
        // TODO(trevnorris): Force non-windows to exit the process.
        log("build failed, exiting with code " + std::to_string(process->status));
        exit(process->status);
      }

      log("ran user build command");

      fs::current_path(oldCwd);
    }

    if (settings.count("build_copy") != 0) {
      Path pathInput = settings["build_copy"].size() > 0
        ? settings["build_copy"]
        : "src";

      auto paths = split(pathInput.string(), ';');
      for (const auto& p : paths) {
        auto mapping = split(p, '=');
        auto src = targetPath / trim(mapping[0]);
        auto dst = mapping.size() == 2
          ? pathResourcesRelativeToUserBuild / trim(mapping[1])
          : pathResourcesRelativeToUserBuild;

        if (!fs::exists(fs::status(src))) {
          log("WARNING: [build] copy entry '" + src.string() +  "' does not exist");
          continue;
        }

        if (!fs::exists(fs::status(dst.parent_path()))) {
          fs::create_directories(dst.parent_path());
        }

        fs::copy(
          src,
          dst,
          fs::copy_options::update_existing | fs::copy_options::recursive
        );
      }
    // @deprecated
    } else if (settings.count("build_input") != 0) {
      log("socket.ini: [build] input is deprecated, use [build] copy instead");

      if (settings.count("build_script") != 0) {
        log("socket.ini: Mixing [build] input and [build] script may lead to unexpected behavior");
      }

      Path pathInput = settings["build_input"].size() > 0
        ? targetPath / settings["build_input"]
        : targetPath / "src";
      fs::copy(
        pathInput,
        pathResourcesRelativeToUserBuild,
        fs::copy_options::update_existing | fs::copy_options::recursive
      );
    }

    for (const auto& tuple : settings) {
      if (!tuple.first.starts_with("build_copy-map_")) {
        continue;
      }

      auto key = replace(tuple.first, "build_copy-map_", "");
      auto value = tuple.second;

      auto src = Path { key };
      auto dst = tuple.second.size() > 0
        ? pathResourcesRelativeToUserBuild / value
        : pathResourcesRelativeToUserBuild;

      src = src.make_preferred();
      dst = dst.make_preferred();

      if (src.is_relative()) {
        src = targetPath / src;
      }

      src = fs::absolute(src);
      dst = fs::absolute(dst);

      if (!fs::exists(fs::status(src))) {
        log("WARNING: [copy-map] entry '" + src.string() +  "' does not exist");
        continue;
      }

      if (!fs::exists(fs::status(dst.parent_path()))) {
        fs::create_directories(dst.parent_path());
      }

      auto mappedSourceFile = (
        pathResourcesRelativeToUserBuild /
        fs::relative(src, targetPath)
      );

      if (fs::exists(fs::status(mappedSourceFile))) {
        fs::remove_all(mappedSourceFile);
      }

      fs::copy(
        src,
        dst,
        fs::copy_options::update_existing | fs::copy_options::recursive
      );
    }

    if (settings.count("build_copy_map") != 0) {
      auto copyMapFile = Path{settings["build_copy_map"]}.make_preferred();

      if (copyMapFile.is_relative()) {
        copyMapFile = targetPath / copyMapFile;
      }

      if (!fs::exists(fs::status(copyMapFile))) {
        log("WARNING: file specified in [build] copy_map does not exist");
      } else {
        auto copyMap = parseINI(tmpl(trim(readFile(copyMapFile)), settings));
        auto copyMapFileDirectory = fs::absolute(copyMapFile.parent_path());

        for (const auto& tuple : copyMap) {
          auto key = tuple.first;
          auto& value = tuple.second;

          if (key.starts_with("debug_")) {
            if (!flagDebugMode) continue;
            key = key.substr(6, key.size() - 6);
          }

          if (key.starts_with("prod_")) {
            if (flagDebugMode) continue;
            key = key.substr(5, key.size() - 5);
          }

          if (key.starts_with("production_")) {
            if (flagDebugMode) continue;
            key = key.substr(11, key.size() - 11);
          }

          auto src = Path { key };
          auto dst = tuple.second.size() > 0
            ? pathResourcesRelativeToUserBuild / value
            : pathResourcesRelativeToUserBuild;

          src = src.make_preferred();
          dst = dst.make_preferred();

          if (src.is_relative()) {
            src = copyMapFileDirectory / src;
          }

          src = fs::absolute(src);
          dst = fs::absolute(dst);

          if (!fs::exists(fs::status(src))) {
            log("WARNING: [build] copy_map entry '" + src.string() +  "' does not exist");
            continue;
          }

          if (!fs::exists(fs::status(dst.parent_path()))) {
            fs::create_directories(dst.parent_path());
          }

          auto mappedSourceFile = (
            pathResourcesRelativeToUserBuild /
            fs::relative(src, copyMapFileDirectory)
          );

          if (fs::exists(fs::status(mappedSourceFile))) {
            fs::remove_all(mappedSourceFile);
          }

          fs::copy(
            src,
            dst,
            fs::copy_options::update_existing | fs::copy_options::recursive
          );
        }
      }
    }

    auto SOCKET_HOME_API = getEnv("SOCKET_HOME_API");

    if (SOCKET_HOME_API.size() == 0) {
      SOCKET_HOME_API = trim(prefixFile("api"));
    }

    if (fs::exists(fs::status(SOCKET_HOME_API))) {
      fs::create_directories(pathResources);
      fs::copy(
        SOCKET_HOME_API,
        pathResources / "socket",
        fs::copy_options::update_existing | fs::copy_options::recursive
      );

      // XXX(@jwerle): 'node_modules/' sometimes can be found in the
      // SOCKET_HOME_API directory if distributed with npm. Handle all
      // `NODE_PATH` entries too (pedantic)
      auto nodePaths = parseStringList(getEnv("NODE_PATH"), { ':', ';' });
      nodePaths.push_back("node_modules");
      for (const auto& path : nodePaths) {
        fs::remove_all(pathResources / "socket" / path);
      }
    }

    if (flagBuildForIOS) {
      if (flagBuildForSimulator && settings["ios_simulator_device"].size() == 0) {
        log("ERROR: 'ios_simulator_device' option is empty");
        exit(1);
      }

      log("building for iOS");

      auto oldCwd = fs::current_path();
      auto pathToDist = oldCwd / paths.platformSpecificOutputPath;

      fs::create_directories(pathToDist);
      fs::current_path(pathToDist);

      //
      // Copy and or create the source files we need for the build.
      //
      fs::copy(trim(prefixFile("src/common.hh")), pathToDist);
      fs::copy(trim(prefixFile("src/init.cc")), pathToDist);

      auto pathBase = pathToDist / "Base.lproj";
      fs::create_directories(pathBase);

      writeFile(pathBase / "LaunchScreen.storyboard", gStoryboardLaunchScreen);
      // TODO allow the user to copy their own if they have one
      writeFile(pathToDist / "socket.entitlements", tmpl(gXcodeEntitlements, settings));

      //
      // For iOS we're going to bail early and let XCode infrastructure handle
      // building, signing, bundling, archiving, noterizing, and uploading.
      //
      StringStream archiveCommand;
      String destination = flagBuildForSimulator
        ? "platform=iOS Simulator,OS=latest,name=" + settings["ios_simulator_device"]
        : "generic/platform=iOS";
      String deviceType;

      // TODO: should be "iPhone Distribution: <name/provisioning specifier>"?
      if (settings["ios_codesign_identity"].size() == 0) {
        settings["ios_codesign_identity"] = "iPhone Distribution";
      }

      auto sup = String("archive");
      auto configuration = String(flagDebugMode ? "Debug" : "Release");

      if (!flagShouldPackage) {
        sup = "CONFIGURATION_BUILD_DIR=" + pathToDist.string();
      }

      archiveCommand
        << "xcodebuild"
        << " build " << sup
        << " -configuration " << configuration
        << " -scheme " << settings["build_name"]
        << " -destination '" << destination << "'";

      if (flagShouldPackage) {
        archiveCommand << " -archivePath build/" << settings["build_name"];
      }

      if (!flagCodeSign) {
        archiveCommand
          << " CODE_SIGN_IDENTITY=\"\""
          << " CODE_SIGNING_REQUIRED=\"NO\""
          << " CODE_SIGN_ENTITLEMENTS=\"\""
          << " CODE_SIGNING_ALLOWED=\"NO\"";
      }

      // log(archiveCommand.str().c_str());
      auto rArchive = exec(archiveCommand.str().c_str());

      if (rArchive.exitCode != 0) {
        auto const noDevice = rArchive.output.find("The requested device could not be found because no available devices matched the request.");
        if (noDevice != std::string::npos) {
          log("ERROR: simulator_device " + settings["ios_simulator_device"] + " from your socket.ini was not found");
          auto const rDevices = exec("xcrun simctl list devices available | grep -e \"  \"");
          log("available devices:\n" + rDevices.output);
          log("please update your socket.ini with a valid device or install Simulator runtime (https://developer.apple.com/documentation/xcode/installing-additional-simulator-runtimes)");
          exit(1);
        }
        log("ERROR: failed to archive project");
        log(rArchive.output);
        fs::current_path(oldCwd);
        exit(1);
      }

      log("created archive");

      if (flagBuildForSimulator && flagShouldRun) {
        String app = (settings["build_name"] + ".app");
        auto pathToApp = paths.platformSpecificOutputPath / app;
        runIOSSimulator(pathToApp, settings);
      }

      if (flagShouldPackage) {
        StringStream exportCommand;

        exportCommand
          << "xcodebuild"
          << " -exportArchive"
          << " -archivePath build/" << settings["build_name"] << ".xcarchive"
          << " -exportPath build/" << settings["build_name"] << ".ipa"
          << " -exportOptionsPlist " << (pathToDist / "exportOptions.plist").string();

        // log(exportCommand.str());
        auto rExport = exec(exportCommand.str().c_str());

        if (rExport.exitCode != 0) {
          log("ERROR: failed to export project");
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
      auto androidHome = getAndroidHome();

      StringStream sdkmanager;
      StringStream packages;
      StringStream gradlew;
      String ndkVersion = "25.0.8775105";
      String androidPlatform = "android-33";

      if (platform.unix) {
        gradlew
          << "ANDROID_HOME=" << androidHome << " ";
      }

      sdkmanager << settings["android_sdk_manager_path"];

      packages
        << " "
        << quote << "ndk;" << ndkVersion << quote << " "
        << quote << "platform-tools" << quote << " "
        << quote << "platforms;" << androidPlatform << quote << " "
        << quote << "emulator" << quote << " "
        << quote << "patcher;v4" << quote << " "
        << quote << "system-images;" << androidPlatform << ";google_apis;x86_64" << quote << " "
        << quote << "system-images;" << androidPlatform << ";google_apis;arm64-v8a" << quote << " ";

      sdkmanager
        << packages.str();

      if (debugEnv || verboseEnv) log(sdkmanager.str());
      if (std::system(sdkmanager.str().c_str()) != 0) {
        log("ERROR: failed to initialize Android SDK (sdkmanager)");
        exit(1);
      }

      if (!androidEnableStandardNdkBuild) {
        StringStream ndkBuild;
        StringStream ndkBuildArgs;
        StringStream ndkTest;
        
        ndkBuild << "ndk-build" << (platform.win ? ".cmd" : "");
        ndkTest << ndkBuild.str() << " --version >" << (!platform.win ? "/dev/null" : "NUL") << " 2>&1";

        if (debugEnv || verboseEnv) log(ndkTest.str());
        if (std::system(ndkTest.str().c_str()) != 0) {
            ndkBuild.str("");
            ndkBuild << androidHome << slash << "ndk" << slash <<  ndkVersion << slash << "ndk-build" << (platform.win ? ".cmd" : "");

            
            ndkTest.str("");
            ndkTest
              << ndkBuild.str() << " --version >" << (!platform.win ? "/dev/null" : "NUL") << " 2>&1";

            if (debugEnv || verboseEnv) log(ndkTest.str());
            if (std::system(ndkTest.str().c_str()) != 0) {
              StringStream ndkError;
              ndkError 
                << "ndk not in path or ANDROID_HOME at "
                << ndkBuild.str();
              log(ndkError.str());
              exit(1);
            }
        }

        // TODO(mribbons): Cache binaries, hash based on source contents. Copy if cache matches rather than building.
        // TODO(mribbons): Expand cache system to other target platforms

        auto output = paths.platformSpecificOutputPath;
        // auto app = output / "app";
        auto src = app / "src";
        auto _main = src / "main";
        auto app_mk = _main / "jni" / "Application.mk";
        auto jni = _main / "jni";
        auto jniLibs = _main / "jniLibs";
        auto libs = _main / "libs";
        auto obj = _main / "obj";

        if (!fs::exists(jniLibs)) {

          if (fs::exists(obj)) {
            fs::remove_all(obj);
          }

          // TODO(mribbons) - Copy specific abis
          fs::create_directories(libs);
          for (auto const& dir_entry : fs::directory_iterator(prefixFile() + "lib")) {
            if (dir_entry.is_directory() && dir_entry.path().stem().string().find("-android") != String::npos) {
              auto dest = libs / replace(dir_entry.path().stem().string(), "-android", "");
              try {
                if (debugEnv) log("copy android lib: "+ dir_entry.path().string() + " => " + dest.string());
                fs::copy(
                  dir_entry.path(),
                  dest,
                  fs::copy_options::overwrite_existing | fs::copy_options::recursive
                );
              } catch (fs::filesystem_error &e) {
                std::cerr << "Unable to copy android lib: " << fs::exists(dest) << ": " << e.what() << std::endl;
                throw;
              }
            }
          }

          fs::create_directories(jniLibs);

          ndkBuildArgs
            << ndkBuild.str()
            << " -j"
            << " NDK_PROJECT_PATH=" << _main
            << " NDK_APPLICATION_MK=" << app_mk
            << (flagDebugMode ? " NDK_DEBUG=1" : "")
            << " APP_PLATFORM=" << androidPlatform
            << " NDK_LIBS_OUT=" << jniLibs
          ;

          if (!(debugEnv || verboseEnv)) ndkBuildArgs << " >" << (!platform.win ? "/dev/null" : "NUL") << " 2>&1";

          if (debugEnv || verboseEnv) log(ndkBuildArgs.str());
          if (std::system(ndkBuildArgs.str().c_str()) != 0) {
            log(ndkBuildArgs.str());
            log("ERROR: ndk build failed.");
            exit(1);
          }
        }
      }

      // just build for CI
      if (getEnv("SSC_CI").size() > 0) {
        StringStream gradlew;
        gradlew << localDirPrefix << "gradlew build";

        if (std::system(gradlew.str().c_str()) != 0) {
          log("ERROR: failed to invoke `gradlew build` command");
          exit(1);
        }

        exit(0);
      }

      String bundle = flagDebugMode ?
        localDirPrefix + "gradlew :app:bundleDebug --warning-mode all" :
        localDirPrefix + "gradlew :app:bundle";

      if (debugEnv || verboseEnv) log(bundle);
      if (std::system(bundle.c_str()) != 0) {
        log("ERROR: failed to invoke " + bundle + " command.");
        exit(1);
      }

      // clear stream
      gradlew.str("");
      gradlew
        << localDirPrefix
        << "gradlew assemble";

      if (debugEnv || verboseEnv) log(gradlew.str());
      if (std::system(gradlew.str().c_str()) != 0) {
        log("ERROR: failed to invoke `gradlew assemble` command");
        exit(1);
      }

      if (flagBuildForAndroidEmulator) {
        StringStream avdmanager;
        String package = quote + "system-images;" + androidPlatform + ";google_apis;" + replace(platform.arch, "arm64", "arm64-v8a") + quote;

        if (!platform.win) {
          if (std::system("avdmanager list 2>&1 >/dev/null") != 0) {
            avdmanager << androidHome << "/cmdline-tools/latest/bin/";
          }
        } else
          avdmanager << androidHome << "\\cmdline-tools\\latest\\bin\\";

        avdmanager
          << "avdmanager create avd "
          << "--device 5 "
          << "--force "
          << "--name SSCAVD "
          << ("--abi google_apis/" + replace(platform.arch, "arm64", "arm64-v8a")) << " "
          << "--package " << package;

        if (debugEnv || verboseEnv) log(avdmanager.str());
        if (std::system(avdmanager.str().c_str()) != 0) {
          log("ERROR: failed to Android Virtual Device (avdmanager)");
          exit(1);
        }
      }

      if (flagShouldRun) {
        // the emulator must be running on device SSCAVD for now
        StringStream adbShellStart;
        StringStream adb;

        if (!platform.win) {
          adb << androidHome << "/platform-tools/";
        } else {
          adb << androidHome << "\\platform-tools\\";
        }

        if (debugEnv || verboseEnv) log((adb.str() + (" --version > ") + SSC::String((!platform.win) ? "/dev/null" : "NUL") + (" 2>&1")));
        if (!std::system((adb.str() + (" --version > ") + SSC::String((!platform.win) ? "/dev/null" : "NUL") + (" 2>&1")).c_str())) {
          log("Warn: Failed to locate adb at " + adb.str());
        }

        adb << "adb ";

        adbShellStart << adb.str();
        adb << "install ";

        if (flagDebugMode) {
          adb << (app / "build" / "outputs" / "apk" / "dev" / "debug" / "app-dev-debug.apk").string();
        } else {
          adb << (app / "build" / "outputs" / "apk" / "live" / "debug" / "app-live-debug.apk").string();
        }

        if (debugEnv || verboseEnv) log(adb.str());
        if (std::system(adb.str().c_str()) != 0) {
          log("ERROR: failed to install APK to Android Emulator (adb)");
          exit(1);
        }

        if (debugEnv || verboseEnv) log(adbShellStart.str());
        adbShellStart << "shell am start -n " << settings["meta_bundle_identifier"] << "/" << settings["meta_bundle_identifier"] << settings["android_main_activity"];
        if (std::system(adbShellStart.str().c_str()) != 0) {
          log("Failed to run app on emulator.");
          exit(1);
        }
      }

      exit(0);
    }

    if (flagRunUserBuildOnly == false) {
      StringStream compileCommand;

      auto extraFlags = flagDebugMode
        ? settings.count("debug_flags") ? settings["debug_flags"] : ""
        : settings.count("build_flags") ? settings["build_flags"] : "";

      auto quote = "";
      if (platform.win && getEnv("CXX").find(" ") != SSC::String::npos) {
        quote = "\"";
      }

      // windows / spaces in bin path - https://stackoverflow.com/a/27976653/3739540
      compileCommand
        << quote // win32 - quote the entire command
        << quote // win32 - quote the binary path
        << getEnv("CXX")
        << quote // win32 - quote the binary path
        << " " << files
        << " " << flags
        << " " << extraFlags
        << " -o " << binaryPath.string()
        << " -DIOS=" << (flagBuildForIOS ? 1 : 0)
        << " -DANDROID=" << (flagBuildForAndroid ? 1 : 0)
        << " -DDEBUG=" << (flagDebugMode ? 1 : 0)
        << " -DHOST=" << devHost
        << " -DPORT=" << devPort
        << " -DSSC_SETTINGS=\"" << encodeURIComponent(_settings) << "\""
        << " -DSSC_VERSION=" << SSC::VERSION_STRING
        << " -DSSC_VERSION_HASH=" << SSC::VERSION_HASH_STRING
        << quote // win32 - quote the entire command
      ;

      if (getEnv("DEBUG") == "1" || getEnv("VERBOSE") == "1")
        log(compileCommand.str());

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
      Path pathSymLinks = {
        paths.pathPackage /
        "usr" /
        "local" /
        "bin"
      };

      auto linuxExecPath = Path {
        Path("/opt") /
        settings["build_name"] /
        settings["build_name"]
      };

      fs::create_directories(pathSymLinks);
      fs::create_symlink(
        linuxExecPath,
        pathSymLinks / settings["build_name"]
      );

      StringStream archiveCommand;

      archiveCommand
        << "dpkg-deb --build --root-owner-group "
        << paths.pathPackage.string()
        << " "
        << (paths.platformSpecificOutputPath).string();

      if (debugEnv || verboseEnv) log(archiveCommand.str());
      auto r = std::system(archiveCommand.str().c_str());

      if (r != 0) {
        log("ERROR: failed to create deb package");
        exit(1);
      }
    }

    //
    // MacOS Stripping
    //
    if (platform.mac) {
      StringStream stripCommand;

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
      StringStream signCommand;
      String entitlements = "";

      if (settings.count("entitlements") == 1) {
        // entitlements = " --entitlements " + (targetPath / settings["entitlements"]).string();
      }

      if (settings.count("mac_sign") == 0) {
        log("'mac_sign' key/value is required");
        exit(1);
      }

      StringStream commonFlags;

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
          String prefix = (i > 0) ? ";" : "";

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
      StringStream zipCommand;
      auto ext = ".zip";
      auto pathToBuild = paths.platformSpecificOutputPath / "build";

      pathToArchive = paths.platformSpecificOutputPath / (settings["build_name"] + ext);

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
        log("ERROR: failed to create zip for notarization");
        exit(1);
      }

      log("created package artifact");
    }

    //
    // MacOS Notarization
    // ---
    //
    if (flagShouldNotarize && platform.mac) {
      StringStream notarizeCommand;
      String username = getEnv("APPLE_ID");
      String password = getEnv("APPLE_ID_PASSWORD");

      notarizeCommand
        << "xcrun"
        << " altool"
        << " --notarize-app"
        << " --username \"" << username << "\""
        << " --password \"" << password << "\""
        << " --primary-bundle-id \"" << settings["meta_bundle_identifier"] << "\""
        << " --file \"" << pathToArchive.string() << "\""
      ;

      auto r = exec(notarizeCommand.str().c_str());

      if (r.exitCode != 0) {
        log("Unable to notarize");
        exit(r.exitCode);
      }

      std::regex re(R"(\nRequestUUID = (.+?)\n)");
      std::smatch match;
      String uuid;

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

        msleep(1024 * 6);
        StringStream notarizeStatusCommand;

        notarizeStatusCommand
          << "xcrun"
          << " altool"
          << " --notarization-info " << uuid
          << " -u " << username
          << " -p " << password;

        auto r = exec(notarizeStatusCommand.str().c_str());

        std::regex re(R"(\n *Status: (.+?)\n)");
        std::smatch match;
        String status;

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

          StringStream notarizeHistoryCommand;

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

      WString appx(SSC::StringToWString(paths.pathPackage.string()) + L".appx");

      HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

      if (SUCCEEDED(hr)) {
        IAppxPackageWriter* packageWriter = NULL;

        hr = GetPackageWriter(appx.c_str(), &packageWriter);

        std::function<void(LPCWSTR, Path)> addFiles = [&](auto basePath, auto last) {
          for (const auto & entry : fs::directory_iterator(basePath)) {
            auto p = entry.path().filename().string();

            LPWSTR mime = 0;
            FindMimeFromData(NULL, entry.path().c_str(), NULL, 0, NULL, 0, &mime, 0);

            if (p.find("AppxManifest.xml") == 0) {
              continue;
            }

            if (fs::is_directory(entry.path())) {
              addFiles(entry.path().c_str(), Path { last / entry.path().filename() });
              continue;
            }

            auto composite = (Path { last / entry.path().filename() });

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

        addFiles(paths.pathPackage.c_str(), Path {});

        IStream* manifestStream = NULL;

        if (SUCCEEDED(hr)) {
          auto p = Path({ paths.pathPackage / "AppxManifest.xml" });

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
        String msg = WStringToString( err.ErrorMessage() );
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
      auto sdkRoot = Path("C:\\Program Files (x86)\\Windows Kits\\10\\bin");
      auto pathToSignTool = getEnv("SIGNTOOL");

      if (pathToSignTool.size() == 0) {
        // TODO assumes the last dir that contains dot. posix doesnt guarantee
        // order, maybe windows does, but this should probably be smarter.
        for (const auto& entry : fs::directory_iterator(sdkRoot)) {
          auto p = entry.path().string();
          if (p.find(".") != -1) pathToSignTool = p;
        }

        pathToSignTool = Path {
          Path(pathToSignTool) /
          "x64" /
          "signtool.exe"
        }.string();
      }

      if (pathToSignTool.size() == 0) {
        log("WARNING: Can't find windows 10 SDK, assuming signtool.exe is in the path.");
        pathToSignTool = "signtool.exe";
      }

      StringStream signCommand;
      String password = getEnv("CSC_KEY_PASSWORD");

      if (password.size() == 0) {
        log("ERROR: Environment variable 'CSC_KEY_PASSWORD' is empty!");
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

  createSubcommand("run", { "--platform", "--prod", "--test",  "--headless" }, true, [&](const std::span<const char *>& options) -> void {
    String argvForward = "";
    bool isIosSimulator = false;
    bool flagHeadless = false;
    bool flagTest = false;
    String targetPlatform = "";
    String testFile = "";

    for (auto const& option : options) {
      if (is(option, "--test")) {
        flagTest = true;
      }
      const auto testFileTmp = optionValue("run", option, "--test");
      if (testFileTmp.size() > 0) {
        flagTest = true;
        testFile = testFileTmp;
        argvForward += " " + String(option);
      }

      if (is(option, "--headless") || is(rc["run_headless"], "true")) {
        argvForward += " --headless";
        flagHeadless = true;
      }

      if (targetPlatform.size() == 0) {
        targetPlatform = optionValue("run", option, "--platform");
        if (targetPlatform.size() > 0) {
          if (targetPlatform == "ios-simulator") {
            isIosSimulator = true;
          } else {
            log("Unknown platform: " + targetPlatform);
            exit(1);
          }
        }
      }
    }

    if (flagTest && testFile.size() == 0) {
      log("ERROR: --test value is required.");
      exit(1);
    }

    targetPlatform = targetPlatform.size() > 0 ? targetPlatform : platform.os;
    Paths paths = getPaths(targetPlatform);

    if (isIosSimulator) {
      String app = (settings["build_name"] + ".app");
      auto pathToApp = paths.platformSpecificOutputPath / app;
      runIOSSimulator(pathToApp, settings);
    } else {
      auto executable = Path(settings["build_name"] + (platform.win ? ".exe" : ""));
      auto exitCode = runApp(paths.pathBin / executable, argvForward, flagHeadless);
      exit(exitCode);
    }
  });

  createSubcommand("setup", { "--platform", "--yes", "-y" }, false, [&](const std::span<const char *>& options) -> void {
    auto help = false;
    auto yes = false;
    String yesArg;

    String targetPlatform;
    auto targetAndroid = false;
    auto targetLinux = false;
    auto targetWindows = false;

    for (auto const arg : options) {
      if (is(arg, "-h") || is(arg, "--help")) {
        help = true;
      }

      auto platform = optionValue("setup", arg, "--platform");
      if (platform.size() > 0) {
        targetPlatform = platform;
      }

      if (is(arg, "-y") || is(arg, "--yes")) {
        yes = true;
      }

      if (help) {
        printHelp("setup");
        exit(0);
      }
    }

    // Note that multiple --platforms aren't supported by createSubcommand()
    if (is(targetPlatform, "android")) {
      targetAndroid = true;
    } else if (is(targetPlatform, "windows")) {
      targetWindows = true;
    } else if (is(targetPlatform, "linux")) {
      targetLinux = true;
    } else if (targetPlatform.size() > 0) {
      printHelp("setup");
      exit(1);
    }

    if (targetPlatform.size() == 0) {
      if (platform.win) {
        targetWindows = true;
      } else if (platform.linux) {
        targetLinux = true;
      }
    }

    if (!platform.win && targetWindows) {
      log("ERROR: Windows build dependencies can only be installed on Windows.");
      exit(1);
    }

    if (!platform.linux && targetLinux) {
      log("ERROR: Linux build dependencies can only be installed on Linux.");
      exit(1);
    }

    String argument;
    Path script;
    String scriptHost;

    if (platform.win) {
      scriptHost = "powershell.exe";
      script = prefixFile("bin\\install.ps1");
      if (targetPlatform.size() > 0) {
        argument = "-fte:" + targetPlatform;
      } else {
        argument = "-fte:windows";
      }
      yesArg = yes ? "-yesdeps" : "";
    } else if (platform.linux || platform.mac) {
      scriptHost = "bash";
      script = prefixFile("./bin/functions.sh");
      argument = "--fte " + targetPlatform;
    } else {
      argument = "--" + targetPlatform + "-fte";
      if (targetAndroid) {
        script = prefixFile("./bin/android-functions.sh");
      }
      yesArg = yes ? "--yes-deps" : "";
    }

    script = Path(script.string().substr(0, script.string().size()-1));

    if (!fs::exists(script)) {
      log("ERROR: Install script not found: '" + script.string() + "'");
      exit(1);
    }

    fs::current_path(prefixFile());

    if (targetPlatform.size() == 0) {
      if (platform.win) {
        targetPlatform = "windows";
      } else if (platform.linux) {
        targetPlatform = "linux";
      } else if (platform.mac) {
        targetPlatform = "darwin";
      }
    }

    log("Running setup for platform '" + targetPlatform + "' in " + "SOCKET_HOME (" + prefixFile() + ")");
    String command = scriptHost + " \"" + script.string() + "\" " + argument + " " + yesArg;
    auto r = std::system(command.c_str());

    exit(r);
  });

  createSubcommand("env", { }, true, [&](const std::span<const char *>& options) -> void {
    auto envs = Map();

    envs["DEBUG"] = getEnv("DEBUG");
    envs["VERBOSE"] = getEnv("VERBOSE");

    // runtime variables
    envs["SOCKET_HOME"] = getSocketHome(false);
    envs["SOCKET_HOME_API"] = getEnv("SOCKET_HOME_API");

    // platform OS variables
    envs["PWD"] = getEnv("PWD");
    envs["HOME"] = getEnv("HOME");
    envs["LANG"] = getEnv("LANG");
    envs["USER"] = getEnv("USER");
    envs["SHELL"] = getEnv("SHELL");
    envs["HOMEPATH"] = getEnv("HOMEPATH");
    envs["LOCALAPPDATA"] = getEnv("LOCALAPPDATA");
    envs["XDG_DATA_HOME"] = getEnv("XDG_DATA_HOME");

    // compiler variables
    envs["CXX"] = getEnv("CXX");
    envs["PREFIX"] = getEnv("PREFIX");
    envs["CXXFLAGS"] = getEnv("CXXFLAGS");

    // locale variables
    envs["LC_ALL"] = getEnv("LC_ALL");
    envs["LC_CTYPE"] = getEnv("LC_CTYPE");
    envs["LC_TERMINAL"] = getEnv("LC_TERMINAL");
    envs["LC_TERMINAL_VERSION"] = getEnv("LC_TERMINAL_VERSION");

    // platform dependency variables
    envs["JAVA_HOME"] = getEnv("JAVA_HOME");
    envs["GRADLE_HOME"] = getEnv("GRADLE_HOME");
    envs["ANDROID_HOME"] = getAndroidHome();
    envs["ANDROID_SUPPORTED_ABIS"] = getEnv("ANDROID_SUPPORTED_ABIS");

    // apple specific platform variables
    envs["APPLE_ID"] = getEnv("APPLE_ID");
    envs["APPLE_ID_PASSWORD"] = getEnv("APPLE_ID");

    // windows specific platform variables
    envs["SIGNTOOL"] = getEnv("SIGNTOOL");
    envs["WIN_DEBUG_LIBS"] = getEnv("WIN_DEBUG_LIBS");
    envs["CSC_KEY_PASSWORD"] = getEnv("CSC_KEY_PASSWORD");

    // ssc variables
    envs["SSC_CI"] = getEnv("SSC_CI");
    envs["SSC_RC"] = getEnv("SSC_RC");
    envs["SSC_RC_FILENAME"] = getEnv("SSC_RC_FILENAME");
    envs["SSC_ENV_FILENAME"] = getEnv("SSC_ENV_FILENAME");

    if (envs["SOCKET_HOME_API"].size() == 0) {
      envs["SOCKET_HOME_API"] = trim(prefixFile("api"));
    }

    // gather all evironemnt variables relevant to project in the `socket.ini`
    // file, which may overload the variables above
    for (const auto& entry : settings) {
      if (entry.first.starts_with("env_")) {
        auto key = entry.first.substr(4, entry.first.size() - 4);
        auto value = trim(entry.second);

        if (value.size() == 0) {
          value = trim(getEnv(key));
        }

        envs[key] = value;
      } else if (entry.first == "env") {
        for (const auto& key : parseStringList(entry.second)) {
          auto value = trim(getEnv(key));
          envs[key] = value;
        }
      }
    }

    // gather all evironemnt variables relevant to local `.sscrc` files
    // file, which may overload the variables above
    for (const auto& entry : rc) {
      if (entry.first.starts_with("env_")) {
        auto key = entry.first.substr(4, entry.first.size() - 4);
        auto value = trim(entry.second);

        if (value.size() == 0) {
          value = trim(getEnv(key));
        }

        envs[key] = value;
      } else if (entry.first == "env") {
        for (const auto& key : parseStringList(entry.second)) {
          auto value = trim(getEnv(key));
          envs[key] = value;
        }
      }
    }

    // print all environment variables that have values
    for (const auto& entry : envs) {
      auto& key = entry.first;
      auto value = trim(entry.second);

      if (value.size() == 0) {
        value = trim(getEnv(key));
      }

      if (value.size() > 0) {
        std::cout << key << "=" << value << std::endl;
      }
    }

    exit(0);
  });

  log("subcommand 'ssc " + String(subcommand) + "' is not supported.");
  printHelp("ssc");
  exit(1);
}
