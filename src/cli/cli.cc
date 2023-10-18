#if !defined(_WIN32)
#include <unistd.h>
#else
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
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <span>
#include <unordered_set>

#include "../core/core.hh"
#include "../extension/extension.hh"
#include "../process/process.hh"

#include "templates.hh"
#include "cli.hh"

#ifndef CMD_RUNNER
#define CMD_RUNNER
#endif

#ifndef SSC_BUILD_TIME
#define SSC_BUILD_TIME 0
#endif

using namespace SSC;
using namespace std::chrono;

const auto DEFAULT_SSC_RC_FILENAME = String(".sscrc");
const auto DEFAULT_SSC_ENV_FILENAME = String(".ssc.env");

String _settings;
Path targetPath;
Map settings;
Map rc;

auto start = system_clock::now();

bool flagDebugMode = true;
bool flagVerboseMode = true;
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

inline Map& extendMap (Map& dst, const Map& src) {
  for (const auto& tuple : src) {
    dst[tuple.first] = tuple.second;
  }
  return dst;
}

inline String readFile (fs::path path) {
  if (fs::is_directory(path)) {
    log("WARNING: trying to read a directory as a file: " + path.string());
    return "";
  }

  std::ifstream stream(path.c_str());
  String content;
  auto buffer = std::istreambuf_iterator<char>(stream);
  auto end = std::istreambuf_iterator<char>();
  content.assign(buffer, end);
  stream.close();
  return content;
}

inline void writeFile (fs::path path, String s) {
  std::ofstream stream(path.string());
  stream << s;
  stream.close();
}

inline void appendFile (fs::path path, String s) {
  std::ofstream stream;
  stream.open(path.string(), std::ios_base::app);
  stream << s;
  stream.close();
}

bool equal (const String& s1, const String& s2) {
  return s1.compare(s2) == 0;
};

const Map SSC::getUserConfig () {
  return settings;
}

bool SSC::isDebugEnabled () {
  return DEBUG == 1;
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

String getSocketHome (bool verbose) {
  static String XDG_DATA_HOME = Env::get("XDG_DATA_HOME");
  static String LOCALAPPDATA = Env::get("LOCALAPPDATA");
  static String SOCKET_HOME = Env::get("SOCKET_HOME");
  static String HOME = Env::get("HOME");
  static const bool SSC_DEBUG = (
    Env::get("SSC_DEBUG").size() > 0 ||
    Env::get("DEBUG").size() > 0
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
    Env::set((String("SOCKET_HOME=") + socketHome).c_str());
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

String getAndroidHome () {
  static auto androidHome = Env::get("ANDROID_HOME");
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
      androidHome = Env::get("HOME") + "/Library/Android/sdk";
    } else if (platform.unix) {
      androidHome = Env::get("HOME") + "/android";
    } else if (platform.win) {
      // TODO
    }
  }

  if (androidHome.size() > 0) {
    #ifdef _WIN32
    Env::set((String("ANDROID_HOME=") + androidHome).c_str());
    #else
    setenv("ANDROID_HOME", androidHome.c_str(), 1);
    #endif

    static const bool SSC_DEBUG = (
      Env::get("SSC_DEBUG").size() > 0 ||
      Env::get("DEBUG").size() > 0
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

#if defined(__APPLE__)
void pollOSLogStream (bool isForDesktop, String bundleIdentifier, int processIdentifier) {
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
  auto pid = std::to_string(processIdentifier);
  auto bid = bundleIdentifier.c_str();
  queryStream
    << "("
    << (isForDesktop
        ? "category == 'socket.runtime.desktop'"
        : "category == 'socket.runtime.mobile'")
    << " OR category == 'socket.runtime.debug'"
    << ") AND ";

    if (processIdentifier > 0) {
      queryStream << "processIdentifier == " << pid << " AND ";
    }

  queryStream << "subsystem == '" << bid << "'";
  // log store query and predicate for filtering logs based on the currently
  // running application that was just launched and those of a subsystem
  // directly related to the application's bundle identifier which allows us
  // to just get logs that came from the application (not foundation/cocoa/webkit)
  const auto query = [NSString stringWithUTF8String: queryStream.str().c_str()];
  const auto predicate = [NSPredicate predicateWithFormat: query];

  // use the launch date as the initial marker
  const auto now = [NSDate now];
  // and offset it by 1 second in the past as the initial position in the eumeration
  auto offset = [now dateByAddingTimeInterval: -1];

  // tracks the latest log entry date so we ignore older ones
  NSDate* latest = nil;
  NSError* error = nil;

  int pollsAfterTermination = 16;
  int backoffIndex = 0;

  // lucas series of backoffs
  static int backoffs[] = {
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

  if (processIdentifier > 0) {
    dispatch_async(queue, ^{
      while (kill(processIdentifier, 0) == 0) {
        msleep(256);
      }
    });
  }

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
      if (processIdentifier > 0) {
        continue;
      }
    }

    // this is may be set to `true` from a `SIGUSR1` signal
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
          (processIdentifier == 0 || entry.processIdentifier == processIdentifier)
        ) {
          // visit latest log
          if (!latest || [latest compare: entry.date] == NSOrderedAscending) {
            auto message = entry.composedMessage.UTF8String;

            // the OSLogStore may redact log messages the user does not
            // have access to, filter them out
            if (String(message) != "<private>") {
              if (String(message).starts_with("__EXIT_SIGNAL__")) {
                if (appStatus == -1) {
                  appStatus = std::stoi(replace(String(message), "__EXIT_SIGNAL__=", ""));
                }
              } else if (
                entry.level == OSLogEntryLogLevelDebug ||
                entry.level == OSLogEntryLogLevelError ||
                entry.level == OSLogEntryLogLevelFault
              ) {
                if (entry.level == OSLogEntryLogLevelDebug) {
                  if (flagDebugMode) {
                    std::cerr << message << std::endl;
                  }
                } else {
                  std::cerr << message << std::endl;
                }
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
}
#endif

void handleBuildPhaseForUserScript (
  const Map settings,
  const String& targetPlatform,
  const Path pathResourcesRelativeToUserBuild,
  const Path& cwd,
  const String& additionalArgs,
  bool performAfterLifeCycle
) {
  do {
    char prefix[4096] = {0};
    std::memcpy(
    prefix,
    pathResourcesRelativeToUserBuild.string().c_str(),
    pathResourcesRelativeToUserBuild.string().size()
    );

    // @TODO(jwerle): use `Env::set()` if #148 is closed
#if _WIN32
    String prefix_ = "PREFIX=";
    prefix_ += prefix;
    Env::set(prefix_.c_str());
#else
    setenv("PREFIX", prefix, 1);
#endif
  } while (0);

  StringStream buildArgs;
  buildArgs << " " << pathResourcesRelativeToUserBuild.string();
  buildArgs << " " << additionalArgs;

  if (settings.contains("build_script") && settings.at("build_script").size() > 0) {
    auto scriptArgs = buildArgs.str();
    auto buildScript = settings.at("build_script");

    // Windows CreateProcess() won't work if the script has an extension other than exe (say .cmd or .bat)
    // cmd.exe can handle this translation
    if (platform.win) {
      scriptArgs =  " /c \"" + buildScript  + " " + scriptArgs + "\"";
      buildScript = "cmd.exe";
    }

    auto process = new SSC::Process(
      buildScript,
      scriptArgs,
      (cwd / targetPath).string(),
      [](SSC::String const &out) { IO::write(out, false); },
      [](SSC::String const &out) { IO::write(out, true); }
    );

    process->open();
    process->wait();

    if (process->status != 0) {
      // TODO(trevnorris): Force non-windows to exit the process.
      log("build failed, exiting with code " + std::to_string(process->status));
      exit(process->status);
    }

    log("ran user build command");
  }

  // runs async, does not block
  if (performAfterLifeCycle && settings.contains("build_script_after") && settings.at("build_script_after").size() > 0) {
    auto scriptArgs = buildArgs.str();
    auto buildScript = settings.at("build_script_after");

    // Windows CreateProcess() won't work if the script has an extension other than exe (say .cmd or .bat)
    // cmd.exe can handle this translation
    if (platform.win) {
      scriptArgs =  " /c \"" + buildScript  + " " + scriptArgs + "\"";
      buildScript = "cmd.exe";
    }

    SSC::Process* process = new SSC::Process(
      buildScript,
      scriptArgs,
      (cwd / targetPath).string(),
      [](SSC::String const &out) { IO::write(out, false); },
      [](SSC::String const &out) { IO::write(out, true); },
      [process](const auto status) {
        const auto exitCode = std::atoi(status.c_str());
        if (exitCode != 0) {
          log("build after script failed with code: " + status);
        }
      }
    );

    process->open();
  }
}

Vector<Path> handleBuildPhaseForCopyMappedFiles (
  const Map settings,
  const String& targetPlatform,
  const Path pathResourcesRelativeToUserBuild
) {
  Vector<Path> copyMapFiles;

  if (settings.contains("build_copy")) {
    const Path pathInput = settings.contains("build_copy") && settings.at("build_copy").size() > 0
      ? settings.at("build_copy")
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

    auto mappedSourceFile = fs::absolute(
      pathResourcesRelativeToUserBuild /
      fs::relative(src, targetPath)
    );

    if (
      !mappedSourceFile.string().ends_with(".") &&
      pathResourcesRelativeToUserBuild.compare(mappedSourceFile) != 0
     ) {
      if (fs::exists(fs::status(mappedSourceFile))) {
        fs::remove_all(mappedSourceFile);
      }
    }

    fs::copy(
      src,
      dst,
      fs::copy_options::update_existing | fs::copy_options::recursive
    );
  }

  // copy map file for all platforms
  if (settings.contains("build_copy_map")) {
    auto copyMapFile = Path(settings.at("build_copy_map")).make_preferred();

    if (copyMapFile.is_relative()) {
      copyMapFile = targetPath / copyMapFile;
    }

    copyMapFiles.push_back(copyMapFile);
  }

  // copy map file for target platform
  if (
    targetPlatform.starts_with("ios") &&
    settings.contains("build_ios_copy_map")
   ) {
    auto copyMapFile = Path(settings.at("build_ios_copy_map")).make_preferred();

    if (copyMapFile.is_relative()) {
      copyMapFile = targetPath / copyMapFile;
    }

    copyMapFiles.push_back(copyMapFile);
  }

  if (
    targetPlatform.starts_with("android") &&
    settings.contains("build_android_copy_map")
   ) {
    auto copyMapFile = Path(settings.at("build_android_copy_map")).make_preferred();

    if (copyMapFile.is_relative()) {
      copyMapFile = targetPath / copyMapFile;
    }

    copyMapFiles.push_back(copyMapFile);
  }

  if (settings.contains("build_" + platform.os +  "_copy_map")) {
    auto copyMapFile = Path(settings.at("build_" + platform.os +  "_copy_map")).make_preferred();

    if (copyMapFile.is_relative()) {
      copyMapFile = targetPath / copyMapFile;
    }

    copyMapFiles.push_back(copyMapFile);
  }

  for (const auto& copyMapFile : copyMapFiles) {
    if (!fs::exists(fs::status(copyMapFile)) || !fs::is_regular_file(copyMapFile)) {
      log("WARNING: file specified in [build] copy_map does not exist");
    } else {
      auto copyMap = INI::parse(tmpl(trim(readFile(copyMapFile)), settings));
      auto copyMapFileDirectory = fs::absolute(copyMapFile.parent_path());

      for (const auto& tuple : copyMap) {
        auto key = tuple.first;
        auto& value = tuple.second;

        if (key.starts_with("win_")) {
          if (!platform.win) continue;
          key = key.substr(4, key.size() - 4);
        }

        if (key.starts_with("mac_")) {
          if (!platform.mac) continue;
          key = key.substr(4, key.size() - 4);
        }

        if (key.starts_with("ios_")) {
          if (!platform.mac || !targetPlatform.starts_with("ios")) continue;
          key = key.substr(4, key.size() - 4);
        }

        if (key.starts_with("linux_")) {
          if (!platform.linux) continue;
          key = key.substr(6, key.size() - 6);
        }

        if (key.starts_with("android_")) {
          if (!targetPlatform.starts_with("android")) continue;
          key = key.substr(8, key.size() - 8);
        }

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

        if (
          !mappedSourceFile.string().ends_with(".") &&
          pathResourcesRelativeToUserBuild.compare(mappedSourceFile) != 0
         ) {
          if (fs::exists(fs::status(mappedSourceFile))) {
            fs::remove_all(mappedSourceFile);
          }
        }

        fs::copy(
          src,
          dst,
          fs::copy_options::update_existing | fs::copy_options::recursive
        );
      }
    }
  }

  return copyMapFiles;
}

void signalHandler (int signal) {
  if (appPid == 0 && signal == SIGTERM) {
    exit(1);
    return;
  }

  if (appPid == 0 && signal == SIGINT) {
    exit(signal);
    return;
  }

#if !defined(_WIN32)
  if (signal == SIGUSR1) {
  #if defined(__APPLE__)
    checkLogStore = true;
  #endif
    return;
  }

  if (signal == SIGUSR2) {
    exit(0);
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
    log("Executable not found at " + cmd);
    std::cout << "Try running `ssc build` first." << std::endl;
    return 1;
  }

  auto runner = trim(String(CONVERT_TO_STRING(CMD_RUNNER)));
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
      auto envValue = Env::get(cleanKey.c_str());
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

      pollOSLogStream(
        true,
        String(bundle.bundleIdentifier.UTF8String),
        app.processIdentifier
      );
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
    log("ERROR: [ios] simulator_device option is empty");
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
    << " simctl launch --console --terminate-running-process booted"
    << " " + settings["meta_bundle_identifier"];

  Env::set("SIMCTL_CHILD_SSC_CLI_PID", std::to_string(getpid()));
  log("launching the app in simulator");

  exit(std::system(launchAppCommand.str().c_str()));
  #endif
}

struct AndroidCliState {
  StringStream adb;
  SSC::String androidHome;
  // android, android-emulator
  SSC::String targetPlatform;
  // android-34 or current platform number
  SSC::String platform;
  StringStream avdmanager;
  bool sscAvdExists = false;
  bool emulatorRunning = false;
  StringStream emulator;
  SSC::Process* androidEmulatorProcess;
  StringStream adbShellStart;
  StringStream adbInstall;
  Path appPath;
  Path apkPath;
  int androidTaskSleepTime = 200;
  int androidTaskTimeout = 120000;

  // should be moved to a general state struct
  SSC::String devNull;
  bool verbose = false;
  SSC::String quote = "";
  SSC::String slash = "";
};

struct Paths {
  Path pathBin;
  Path pathPackage;
  Path pathResourcesRelativeToUserBuild;
  Path platformSpecificOutputPath;
};

// Android build / run functions
bool getAdbPath (AndroidCliState &state) {
  // check that stream is empty, otherwise don't rebuild
  if (state.adb.tellp() == 0) {
    if (!platform.win) {
      state.adb << state.androidHome << "/platform-tools/";
    }
    else {
      state.adb << state.androidHome << "\\platform-tools\\";
    }

    state.adb << "adb" << (platform.win ? ".exe" : "");
  }

  if (!fs::exists(state.adb.str())) {
    log("Warn: Failed to locate adb at " + state.adb.str());
    return false;
  }

  if (state.verbose) log((state.adb.str() + (" --version ") + state.devNull));
  if (std::system((state.adb.str() + (" --version ") + state.devNull).c_str()) != 0) {
    log("Warn: Failed to run adb at " + state.adb.str());
    return false;
  }


  // run adb from androidHome to prevent file lock issues in app build folder on windows
  auto cwd = fs::current_path();
  fs::current_path(state.androidHome);
  auto deviceQuery = exec(state.adb.str() + " devices");
  state.emulatorRunning = (deviceQuery.output.find("emulator") != SSC::String::npos);
  fs::current_path(cwd);

  if (Env::get("SSC_ANDROID_TIMEOUT").size() > 0) {
    state.androidTaskTimeout = std::stoi(Env::get("SSC_ANDROID_TIMEOUT"));
    log("Using SSC_ANDROID_TIMEOUT=" + Env::get("SSC_ANDROID_TIMEOUT"));
  }

  return true;
}

bool setupAndroidAvd (AndroidCliState& state) {
  String package = state.quote + "system-images;" + state.platform + ";google_apis;" + replace(platform.arch, "arm64", "arm64-v8a") + state.quote;

  state.avdmanager << state.androidHome;
  if (Env::get("ANDROID_SDK_MANAGER").size() > 0)
  {
    state.avdmanager << "/" << replace(Env::get("ANDROID_SDK_MANAGER"), "sdkmanager", "avdmanager");
  }
  else {
    if (!platform.win) {
      if (std::system(("avdmanager list " + state.devNull).c_str()) != 0) {
        state.avdmanager << "/cmdline-tools/latest/bin/";
      }
    }
    else
      state.avdmanager << "\\cmdline-tools\\latest\\bin\\";
  }

  if (!fs::exists(state.avdmanager.str())) {
    log("ERROR: failed to locate Android Virtual Device (avdmanager): " + state.avdmanager.str());
    return false;
  }


  auto avdListResult = exec((state.avdmanager.str() + " list" + state.devNull).c_str());
  if (avdListResult.exitCode != 0) {
    log("ERROR: failed to run Android Virtual Device (avdmanager)");
    return false;
  }
  state.sscAvdExists = avdListResult.output.find("SSCAVD") != SSC::String::npos;

  state.avdmanager
    << " create avd "
    << "--device 30 " // use pixel 6 pro, better for promos than --device 5 (desktop large)
    << "--force "
    << "--name SSCAVD "
    << ("--abi google_apis/" + replace(platform.arch, "arm64", "arm64-v8a")) << " "
    << "--package " << package;

  if (!state.sscAvdExists) {
    auto createResult = exec(state.avdmanager.str());
    if (state.verbose) {
      log(state.avdmanager.str());
    }
    if (createResult.exitCode != 0) {
      log("Failed to create SSCAVD: " + createResult.output);
      return false;
    }
  }

  return true;
}

bool locateAndroidEmulator (AndroidCliState& state) {
  state.emulator << state.androidHome << state.slash << "emulator" << state.slash << "emulator" << (platform.win ? ".exe" : "");

  if (state.verbose) log(state.emulator.str());
  if (!fs::exists(state.emulator.str())) {
    log("ERROR: failed to locate Android emulator.");
    return false;
  }

  return true;
}

void setupAndroidStartCommands (AndroidCliState& state, bool flagDebugMode) {
  state.adbInstall << state.adb.str() << " ";
  state.adbInstall << "install ";
  state.adbShellStart << state.adb.str() << " ";

  state.apkPath = state.appPath / "build" / "outputs" / "apk" / (flagDebugMode ? "dev" : "live") / "debug" / (SSC::String("app-") + (flagDebugMode ? "dev" : "live") + SSC::String("-debug.apk"));
  state.adbInstall << state.apkPath.string();
}

bool startAndroidEmulator (AndroidCliState& state) {
  // start emulator in the background
  int emulatorStartWaited = 0;
  StringStream emulatorOutput;
  bool emulatorStartFailed = false;

  log("Starting emulator...");
  state.androidEmulatorProcess = new SSC::Process(
    state.emulator.str(),
    " @SSCAVD -gpu swiftshader_indirect", // platform-33: swiftshader not supported below platform-32
    state.androidHome,
    [&state, &emulatorOutput](SSC::String const& out) {
      if (state.verbose) {
        std::cout << out << std::endl;
      } else {
        emulatorOutput << out << std::endl;
      }
    },
    [&state, &emulatorOutput](SSC::String const& out) {
      if (state.verbose) {
        std::cerr << out << std::endl;
      } else {
        emulatorOutput << out << std::endl;
      }
    }
  );
  state.androidEmulatorProcess->open();

  log("Waiting for Android Emulator to boot...");
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    emulatorStartWaited += state.androidTaskSleepTime;

    if (std::system((state.adb.str() + " shell getprop sys.boot_completed" + state.devNull).c_str()) == 0) {
      log("OK.");
      return true;
    } else if (state.androidEmulatorProcess->closed) {
      log("Emulator exited with code " + std::to_string(state.androidEmulatorProcess->status));
      break;
    } else {
      if (emulatorStartWaited >= state.androidTaskTimeout) {
        emulatorStartFailed = true;
        log("Emulator start timed out.");
        break;
      }
    }
  }

  if (state.androidEmulatorProcess->status != 0 && !state.verbose) {
    std::cerr << emulatorOutput.str();
  }

  return false;
}

/// <summary>
/// Handles emulator start state by reattempting install until successful.
/// Times out after 120s by default.
/// </summary>
/// <param name="state"></param>
/// <returns>True if the app was installed, otherwise false.</returns>
bool installAndroidApp (AndroidCliState& state) {
  ExecOutput adbInstallOutput;
  int adbInstallWaited = 0;

  if (!fs::exists(state.apkPath)) {
    log("APK doesn't exist: " + state.apkPath.string());
    return false;
  }

  if (state.verbose) {
    log(state.adbInstall.str());
  }

  while (true) {
    // handle emulator boot issue: cmd: Can't find service: package, no reliable way of detecting when emulator is ready without a blocking logcat call
    // Note that there are several different errors that can occur here based on the state of the emulator, just keep trying to install with a timeout
    adbInstallOutput = exec(state.adbInstall.str() + " 2>&1");
    if (adbInstallOutput.exitCode != 0) {
      if (state.targetPlatform == "android") {
        // not waiting for emulator to boot, break immediately
        break;
      }
      if (adbInstallWaited >= state.androidTaskTimeout) {
        log("Wait for ADB Install timed out.");
        break;
      }
      else {
        std::this_thread::sleep_for(std::chrono::milliseconds(state.androidTaskSleepTime));
        adbInstallWaited += state.androidTaskSleepTime;
      }
    }
    else {
      break;
    }
  }

  String target = state.targetPlatform == "android" ? "device" : "Emulator";

  if (adbInstallOutput.exitCode != 0) {
    if (!state.verbose) {
      // log command for user debug purposes
      log(state.adbInstall.str());
    }

    log(adbInstallOutput.output);
    log("ERROR: failed to install APK on Android " + target + " (adb)");
    std::cout << "Ensure your " << target << " serial number appears (without UNAUTHORIZED) when running" << std::endl;
    std::cout << state.adb.str() << " devices" << std::endl;
    return false;
  }

  return true;
}

bool startAndroidApp (AndroidCliState& state) {
  ExecOutput adbInstallOutput;
  int adbStartWaited = 0;

  auto mainActivity = settings["android_main_activity"];
  if (mainActivity.size() == 0) {
    mainActivity = String(DEFAULT_ANDROID_ACTIVITY_NAME);
  }

  state.adbShellStart << "shell am start -n " << settings["meta_bundle_identifier"] << "/" << settings["meta_bundle_identifier"] << mainActivity << " 2>&1";
  if (state.verbose) log(state.adbShellStart.str());
  ExecOutput adbShellStartOutput;
  while (true) {
    adbShellStartOutput = exec(state.adbShellStart.str());
    if (adbShellStartOutput.output.find("Error type 3") != SSC::String::npos) {
      // ignore this timing related startup error
      std::this_thread::sleep_for(std::chrono::milliseconds(state.androidTaskSleepTime));
      adbStartWaited += state.androidTaskSleepTime;
    }
    else {
      log("App started.");
      break;
    }
    if (adbStartWaited >= state.androidTaskTimeout) {
      log("Wait for shell start timed out.");
      break;
    }
  }

  if (state.verbose) {
    log(adbShellStartOutput.output);
  }

  if (adbShellStartOutput.exitCode != 0) {
    log("Failed to run app on emulator.");
    return false;
  }

  return true;
}

bool initAndStartAndroidEmulator (AndroidCliState& state) {
  if (!setupAndroidAvd(state)) {
    return false;
  }

  if (!locateAndroidEmulator(state)) {
    return false;
  }

  if (!startAndroidEmulator(state))
    return false;

  return true;
}

bool initAndStartAndroidApp (AndroidCliState& state, bool flagDebugMode) {

  setupAndroidStartCommands(state, flagDebugMode);

  log("Installing app...");
  if (!installAndroidApp(state)) {
    return false;
  }

  log("Starting app...");
  if (!startAndroidApp(state)) {
    return false;
  }

  return true;
}

static String getCxxFlags () {
  auto flags = Env::get("CXXFLAGS");
  return flags.size() > 0 ? " " + flags : "";
}

inline String getCfgUtilPath () {
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
  static auto SSC_ENV_FILENAME = Env::get("SSC_ENV_FILENAME");
  static auto filename = SSC_ENV_FILENAME.size() > 0
    ? SSC_ENV_FILENAME
    : DEFAULT_SSC_ENV_FILENAME;

  auto path = targetPath / filename;

  // just set to `targetPath` if resolved `path` doesn't exist
  if (!fs::exists(path) && fs::is_regular_file(path)) {
    path = targetPath;
  }

  if (fs::exists(path) && fs::is_regular_file(path)) {
    auto env = INI::parse(readFile(path));
    for (const auto& tuple : env) {
      auto key = tuple.first;
      auto value = tuple.second;
      auto valueAsPath = Path(value).make_preferred();

      // convert env value to normalized path if it exists
      if (fs::exists(fs::status(valueAsPath))) {
        value = valueAsPath.string();
      }

      Env::set(key, value);
    }
  }
}

void initializeRC (Path targetPath) {
  static auto SSC_RC_FILENAME = Env::get("SSC_RC_FILENAME");
  static auto SSC_RC = Env::get("SSC_RC");
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
    extendMap(rc, INI::parse(readFile(path)));

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
        Env::set(key, value);
      }
    }
  }
}

bool isSetupCompleteAndroid () {
  auto androidHome = getAndroidHome();
  if (androidHome.size() == 0) {
    return false;
  }

  if (!fs::exists(androidHome)) {
    return false;
  }

  Path sdkManager = androidHome + "/" + Env::get("ANDROID_SDK_MANAGER");

  if (!fs::exists(sdkManager)) {
    return false;
  }

  if (!fs::exists(Env::get("JAVA_HOME"))) {
    return false;
  }

  return true;
}

bool isSetupCompleteWindows () {
  if (Env::get("CXX").size() == 0) {
    return false;
  }

  return fs::exists(Env::get("CXX"));
}

bool isSetupComplete (SSC::String platform) {
  std::map<SSC::String, bool(*)()> funcs;

  funcs["android"] = isSetupCompleteAndroid;
  funcs["windows"] = isSetupCompleteWindows;

  if (funcs.count(platform) == 0) return true;

  return funcs[platform]();
}

void run (const String& targetPlatform, Map& settings, const Paths& paths, const bool& flagDebugMode, const bool& flagRunHeadless, const String& argvForward, AndroidCliState& androidState) {
  if (targetPlatform == "ios-simulator") {
    String app = (settings["build_name"] + ".app");
    auto pathToApp = paths.platformSpecificOutputPath / app;
    runIOSSimulator(pathToApp, settings);
  } else if (targetPlatform == "android" || targetPlatform == "android-emulator") {
    if (!getAdbPath(androidState)) {
      exit(1);
    }

    if (targetPlatform == "android-emulator" && !androidState.emulatorRunning) {
      if (!initAndStartAndroidEmulator(androidState)) {
        exit(1);
      }
    }

    if (!initAndStartAndroidApp(androidState, flagDebugMode)) {
      exit(1);
    }

    exit(0);
  } else {
    auto executable = Path(settings["build_name"] + (platform.win ? ".exe" : ""));
    auto exitCode = runApp(paths.pathBin / executable, argvForward, flagRunHeadless);
    return exit(exitCode);
  }

  // Fixes "subcommand 'ssc run' is not supported.""
  log("App failed to run, please contact support.");
  exit(1);
}

struct Option {
  std::vector<String> aliases;
  bool isOptional;
  bool shouldHaveValue;
};
using Options = std::vector<Option>;

struct optionsAndEnv {
  Map optionsWithValue;
  std::unordered_set<String> optionsWithoutValue;
  std::vector<String> envs;
};

optionsAndEnv parseCommandLineOptions (
  const std::span<const char*>& options,
  const Options& availableOptions,
  const String& subcommand
) {
  optionsAndEnv result;
  Map optionsWithValue;
  std::unordered_set<String> optionsWithoutValue;
  std::vector<String> envs;

  for (size_t i = 0; i < options.size(); i++) {
    String arg = options[i];
    size_t equalPos = arg.find('=');
    String key;
    String value;

    if (arg == "-h" || arg == "--help") {
      printHelp(subcommand);
      exit(0);
    }

    if (equal(arg, "--verbose")) {
      flagVerboseMode = true;
      Env::set("SSC_VERBOSE", "1");
      continue;
    }

    if (equal(arg, "--debug")) {
      Env::set("SSC_DEBUG", "1");
      continue;
    }

    bool hasEqualSignDelimiter = equalPos != String::npos;
    // Option in the form "--key=value" or "-k=value"
    if (hasEqualSignDelimiter) {
      key = arg.substr(0, equalPos);
      value = arg.substr(equalPos + 1);
    } else {
      key = arg;
    }

    // path
    if (key.size() && !key.starts_with("-")) {
      targetPath = fs::absolute(key).lexically_normal();
      value = "";
      key = "";
      continue;
    }

    // find option
    Option recognizedOption;
    bool found = false;
    for (const auto option : availableOptions) {
      for (const auto alias : option.aliases) {
        if (alias == key) {
          recognizedOption = option;
          found = true;
          key = recognizedOption.aliases[0];
          if (!recognizedOption.shouldHaveValue && value.size() > 0) {
            std::cerr << "ERROR: option '" << key << "' does not require a value" << std::endl;
            printHelp(subcommand);
            exit(1);
          }
          if (!hasEqualSignDelimiter && recognizedOption.shouldHaveValue) {
            // Option in the form "--key value" or "-k value"
            if (i + 1 < options.size() && options[i + 1][0] != '-') {
              value = options[++i];
            // Option in the form "--key" or "-k"
            } else {
              value = "";
              if (i + 1 < options.size() && options[i + 1][0] != '-') {
                targetPath = fs::absolute(options[i + 1]).lexically_normal();
              }
            }
          }
          if (!recognizedOption.isOptional && value.empty()) {
            std::cerr << "ERROR: option '" << key << "' requires a value" << std::endl;
            printHelp(subcommand);
            exit(1);
          }
          break;
        }
      }
      if (found) break;
    }

    if (!found) {
      std::cerr << "ERROR: unrecognized option '" << key << "'" << std::endl;
      printHelp(subcommand);
      exit(1);
    }

    if (optionsWithValue.count(key) > 0 || optionsWithoutValue.find(key) != optionsWithoutValue.end()) {
      std::cerr << "ERROR: Option '" << key << "' is used more than once." << std::endl;
      printHelp(subcommand);
      exit(1);
    }

    if (value.size() == 0) {
      value = rc[subcommand + "_" + key];
    }

    if (equal(key, "--env")) {
      if (value.size() > 0) {
        auto parts = parseStringList(value);
        for (const auto& part : parts) {
          envs.push_back(part);
        }
      }
      continue;
    }

    if (!value.empty()) {
      optionsWithValue[key] = value;
    } else {
      optionsWithoutValue.insert(key);
    }
  }

  if (targetPath.empty()) {
    targetPath = fs::current_path();
  }

  result.optionsWithValue = optionsWithValue;
  result.optionsWithoutValue = optionsWithoutValue;
  result.envs = envs;

  for (const auto& requiredOption : availableOptions) {
    if (requiredOption.isOptional) {
      continue;
    }
    bool isMissing = optionsWithValue.count(requiredOption.aliases[0]) == 0 || optionsWithoutValue.find(requiredOption.aliases[0]) != optionsWithoutValue.end();
    if (!requiredOption.isOptional && isMissing) {
      std::cerr << "ERROR: required option '" << requiredOption.aliases[0] << "' is missing" << std::endl;
      printHelp(subcommand);
      exit(1);
    }
  }

  return result;
}

int main (const int argc, const char* argv[]) {
  defaultTemplateAttrs = {
    { "ssc_version", SSC::VERSION_FULL_STRING },
    { "project_name", "beepboop" }
  };
  if (argc < 2) {
    printHelp("ssc");
    exit(0);
  }

  auto const subcommand = argv[1];

#ifndef _WIN32
  signal(SIGHUP, signalHandler);
  signal(SIGUSR1, signalHandler);
  signal(SIGUSR2, signalHandler);
#endif

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  if (equal(subcommand, "-v") || equal(subcommand, "--version")) {
    std::cout << SSC::VERSION_FULL_STRING << std::endl;
    std::cerr << "Installation path: " << getSocketHome() << std::endl;
    exit(0);
  }

  if (equal(subcommand, "-h") || equal(subcommand, "--help")) {
    printHelp("ssc");
    exit(0);
  }

  if (equal(subcommand, "--prefix")) {
    std::cout << getSocketHome() << std::endl;
    exit(0);
  }

  if (subcommand[0] == '-') {
    log("unknown option: " + String(subcommand));
    printHelp("ssc");
    exit(0);
  }

  auto const lastOption = argv[argc-1];
  const int numberOfOptions = argc - 2;

#if defined(_WIN32)
  static String HOME = Env::get("HOMEPATH");
#else
  static String HOME = Env::get("HOME");
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
    const Options& availableOptions,
    const bool& needsConfig,
    std::function<void(Map, std::unordered_set<String>)> subcommandHandler
  ) -> void {
    if (argv[1] == subcommand) {
      auto commandlineOptions = std::span(argv, argc).subspan(2, numberOfOptions);
      auto optionsAndEnv = parseCommandLineOptions(commandlineOptions, availableOptions, subcommand);

      auto envs = optionsAndEnv.envs;

      if (needsConfig) {
        auto configExists = false;
        auto configPath = targetPath / "socket.ini";
        auto code = String("");
        auto ini = String("");

        if (fs::exists(configPath)) {
          ini = readFile(configPath);
          configExists = true;
        } else if (!equal(subcommand, "init") && !equal(subcommand, "env")) {
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
            } else if (parts.size() == 1 && Env::has(parts[0])) {
              stream << parts[0] << " = " << Env::get(parts[0]) << "\n";
            }
          }

          ini += stream.str();
        }

        String arguments = "";
        const auto args = std::span(argv, argc).subspan(2);
        int i = 0;
        for (const auto& arg : args) {
          const auto value = String(arg);
          arguments += "'" + trim(value) + "'";
          if (++i < args.size()) {
            arguments += ", ";
          }
        }

        ini += "\n";
        ini += "[ssc]\n";
        ini += "argv = " + arguments;
        ini += "\n";
        ini += "\n";

        if (configExists) {
          auto hex = encodeHexString(ini);
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

        settings = INI::parse(ini);

        if (settings["meta_type"] == "extension" || settings["build_type"] == "extension") {
          auto extension = settings["build_name"];
          settings["build_extensions_" + extension + "_path"] = fs::current_path().string();

          for (const auto& entry : settings) {
            if (entry.first.starts_with("extension_sources")) {
              settings["build_extensions_" + extension] += entry.second;
            } else if (entry.first.starts_with("extension_")) {
              auto key = replace(entry.first, "extension_", extension + "_");
              auto value = entry.second;
              auto index = "build_extensions_" + key;
              if (settings[index].size() > 0) {
                settings[index] += " " + value;
              } else {
                settings[index] = value;
              }
            }
          }
        }

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
        if (configExists && !equal(subcommand, "init") && !equal(subcommand, "env")) {
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
            if (equal(arg, "--prod")) {
              flagDebugMode = false;
              break;
            }
          }

          String suffix = "";

          // internal
          if (flagDebugMode) {
            suffix += "-dev";
          } else {
          }

          settings["debug"] = flagDebugMode;

          std::replace(settings["build_name"].begin(), settings["build_name"].end(), ' ', '_');
          settings["build_name"] += suffix;
        }
      }

      subcommandHandler(optionsAndEnv.optionsWithValue, optionsAndEnv.optionsWithoutValue);
    }
  };

  // first flag indicating whether option is optional
  // second flag indicating whether option should be followed by a value
  Options initOptions = {
    { { "--config", "-C" }, true, false },
    { { "--name", "-n" }, true, true }
  };
  createSubcommand("init", initOptions, false, [&](Map optionsWithValue, std::unordered_set<String> optionsWithoutValue) -> void {
    auto isCurrentPathEmpty = fs::is_empty(fs::current_path());
    auto configOnly = optionsWithoutValue.find("--config") != optionsWithoutValue.end();
    auto projectName = optionsWithValue["--name"];

    if (fs::exists(targetPath / "socket.ini")) {
      log("socket.ini already exists in " + targetPath.string());
    } else {
      if (projectName.size() > 0) {
        defaultTemplateAttrs["project_name"] = projectName;
      }
      writeFile(targetPath / "socket.ini", tmpl(gDefaultConfig, defaultTemplateAttrs));
      log("socket.ini created in " + targetPath.string());
    }
    if (!configOnly) {
      if (isCurrentPathEmpty) {
        fs::create_directories(targetPath / "src");
        writeFile(targetPath / "src" / "index.html", gHelloWorld);
        log("src/index.html created in " + targetPath.string());
      } else {
        log("Current directory was not empty. Assuming index.html is already in place.");
      }
      if (fs::exists(targetPath / ".gitignore")) {
        log(".gitignore already exists in " + targetPath.string());
      } else {
        writeFile(targetPath / ".gitignore", gDefaultGitignore);
        log(".gitignore created in " + targetPath.string());
      }
    }
    exit(0);
  });

  // first flag indicating whether option is optional
  // second flag indicating whether option should be followed by a value
  Options listDevicesOptions = {
    { { "--platform" }, false, true },
    { { "--ecid" }, true, false },
    { { "--udid" }, true, false },
    { { "--only" }, true, false }
  };
  createSubcommand("list-devices", listDevicesOptions, false, [&](Map optionsWithValue, std::unordered_set<String> optionsWithoutValue) -> void {
    bool isUdid =
      optionsWithoutValue.find("--udid") != optionsWithoutValue.end() ||
      equal(rc["list-devices_udid"], "true");
    bool isEcid =
      optionsWithoutValue.find("--ecid") != optionsWithoutValue.end() ||
      equal(rc["list-devices_ecid"], "true");
    bool isOnly =
      optionsWithoutValue.find("--only") != optionsWithoutValue.end() ||
      equal(rc["list-devices_only"], "true");

    auto targetPlatform = optionsWithValue["--platform"];

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

  // first flag indicating whether option is optional
  // second flag indicating whether option should be followed by a value
  Options installAppOptions = {
    { { "--debug", "-D" }, true, false },
    { { "--device" }, true, true },
    { { "--platform" }, true, true },
    { { "--prod", "-P" }, true, false },
    { { "--verbose", "-V" }, true, false },
    { { "--target" }, true, true }
  };
  createSubcommand("install-app", installAppOptions, true, [&](Map optionsWithValue, std::unordered_set<String> optionsWithoutValue) -> void {
    String commandOptions = "";
    String targetPlatform = optionsWithValue["--platform"];
    String installTarget = optionsWithValue["--target"];

    if (targetPlatform.size() > 0) {
      // just assume android when 'android-emulator' is given
      if (targetPlatform == "android-emulator") {
        targetPlatform = "android";
      }

      if (targetPlatform != "ios" && targetPlatform != "android") {
        log("ERROR: Unsupported platform: " + targetPlatform);
        exit(1);
      }
    }

    if (targetPlatform.size() == 0) {
      if (!platform.mac && !platform.linux) {
        log("ERROR: Unsupported host desktop platform. Only 'macOS' and 'Linux' is supported");
        exit(1);
      }

      targetPlatform = platform.os;
    }

    auto device = optionsWithValue["--device"];
    if (device.size() > 0) {
      if (targetPlatform == "ios") {
        commandOptions += " --ecid " + device + " ";
      } else if (targetPlatform == "android") {
        commandOptions += " -s " + device;
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
        log("ERROR: Could not find " + ipaPath.string());
        exit(1);
      }

      // this command will install the app to the first connected device which was
      // added to the provisioning profile if no --device is provided.
      auto command = cfgUtilPath + " " + commandOptions + "install-app " + ipaPath.string();
      if (flagDebugMode) {
        log(command);
      }

      auto r = exec(command);

      if (r.exitCode != 0) {
        log("ERROR: failed to install the app. Is the device plugged in?");
        if (flagDebugMode) {
          log(r.output);
        }
        exit(1);
      }
    } else if (targetPlatform == "android") {
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
    } else if (platform.mac) {
      if (installTarget.size() == 0) {
        installTarget = "/";
      }

      auto paths = getPaths(targetPlatform);
      auto pkgArchive = paths.platformSpecificOutputPath / (settings["build_name"] + ".pkg");
      auto zipArchive = paths.platformSpecificOutputPath / (settings["build_name"] + ".zip");
      auto appDirectory = paths.platformSpecificOutputPath / (settings["build_name"] + ".app");

      if (fs::exists(pkgArchive)) {
        String command = "";

        if (installTarget == "/") {
          command = "sudo installer";
        } else {
          command = "installer";
        }

        if (flagVerboseMode) {
          command += " -verbose";
        }

        command += " -pkg " + pkgArchive.string();
        command += " -target " + installTarget;

        auto r = exec(command);
        if (r.exitCode != 0) {
          log("ERROR: Failed to install app in target");

          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }

        if (flagVerboseMode) {
          log(r.output);
        }
      } else if (fs::exists(zipArchive)) {
        String command = (
          "ditto -x -k " +
          zipArchive.string() +
          " " + (Path(installTarget) / "Applications").string()
        );

        auto r = exec(command);
        if (r.exitCode != 0) {
          log("ERROR: Failed to unzip app to install target");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }

        if (flagVerboseMode) {
          log(r.output);
        }
      } else if (fs::exists(appDirectory)) {
        String command = (
          "cp -r " +
          appDirectory.string() +
          " " + (Path(installTarget) / "Applications").string()
        );

        auto r = exec(command);
        if (r.exitCode != 0) {
          log("ERROR: Failed to copy app to install target");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }

        if (flagVerboseMode) {
          log(r.output);
        }
      } else {
        log("ERROR: Unable to determine macOS application or package to install.");
        exit(1);
      }
    } else if (platform.linux) {
      auto paths = getPaths(targetPlatform);
      auto debArchive = paths.pathPackage.string() + ".deb";

      String command = "dpkg -i " + debArchive;
      auto r = exec(command);
      if (r.exitCode != 0) {
        log("ERROR: Failed to install app to target");
        if (flagDebugMode) {
          log(r.output);
        }
        exit(r.exitCode);
      }

      if (flagVerboseMode) {
        log(r.output);
      }
    } else {
      log("ERROR: Unsupported platform target.");
      exit(1);
    }

    if (targetPlatform == "ios" || targetPlatform == "android") {
      log("Successfully installed the app on your device(s).");
    } else {
      log("Successfully installed the app to your desktop.");
    }
    exit(0);
  });

  // first flag indicating whether option is optional
  // second flag indicating whether option should be followed by a value
  Options printBuildDirOptions = {
    { { "--platform" }, true, true }, { { "--root" }, true, false}
  };

  createSubcommand("print-build-dir", printBuildDirOptions, true, [&](Map optionsWithValue, std::unordered_set<String> optionsWithoutValue) -> void {
    bool flagRoot = optionsWithoutValue.find("--root") != optionsWithoutValue.end();
    // if --platform is specified, use the build path for the specified platform
    auto targetPlatform = optionsWithValue["--platform"];
    Paths paths = getPaths(targetPlatform.size() > 0 ? targetPlatform : platform.os);

    if (flagRoot) {
      std::cout << paths.platformSpecificOutputPath.string() << std::endl;
      exit(0);
    }

    std::cout << paths.pathResourcesRelativeToUserBuild.string() << std::endl;
    exit(0);
  });

  // first flag indicating whether option is optional
  // second flag indicating whether option should be followed by a value
  Options runOptions = {
    { { "--platform" }, true, true },
    { { "--prod", "-P" }, true, false },
    { { "--test", "-t" }, true, true },
    { { "--headless", "-H" }, true, false },
    { { "--debug", "-D" }, true, false },
    { { "--verbose", "-V" }, true, false },
  };

  Options buildOptions = {
    { { "--quiet", "-q" }, true, false },
    { { "--only-build", "-o" }, true, false },
    { { "--run", "-r" }, true, false },
    { { "--watch", "-W" }, true, false },
    { { "--debug", "-D" }, true, false },
    { { "--verbose", "-V" }, true, false },
    { { "--prod", "-P" }, true, false },
    { { "--package", "-p" }, true, false },
    { { "--package-format", "-f" }, true, true },
    { { "--codesign", "-c" }, true, false },
    { { "--notarize", "-n" }, true, false }
  };

  // Insert the elements of runOptions into buildOptions
  buildOptions.insert(buildOptions.end(), runOptions.begin(), runOptions.end());
  createSubcommand("build", buildOptions, true, [&](Map optionsWithValue, std::unordered_set<String> optionsWithoutValue) -> void {
    auto isForExtensionOnly = settings["meta_type"] == "extension" || settings["build_type"] == "extension";

    if (!isForExtensionOnly) {
      if (settings["meta_bundle_identifier"].size() == 0) {
        log("ERROR: [meta] bundle_identifier option is empty");
        exit(1);
      }

      std::regex reverseDnsPattern(R"(^[a-z0-9]+([-a-z0-9]*[a-z0-9]+)?(\.[a-z0-9]+([-a-z0-9]*[a-z0-9]+)?)*$)", std::regex::icase);
      if (!std::regex_match(settings["meta_bundle_identifier"], reverseDnsPattern)) {
          log("ERROR: [meta] bundle_name has invalid format :" + settings["meta_bundle_identifier"]);
          log("Please use reverse DNS notation (https://developer.apple.com/documentation/bundleresources/information_property_list/cfbundleidentifier#discussion)");
          exit(1);
      }

    }

    String argvForward = "";
    String targetPlatform = optionsWithValue["--platform"];
    String additionalBuildArgs = "";

    bool flagRunUserBuildOnly = optionsWithoutValue.find("--only-build") != optionsWithoutValue.end() || equal(rc["build_only"], "true");
    bool flagCodeSign = optionsWithoutValue.find("--codesign") != optionsWithoutValue.end() || equal(rc["build_codesign"], "true");
    bool flagBuildHeadless = settings["build_headless"] == "true";
    bool flagRunHeadless = optionsWithoutValue.find("--headless") != optionsWithoutValue.end();
    bool flagShouldRun = optionsWithoutValue.find("--run") != optionsWithoutValue.end() || equal(rc["build_run"], "true");
    bool flagShouldNotarize = optionsWithoutValue.find("--notarize") != optionsWithoutValue.end() || equal(rc["build_notarize"], "true");
    bool flagShouldPackage = optionsWithoutValue.find("--package") != optionsWithoutValue.end() || equal(rc["build_package"], "true");
    bool flagBuildForIOS = false;
    bool flagBuildForAndroid = false;
    bool flagBuildForAndroidEmulator = false;
    bool flagBuildForSimulator = false;
    bool flagBuildTest = optionsWithoutValue.find("--test") != optionsWithoutValue.end() || optionsWithValue["--test"].size() > 0;
    bool flagShouldWatch = optionsWithoutValue.find("--watch") != optionsWithoutValue.end() || equal(rc["build_watch"], "true");
    String testFile = optionsWithValue["--test"];

    if (optionsWithValue["--package-format"].size() > 0) {
      flagShouldPackage = true;
    }

    if (flagBuildTest && testFile.size() == 0) {
      log("ERROR: --test value is required.");
      exit(1);
    }

    if (testFile.size() > 0) {
      argvForward += " --test=" + testFile;
    }

    if (flagBuildHeadless || flagRunHeadless) {
      argvForward += " --headless";
    }

    if (optionsWithoutValue.find("--quiet") != optionsWithoutValue.end() || equal(rc["build_quiet"], "true")) {
      flagQuietMode = true;
    }

    if (flagShouldNotarize && !platform.mac) {
      log("WARNING: Notarization is only supported on macOS. Ignoring option.");
      flagShouldNotarize = false;
    }

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
      } else {
        std::cout << "Unsupported platform: " << targetPlatform << std::endl;
        exit(1);
      }
    } else {
      targetPlatform = platform.os;
    }

    argvForward +=  " --platform=" + targetPlatform;
    auto platformFriendlyName = targetPlatform == "win32" ? "windows" : targetPlatform;
    platformFriendlyName = platformFriendlyName == "android-emulator" ? "android" : platformFriendlyName;

    String devHost = "localhost";
    if (optionsWithValue.count("--host") > 0) {
      devHost = optionsWithValue["--host"];
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
    settings.insert(std::make_pair("host", devHost));

    String devPort = "0";
    if (optionsWithValue.count("--port") > 0) {
      devPort = optionsWithValue["--port"];
    }
    settings.insert(std::make_pair("port", devPort));

    auto cnt = 0;

    AndroidCliState androidState;
    androidState.targetPlatform = targetPlatform;

    const bool debugEnv = (
      Env::get("SSC_DEBUG").size() > 0 ||
      Env::get("DEBUG").size() > 0
    );
    auto debugBuild = debugEnv;

    const bool verboseEnv = (
      Env::get("SSC_VERBOSE").size() > 0 ||
      Env::get("VERBOSE").size() > 0
    );

    auto oldCwd = fs::current_path();
    auto devNull = ">" + SSC::String((!platform.win) ? "/dev/null" : "NUL") + (" 2>&1");

    String localDirPrefix = !platform.win ? "./" : "";
    String quote = !platform.win ? "'" : "\"";
    String slash = !platform.win ? "/" : "\\";

    if (settings.count("meta_file_limit") == 0) {
      settings["meta_file_limit"] = "4096";
    }

    // set it automatically, hide it from the user
    settings["meta_revision"] = "1";

    if (Env::get("CXX").size() == 0) {
      log("WARNING: $CXX env var not set, assuming defaults");

      if (platform.win) {
        Env::set("CXX=clang++");
      } else {
        Env::set("CXX=/usr/bin/clang++");
      }
    }

    if (Env::get("CI").size() == 0 && !isSetupComplete(platformFriendlyName)) {
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
      if (stat(convertWStringToString(binaryPath).c_str(), &stats) == 0) {
        if (SSC_BUILD_TIME > stats.st_mtime) {
          flagRunUserBuildOnly = false;
        }
      }
    }

    if (flagRunUserBuildOnly) {
      struct stat binaryPathStats;
      struct stat configPathStats;

      if (stat(convertWStringToString(binaryPath).c_str(), &binaryPathStats) == 0) {
        if (stat(convertWStringToString(configPath).c_str(), &configPathStats) == 0) {
          if (configPathStats.st_mtime > binaryPathStats.st_mtime) {
            flagRunUserBuildOnly = false;
          }
        }
      }
    }

    auto removePath = paths.platformSpecificOutputPath;
    if (targetPlatform == "android") {
      removePath = settings["build_output"] + "/android/app/src";
    }

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

    auto pathResourcesRelativeToUserBuild = paths.pathResourcesRelativeToUserBuild;

    if (flagDebugMode) {
      additionalBuildArgs += " --debug=true";
    }

    if (flagBuildTest) {
      additionalBuildArgs += " --test=true";
    }

    String flags;
    String files;

    Path pathResources;
    Path pathToArchive;

    if (platform.mac && exec("xcode-select -p").exitCode != 0) {
      log("Please install Xcode from https://apps.apple.com/us/app/xcode/id497799835");
      exit(1);
    }

    bool isForDesktop = !flagBuildForIOS && !flagBuildForAndroid;

    if (isForDesktop) {
      fs::create_directories(paths.platformSpecificOutputPath / "include");
      writeFile(paths.platformSpecificOutputPath / "include" / "user-config-bytes.hh", settings["ini_code"]);
    }

    //
    // Darwin Package Prep
    // ---
    //
    if (platform.mac && isForDesktop) {
      log("preparing build for mac");

      flags = "-std=c++2a -ObjC++ -v";
      flags += " -framework UniformTypeIdentifiers";
      flags += " -framework CoreBluetooth";
      flags += " -framework CoreLocation";
      flags += " -framework Network";
      flags += " -framework UserNotifications";
      flags += " -framework WebKit";
      flags += " -framework Cocoa";
      flags += " -framework OSLog";
      flags += " -DMACOS=1";
      if (flagCodeSign) {
        flags += " -DWAS_CODESIGNED=1";
      } else {
        flags += " -DWAS_CODESIGNED=0";
      }
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

      if (settings.contains("mac_info_plist_file")) {
        const auto files = parseStringList(settings["mac_info_plist_file"]);
        for (const auto file : files) {
          settings["mac_info_plist_data"] += readFile(file);
        }
      }

      if (!settings.contains("mac_info_plist_data")) {
        settings["mac_info_plist_data"] = "";
      }

      if (flagRunHeadless) {
        settings["mac_info_plist_data"] += (
          "  <key>LSBackgroundOnly</key>\n"
          "  <true/>\n"
        );
      }

      // determine XCode version
      do {
        auto r = exec("xcodebuild -version | head -n1 | awk '{print $2}'");
        if (r.exitCode != 0) {
          if (flagDebugMode) {
          log("Failed to determine XCode version.");
            log(r.output);
          }
          exit(r.exitCode);
        }

        settings["__xcode_version"] = trim(r.output);
      } while (0);

      // determine XCode Build version
      do {
        auto r = exec("xcodebuild -version | tail -n1 | awk '{print $3}'");
        if (r.exitCode != 0) {
          log("Failed to determine XCode Build version code.");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }

        settings["__xcode_build_version"] = trim(r.output);
      } while (0);

      // determine macOS SDK build version
      do {
        auto r = exec(" xcodebuild -sdk macosx -version | grep ProductBuildVersion | awk '{ print $2 }'");
        if (r.exitCode != 0) {
          if (flagDebugMode) {
          log("Failed to determine MacOSX SDK build version code.");
            log(r.output);
          }
          exit(r.exitCode);
        }

        settings["__xcode_macosx_sdk_build_version"] = trim(r.output);
      } while (0);

      // determine macOS SDK version
      do {
        auto r = exec(" xcodebuild -sdk macosx -version | grep ProductVersion | awk '{ print $2 }'");
        if (r.exitCode != 0) {
          if (flagDebugMode) {
          log("Failed to determine MacOSX SDK version.");
            log(r.output);
          }
          exit(r.exitCode);
        }

        settings["__xcode_macosx_sdk_version"] = trim(r.output);
      } while (0);

      if (settings["mac_minimum_supported_version"].size() == 0) {
        settings["mac_minimum_supported_version"] = "13.0.0";
      }

      auto plistInfo = tmpl(gMacOSInfoPList, settings);

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
      fs::create_directories(res / "mipmap");

      pathResources = { src / "main" / "assets" };

      // set current path to output directory
      if (debugEnv || verboseEnv) log("cd " + output.string());
      fs::current_path(output);

      auto androidHome = getAndroidHome();
      StringStream sdkmanager;

      if (debugEnv || verboseEnv) log("sdkmanager --version 2>&1 >/dev/null");

      sdkmanager << androidHome;
      if (Env::get("ANDROID_SDK_MANAGER").size() > 0) {
        sdkmanager << "/" << Env::get("ANDROID_SDK_MANAGER");
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


      // quote entire command and "yes" for windows, this is the only to get around "c:\Program" not a command error
      // note that we don't want to use this quote on non windows, therefore make an extra variable
      auto cmdQuote = platform.win ? "\"" : "";
      // redirect yes stderr to stdout, this hides "broken pipe" / "no space left on device" errors that are caused by sdkmanager terminating normally
      String licenseAccept =
      cmdQuote + quote + (Env::get("ANDROID_SDK_MANAGER_ACCEPT_LICENSES").size() > 0 ? (Env::get("ANDROID_SDK_MANAGER_ACCEPT_LICENSES")) : "echo") + quote  +
      (platform.win ? " 2>&1" : "") + " | " +
      quote + sdkmanager.str() + quote + " --licenses" + cmdQuote;

      auto licenseResult = exec(licenseAccept.c_str());
      if (licenseResult.exitCode != 0) {
        log(licenseResult.output);
        log(("Check licenses and run again: \n" + licenseAccept + "\n").c_str());
      }

      // TODO: internal, no need to save to settings
      settings["android_sdk_manager_path"] = sdkmanager.str();

      String gradlePath = Env::get("GRADLE_HOME").size() > 0 ? Env::get("GRADLE_HOME") + slash + "bin" + slash : "";

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
          (" JAVA_HOME=\"" + Env::get("JAVA_HOME") + "\" && ") +
          licenseAccept +
          "\n"
         );
        exit(1);
      }

      // user entry
      fs::copy(trim(prefixFile("src/init.cc")), jni, fs::copy_options::overwrite_existing);
      // android runtime
      fs::copy(trim(prefixFile("src/android/bridge.cc")), jni / "android", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/android/runtime.cc")), jni / "android", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/android/string_wrap.cc")), jni / "android", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/android/window.cc")), jni / "android", fs::copy_options::overwrite_existing);
      // core
      fs::copy(trim(prefixFile("src/core/codec.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/config.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/core.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/debug.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/env.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/ini.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/io.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/json.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/platform.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/preload.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/string.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/types.hh")), jni / "core", fs::copy_options::overwrite_existing);
      fs::copy(trim(prefixFile("src/core/version.hh")), jni / "core", fs::copy_options::overwrite_existing);
      // ipc
      fs::copy(trim(prefixFile("src/ipc/ipc.hh")), jni / "ipc", fs::copy_options::overwrite_existing);
      // window
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
        settings["android_native_abis"] = Env::get("ANDROID_SUPPORTED_ABIS");
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

      if (settings["permission_allow_notifications"] != "false") {
        manifestContext["android_manifest_xml_permissions"] += "<uses-permission android:name=\"android.permission.POST_NOTIFICATIONS\" />\n";
      }

      if (settings["permission_allow_geolocation"] != "false") {
        manifestContext["android_manifest_xml_permissions"] += "<uses-permission android:name=\"android.permission.ACCESS_FINE_LOCATION\" />\n";
        manifestContext["android_manifest_xml_permissions"] += "<uses-permission android:name=\"android.permission.ACCESS_COARSE_LOCATION\" />\n";
        if (settings["permission_allow_geolocation_in_background"] != "false") {
          manifestContext["android_manifest_xml_permissions"] += "<uses-permission android:name=\"android.permission.ACCESS_BACKGROUND_LOCATION\" />\n";
        }
      }

      if (settings["permission_allow_user_media"] != "false") {
        if (settings["permission_allow_camera"] != "false") {
          manifestContext["android_manifest_xml_permissions"] += "<uses-permission android:name=\"android.permission.CAMERA\" />\n";
        }

        if (settings["permission_allow_microphone"] != "false") {
          manifestContext["android_manifest_xml_permissions"] += "<uses-permission android:name=\"android.permission.CAPTURE_AUDIO_OUTPUT\" />\n";
        }
      }

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
        if (flagDebugMode) {
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

      auto androidResources = settings["android_resources"];
      auto androidIcon = settings["android_icon"];

      if (androidIcon.size() > 0) {
        fs::copy(targetPath / androidIcon, res / "mipmap" / "icon.png", fs::copy_options::overwrite_existing);
        fs::copy(targetPath / androidIcon, res / "mipmap" / "ic_launcher.png", fs::copy_options::overwrite_existing);
        fs::copy(targetPath / androidIcon, res / "mipmap" / "ic_launcher_round.png", fs::copy_options::overwrite_existing);
      } else {
        // create empty icons
        std::ofstream icon((targetPath / androidIcon, res / "mipmap" / "icon.png").string());
        std::ofstream ic_launcher((targetPath / androidIcon, res / "mipmap" / "ic_launcher.png").string());
        std::ofstream ic_launcher_round((targetPath / androidIcon, res / "mipmap" / "ic_launcher_round.png").string());
      }

      // allow user space to override all `res/` files
      if (fs::exists(androidResources)) {
        fs::copy(androidResources, res, fs::copy_options::overwrite_existing);
      }

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
      makefileContext["__android_native_extensions_context"] = "";

      if (settings["android_native_abis"].size() > 0) {
        makefileContext["android_native_abis"] = settings["android_native_abis"];
      } else {
        makefileContext["android_native_abis"] = "arm64-v8a x86_64";
      }

      // custom native sources
      for (
        auto const &file :
        parseStringList(settings["android_native_sources"], ' ')
      ) {
        auto filename = Path(file).filename();
        writeFile(
          jni / "src" / filename,
          tmpl(std::regex_replace(
            convertWStringToString(readFile(targetPath / file )),
            std::regex("__BUNDLE_IDENTIFIER__"),
            bundle_identifier
          ), settings)
        );
      }

      Vector<String> seenExtensions;
      for (const auto& tuple : settings) {
        auto& key = tuple.first;
        if (tuple.second.size() == 0) continue;
        if (key.starts_with("build_extensions_")) {
          if (key.find("compiler_flags") != String::npos) continue;
          if (key.find("compiler_debug_flags") != String::npos) continue;
          if (key.find("linker_flags") != String::npos) continue;
          if (key.find("linker_debug_flags") != String::npos) continue;
          if (key.find("configure_script") != String::npos) continue;
          if (key.starts_with("build_extensions_mac_")) continue;
          if (key.starts_with("build_extensions_linux_")) continue;
          if (key.starts_with("build_extensions_win_")) continue;
          if (key.starts_with("build_extensions_ios_")) continue;
          if (key.ends_with("_path")) continue;

          String extension;
          if (key.starts_with("build_extensions_android")) {
            extension = replace(key, "build_extensions_android_", "");
          } else {
            extension = replace(key, "build_extensions_", "");
          }

          extension = split(extension, '_')[0];

          if (std::find(seenExtensions.begin(), seenExtensions.end(), extension) != seenExtensions.end()) {
            continue;
          }
          seenExtensions.push_back(extension);

          auto source = settings["build_extensions_" + extension + "_source"];
          auto oldCwd = fs::current_path();
          fs::current_path(targetPath);

          if (source.size() == 0 && fs::is_directory(settings["build_extensions_" + extension])) {
            source = settings["build_extensions_" + extension];
            settings["build_extensions_" + extension] = "";
          }

          if (source.size() > 0) {
            Path target;
            if (fs::exists(source)) {
              target = source;
            } else if (source.ends_with(".git")) {
              auto path = Path { source };
              target = paths.platformSpecificOutputPath / "extensions" / replace(path.filename().string(), ".git", "");

              if (!fs::exists(target)) {
                auto exitCode = std::system(("git clone " + source + " " + target.string()).c_str());

                if (exitCode) {
                  exit(exitCode);
                }
              }
            }

            if (fs::exists(target)) {
              auto configFile = target / "socket.ini";
              auto config = INI::parse(fs::exists(configFile) ? readFile(configFile) : "");
              settings["build_extensions_" + extension + "_path"] = target.string();

              for (const auto& entry : config) {
                if (entry.first.starts_with("extension_sources")) {
                  settings["build_extensions_" + extension] += entry.second;
                } else if (entry.first.starts_with("extension_")) {
                  auto key = replace(entry.first, "extension_", extension + "_");
                  auto value = entry.second;
                  auto index = "build_extensions_" + key;
                  if (settings[index].size() > 0) {
                    settings[index] += " " + value;
                  } else {
                    settings[index] = value;
                  }
                }
              }
            }
          }

          auto compilerFlags = replace(
            settings["build_extensions_compiler_flags"] + " " +
            settings["build_extensions_android_compiler_flags"] + " " +
            settings["build_extensions_" + extension + "_compiler_flags"] + " " +
            settings["build_extensions_" + extension + "_android_compiler_flags"] +  " ",
            "-I",
            "-I$(LOCAL_PATH)/"
          );

          auto compilerDebugFlags = replace(
            settings["build_extensions_compiler_debug_flags"] + " " +
            settings["build_extensions_android_compiler_debug_flags"] + " " +
            settings["build_extensions_" + extension + "_compiler_debug_flags"] + " " +
            settings["build_extensions_" + extension + "_android_compiler_debug_flags"] +  " ",
            "-I",
            "-I$(LOCAL_PATH)/"
          );

          auto linkerFlags = replace(
            settings["build_extensions_linker_flags"] + " " +
            settings["build_extensions_android_linker_flags"] + " " +
            settings["build_extensions_" + extension + "_linker_flags"] + " " +
            settings["build_extensions_" + extension + "_android_linker_flags"] +  " ",
            "-I",
            "-I$(LOCAL_PATH)/"
          );

          auto linkerDebugFlags = replace(
            settings["build_extensions_linker_debug_flags"] + " " +
            settings["build_extensions_android_linker_debug_flags"] + " " +
            settings["build_extensions_" + extension + "_linker_debug_flags"] + " " +
            settings["build_extensions_" + extension + "_android_linker_debug_flags"] + " ",
            "-I",
            "-I$(LOCAL_PATH)/"
          );

          auto configure = settings["build_extensions_" + extension + "_configure_script"];
          auto build = settings["build_extensions_" + extension + "_build_script"];
          auto copy = settings["build_extensions_" + extension + "_build_copy"];
          auto path = settings["build_extensions_" + extension + "_path"];

          auto sources = StringStream();
          auto make = StringStream();
          auto cwd = targetPath;
          std::unordered_set<String> cflags;
          std::unordered_set<String> cppflags;

          if (path.size() > 0) {
            fs::current_path(targetPath);
            fs::current_path(path);
          } else {
            fs::current_path(targetPath);
          }

          if (build.size() > 0) {
            auto exitCode = std::system((build + argvForward).c_str());
            if (exitCode) {
              exit(exitCode);
            }
          }

          if (configure.size() > 0) {
            auto output = replace(exec(configure + argvForward).output, "\n", " ");
            if (output.size() > 0) {
              for (const auto& source : parseStringList(output, ' ')) {
                auto destination = fs::absolute(targetPath / source);
                sources << destination.string() << " ";
              }
            }
          }

          if (copy.size() > 0) {
            auto pathResources = paths.pathResourcesRelativeToUserBuild;
            for (const auto& file : parseStringList(copy, ' ')) {
              auto parts = split(file, ':');
              auto target = parts[0];
              auto destination = parts.size() == 2 ? pathResources / parts[1] : pathResources;
              fs::create_directories(destination.parent_path());
              fs::copy(
                target,
                destination,
                fs::copy_options::update_existing | fs::copy_options::recursive
              );
            }
          }

          for (
            auto source : parseStringList(
              trim(settings["build_extensions_" + extension] + " " + settings["build_extensions_" + extension + "_android"]),
              ' '
            )
          ) {
            if (fs::is_directory(source)) {
              auto target = Path(source).filename();
              fs::create_directories(jni / target);
              fs::copy(
                source,
                jni / target,
                fs::copy_options::update_existing | fs::copy_options::recursive
              );
              continue;
            } else if (!fs::is_regular_file(source)) {
              continue;
            }

            auto destination = (
              Path(source).parent_path() /
              Path(source).filename().string()
            ).string();

            if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
              log("extension source: " + source);
            }

            fs::create_directories(jni / Path(destination).parent_path());
            fs::copy(source, jni / destination, fs::copy_options::overwrite_existing);

            if (destination.ends_with(".hh") || destination.ends_with(".h")) {
              continue;
            } else if (destination.ends_with(".cc") || destination.ends_with(".cpp") || destination.ends_with(".c++") || destination.ends_with(".mm")) {
              cppflags.insert("-std=c++2a");
              cppflags.insert("-fexceptions");
              cppflags.insert("-frtti");
              cppflags.insert("-fsigned-char");
            }

            sources << destination << " ";
          }

          fs::current_path(oldCwd);

          Vector<String> libs;
          auto archs = settings["android_native_abis"];

          if (archs.size() == 0) {
            archs = "arm64-v8a x86_64";
          }

          for (const auto& source : parseStringList(sources.str(), ' ')) {
            if (source.ends_with(".a")) {
              auto name = replace(Path(source).filename().string(), ".a", "");
              for (const auto& arch : parseStringList(archs, ' ')) {
                if (source.find(arch) != -1) {
                  fs::create_directories(jni / ".." / "libs" / arch);
                  fs::copy(source, jni / ".." / "libs" / arch / Path(source).filename(), fs::copy_options::overwrite_existing);
                }
              }

              if (std::find(libs.begin(), libs.end(), name) == libs.end()) {
                libs.push_back(name);
                make << "## " << Path(source).filename().string() << std::endl;
                make << "include $(CLEAR_VARS)" << std::endl;
                make << "LOCAL_MODULE := " << name << std::endl;
                make << "LOCAL_SRC_FILES = ../libs/$(TARGET_ARCH_ABI)/" << Path(source).filename().string() << std::endl;
                make << "include $(PREBUILT_STATIC_LIBRARY)" << std::endl;
              }
            }
          }

          make << "## socket/extensions/" << extension << RUNTIME_EXTENSION_FILE_EXT << std::endl;
          make << "include $(CLEAR_VARS)" << std::endl;
          make << "LOCAL_MODULE := extension-" << extension << std::endl;
          make << std::endl;
          make << "LOCAL_CFLAGS += \\" << std::endl;
          for (const auto& cflag : cflags) {
            make << "  " << cflag << " \\" << std::endl;
          }
          if (flagDebugMode) {
            make << "  -g                         \\" << std::endl;
          }
          make << "  -I$(LOCAL_PATH)/include    \\" << std::endl;
          make << "  -I$(LOCAL_PATH)            \\" << std::endl;
          make << "  -pthreads                  \\" << std::endl;
          make << "  -fPIC                      \\" << std::endl;
          make << "  -O0" << std::endl;
          make << std::endl;

          make << "LOCAL_CFLAGS += \\" << std::endl;
          make << "  -DDEBUG=" << (flagDebugMode ? 1 : 0) << " \\" << std::endl;
          make << "  -DANDROID=1" << " \\" << std::endl;
          make << "  -DSSC_VERSION=" << SSC::VERSION_STRING << " \\" << std::endl;
          make << "  -DSSC_VERSION_HASH=" << SSC::VERSION_HASH_STRING << std::endl;
          make << std::endl;

          if (compilerFlags.size() > 0) {
            make << "LOCAL_CFLAGS +=  \\" << std::endl;
            make << compilerFlags << std::endl;
            make << std::endl;
          }

          if (flagDebugMode && compilerDebugFlags.size() > 0) {
            make << "LOCAL_CFLAGS +=  \\" << std::endl;
            make << compilerDebugFlags << std::endl;
            make << std::endl;
          }

          if (linkerFlags.size() > 0) {
            make << "LOCAL_LDFLAGS +=  \\" << std::endl;
            make << linkerFlags << std::endl;
            make << std::endl;
          }

          if (flagDebugMode && linkerDebugFlags.size() > 0) {
            make << "LOCAL_LDFLAGS +=  \\" << std::endl;
            make << linkerDebugFlags << std::endl;
            make << std::endl;
          }

          if (flags.size() > 0) {
            make << "LOCAL_CFLAGS += " << flags << std::endl;
          }

          if (settings["android_native_cflags"].size() > 0) {
            make << "LOCAL_CFLAGS += " << settings["android_native_cflags"] << std::endl;
          }

          make << "LOCAL_CPPFLAGS += \\" << std::endl;
          for (const auto& cppflag : cppflags) {
            make << "  " << cppflag << " \\" << std::endl;
          }
          make << std::endl;

          make << "LOCAL_LDLIBS += -landroid -llog" << std::endl;

          for (const auto& lib : libs) {
            make << "LOCAL_STATIC_LIBRARIES += " << lib << std::endl;
          }
          make << std::endl;

          make << "LOCAL_STATIC_LIBRARIES += libuv libsocket-runtime-static" << std::endl;
          make << "LOCAL_SRC_FILES = init.cc" <<  std::endl;

          for (const auto& source : parseStringList(sources.str(), ' ')) {
            if (source.ends_with(".c") || source.ends_with(".cc") || source.ends_with(".mm") || source.ends_with(".m")) {
              make << "LOCAL_SRC_FILES += " << source <<  std::endl;
            }
          }

          make << std::endl;

          make << "include $(BUILD_SHARED_LIBRARY)" << std::endl;
          make << std::endl;

          makefileContext["__android_native_extensions_context"] += make.str();
        }
      }

      if (settings["android_native_makefile"].size() > 0) {
        makefileContext["android_native_make_context"] =
          trim(tmpl(tmpl(convertWStringToString(readFile(targetPath / settings["android_native_makefile"])), settings), makefileContext));
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
          convertWStringToString(readFile(trim(prefixFile("src/android/internal.hh")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_path_underscored
        )
      );

      writeFile(
        pkg / "bridge.kt",
        std::regex_replace(
          convertWStringToString(readFile(trim(prefixFile("src/android/bridge.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      writeFile(
        pkg / "main.kt",
        std::regex_replace(
          convertWStringToString(readFile(trim(prefixFile("src/android/main.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      writeFile(
        pkg / "runtime.kt",
        std::regex_replace(
          convertWStringToString(readFile(trim(prefixFile("src/android/runtime.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      writeFile(
        pkg / "window.kt",
        std::regex_replace(
          convertWStringToString(readFile(trim(prefixFile("src/android/window.kt")))),
          std::regex("__BUNDLE_IDENTIFIER__"),
          bundle_identifier
        )
      );

      writeFile(
        pkg / "webview.kt",
        std::regex_replace(
          convertWStringToString(readFile(trim(prefixFile("src/android/webview.kt")))),
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
            convertWStringToString(readFile(targetPath / file )),
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

        auto pathToInstalledProfile = Path(Env::get("HOME")) /
          "Library" /
          "MobileDevice" /
          "Provisioning Profiles" /
          (uuid + ".mobileprovision");

        if (!fs::exists(pathToInstalledProfile)) {
          fs::copy(pathToProfile, pathToInstalledProfile);
        }

        settings["ios_provisioning_specifier"] = provSpec;
        settings["ios_provisioning_profile"] = uuid;
        settings["apple_team_identifier"] = team;
      }

      if (flagBuildForSimulator) {
        settings["ios_provisioning_specifier"] = "";
        settings["ios_provisioning_profile"] = "";
        settings["ios_codesign_identity"] = "";
        settings["apple_team_identifier"] = "";
      }

      // --platform=ios should always build for arm64 even on Darwin x86_64
      auto arch = flagBuildForSimulator ? platform.arch : "arm64";
      auto deviceType = arch + "-iPhone" + (flagBuildForSimulator ? "Simulator" : "OS");

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

      Map xCodeProjectVariables = settings;
      extendMap(xCodeProjectVariables, Map {
        {"SSC_SETTINGS", _settings},
        {"SSC_VERSION", VERSION_STRING},
        {"SSC_VERSION_HASH", VERSION_HASH_STRING},
        {"WAS_CODESIGNED", flagCodeSign ? "1" : "0"},
        {"__ios_native_extensions_build_ids", ""},
        {"__ios_native_extensions_build_refs", ""},
        {"__ios_native_extensions_build_context_refs", ""},
        {"__ios_native_extensions_build_context_sections", ""}
      });

      fs::create_directories(paths.pathResourcesRelativeToUserBuild / "socket" / "extensions");
      auto iosSdkPath = trim(flagBuildForSimulator
        ? exec("xcrun -sdk iphonesimulator -show-sdk-path").output
        : exec("xcrun -sdk iphoneos -show-sdk-path").output
      );

      auto extensions = Vector<String>();
      for (const auto& tuple : settings) {
        auto& key = tuple.first;
        if (tuple.second.size() == 0) continue;
        if (key.starts_with("build_extensions_")) {
          if (key.find("compiler_flags") != String::npos) continue;
          if (key.find("compiler_debug_flags") != String::npos) continue;
          if (key.find("linker_flags") != String::npos) continue;
          if (key.find("linker_debug_flags") != String::npos) continue;
          if (key.find("configure_script") != String::npos) continue;
          if (key.starts_with("build_extensions_mac_")) continue;
          if (key.starts_with("build_extensions_linux_")) continue;
          if (key.starts_with("build_extensions_win_")) continue;
          if (key.starts_with("build_extensions_android_")) continue;
          if (key.ends_with("_path")) continue;

          String extension;
          if (key.starts_with("build_extensions_ios")) {
            extension = replace(key, "build_extensions_ios_", "");
          } else {
            extension = replace(key, "build_extensions_", "");
          }

          extension = split(extension, '_')[0];

          auto source = settings["build_extensions_" + extension + "_source"];

          if (source.size() == 0 && fs::is_directory(settings["build_extensions_" + extension])) {
            source = settings["build_extensions_" + extension];
            settings["build_extensions_" + extension] = "";
          }

          if (source.size() > 0) {
            Path target;
            if (fs::exists(source)) {
              target = source;
            } else if (source.ends_with(".git")) {
              auto path = Path { source };
              target = paths.platformSpecificOutputPath / "extensions" / replace(path.filename().string(), ".git", "");

              if (!fs::exists(target)) {
                auto exitCode = std::system(("git clone " + source + " " + target.string()).c_str());

                if (exitCode) {
                  exit(exitCode);
                }
              }
            }

            if (fs::exists(target)) {
              auto configFile = target / "socket.ini";
              auto config = INI::parse(fs::exists(configFile) ? readFile(configFile) : "");
              settings["build_extensions_" + extension + "_path"] = target.string();

              for (const auto& entry : config) {
                if (entry.first.starts_with("extension_sources")) {
                  settings["build_extensions_" + extension] += entry.second;
                } else if (entry.first.starts_with("extension_")) {
                  auto key = replace(entry.first, "extension_", extension + "_");
                  auto value = entry.second;
                  auto index = "build_extensions_" + key;
                  if (settings[index].size() > 0) {
                    settings[index] += " " + value;
                  } else {
                    settings[index] = value;
                  }
                }
              }
            }
          }

          auto configure = settings["build_extensions_" + extension + "_configure_script"];
          auto build = settings["build_extensions_" + extension + "_build_script"];
          auto copy = settings["build_extensions_" + extension + "_build_copy"];
          auto path = settings["build_extensions_" + extension + "_path"];

          auto sources = parseStringList(
            trim(settings["build_extensions_" + extension] + " " + settings["build_extensions_" + extension + "_ios"]),
            ' '
          );

          auto objects = StringStream();
          auto libdir = platform.arch == "arm64"
            ? prefixFile(String("lib/arm64-") + (flagBuildForSimulator ? "iPhoneSimulator" : "iPhoneOS"))
            : prefixFile(String("lib/") + (flagBuildForSimulator ? "x86_64-iPhoneSimulator" : "arm64-iPhoneOS"));

          if (std::find(extensions.begin(), extensions.end(), extension) == extensions.end()) {
            log("Building extension: " + extension + " (" + (flagBuildForSimulator ? "x86_64-iPhoneSimulator" : "arm64-iPhoneOS") + ")");
            extensions.push_back(extension);
          }

          if (path.size() > 0) {
            fs::current_path(targetPath);
            fs::current_path(path);
          } else {
            fs::current_path(targetPath);
          }

          if (build.size() > 0) {
            auto exitCode = std::system((build + argvForward).c_str());
            if (exitCode) {
              exit(exitCode);
            }
          }

          if (configure.size() > 0) {
            auto output = replace(exec(configure + argvForward).output, "\n", " ");
            if (output.size() > 0) {
              for (const auto& source : parseStringList(output, ' ')) {
                sources.push_back(source);
              }
            }
          }

          if (copy.size() > 0) {
            auto pathResources = paths.platformSpecificOutputPath / "ui";
            for (const auto& file : parseStringList(copy, ' ')) {
              auto parts = split(file, ':');
              auto target = parts[0];
              auto destination = parts.size() == 2 ? pathResources / parts[1] : pathResources;
              fs::copy(
                target,
                destination,
                fs::copy_options::update_existing | fs::copy_options::recursive
              );
            }
          }

          for (auto source : sources) {
            if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
              log("extension source: " + source);
            }

            String compiler;

            auto compilerFlags = (
              settings["build_extensions_compiler_flags"] +
              settings["build_extensions_ios_compiler_flags"] +
              settings["build_extensions_" + extension + "_compiler_flags"] +
              settings["build_extensions_" + extension + "_ios_compiler_flags"] +
              " -framework UniformTypeIdentifiers" +
              " -framework CoreBluetooth" +
              " -framework CoreLocation" +
              " -framework Network" +
              " -framework UserNotifications" +
              " -framework WebKit" +
              " -framework Cocoa" +
              " -framework OSLog"
            );

            auto compilerDebugFlags = (
              settings["build_extensions_compiler_debug_flags"] + " " +
              settings["build_extensions_ios_compiler_debug_flags"] + " " +
              settings["build_extensions_" + extension + "_compiler_debug_flags"] + " " +
              settings["build_extensions_" + extension + "_ios_compiler_debug_flags"] + " "
            );

            // --platform=ios should always build for arm64 even on Darwin x86_64
            if (!flagBuildForSimulator) {
              compilerFlags += " -arch arm64 ";
            }

            if (source.ends_with(".hh") || source.ends_with(".h")) {
              continue;
            } else if (source.ends_with(".cc") || source.ends_with(".cpp") || source.ends_with(".c++") || source.ends_with(".mm")) {
              compiler = "clang++";
              compilerFlags += " -std=c++2a -ObjC++ -v ";
            } else if (source.ends_with(".o") || source.ends_with(".a")) {
              objects << source << " ";
              continue;
            } else if (source.ends_with(".c")) {
              compiler = "clang";
              compilerFlags += " -ObjC -v ";
            } else {
              continue;
            }

            auto objectFile = source;
            objectFile = replace(objectFile, "\\.mm", ".o");
            objectFile = replace(objectFile, "\\.m", ".o");
            objectFile = replace(objectFile, "\\.cc", ".o");
            objectFile = replace(objectFile, "\\.c", ".o");

            auto filename = Path(objectFile).filename();
            auto object = (
              paths.pathResourcesRelativeToUserBuild /
              "socket" /
              "extensions" /
              extension /
              filename
            );

            fs::create_directories(object.parent_path());

            objects << object.string() << " ";
            auto compileExtensionObjectCommand = StringStream();
            compileExtensionObjectCommand
              << "xcrun -sdk " << (flagBuildForSimulator ? "iphonesimulator" : "iphoneos")
              << " " << compiler
              << " -I" << Path(paths.platformSpecificOutputPath / "include").string()
              << " -I" << prefixFile()
              << " -I" << prefixFile("include")
              << " -DIOS=1"
              << " -DANDROID=0"
              << " -DDEBUG=" << (flagDebugMode ? 1 : 0)
              << " -DHOST=" << devHost
              << " -DPORT=" << devPort
              << " -DSSC_VERSION=" << SSC::VERSION_STRING
              << " -DSSC_VERSION_HASH=" << SSC::VERSION_HASH_STRING
              << " -target " << (flagBuildForSimulator ? platform.arch + "-apple-ios-simulator": "arm64-apple-ios")
              << " -fembed-bitcode"
              << " -fPIC"
              << " " << trim(compilerFlags + " " + (flagDebugMode ? compilerDebugFlags : ""))
              << " " << (flagBuildForSimulator ? "-mios-simulator-version-min=" : "-miphoneos-version-min=") + Env::get("IPHONEOS_VERSION_MIN", "15.0")
              << " -c " << source
              << " -o " << object.string()
            ;

            if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
              log(compileExtensionObjectCommand.str());
            }

            do {
              auto r = exec(compileExtensionObjectCommand.str());

              if (r.exitCode != 0) {
                log("Unable to build extension object (" + object.string() + ")");
                log(r.output);
                exit(r.exitCode);
              }
            } while (0);
          }

          auto linkerFlags = (
            settings["build_extensions_linker_flags"] + " " +
            settings["build_extensions_ios_linker_flags"] + " " +
            settings["build_extensions_" + extension + "_linker_flags"] + " " +
            settings["build_extensions_" + extension + "_ios_linker_flags"] + " "
          );

          auto linkerDebugFlags = (
            settings["build_extensions_linker_debug_flags"] + " " +
            settings["build_extensions_ios_linker_debug_flags"] + " " +
            settings["build_extensions_" + extension + "_linker_debug_flags"] + " " +
            settings["build_extensions_" + extension + "_ios_linker_debug_flags"] + " "
          );

          // --platform=ios should always build for arm64 even on Darwin x86_64
          if (!flagBuildForSimulator) {
            linkerFlags += " -arch arm64 ";
          }

          auto lib = (paths.pathResourcesRelativeToUserBuild / "socket" / "extensions" / extension / (extension + RUNTIME_EXTENSION_FILE_EXT));
          fs::create_directories(lib.parent_path());
          auto compileExtensionLibraryCommand = StringStream();
          compileExtensionLibraryCommand
            << "xcrun -sdk " << (flagBuildForSimulator ? "iphonesimulator" : "iphoneos")
            << " clang++"
            << " " << objects.str()
            << " " << prefixFile("src/init.cc")
            << " " << flags
            << " -I" << Path(paths.platformSpecificOutputPath / "include").string()
            << " -I" << prefixFile()
            << " -I" << prefixFile("include")
            << " -L" + libdir
            << " -lsocket-runtime"
            << " -luv"
            << " -isysroot " << iosSdkPath << "/"
            << " -iframeworkwithsysroot /System/Library/Frameworks/"
            << " -F " << iosSdkPath << "/System/Library/Frameworks/"
            << " -framework UniformTypeIdentifiers"
            << " -framework CoreBluetooth"
            << " -framework CoreLocation"
            << " -framework Foundation"
            << " -framework Network"
            << " -framework UserNotifications"
            << " -framework WebKit"
            << " -framework UIKit"
            << " -fembed-bitcode"
            << " -std=c++2a"
            << " -ObjC++"
            << " -shared"
            << " -v"
            << " -target " << (flagBuildForSimulator ? platform.arch + "-apple-ios-simulator": "arm64-apple-ios")
            << " " << (flagBuildForSimulator ? "-mios-simulator-version-min=" : "-miphoneos-version-min=") + Env::get("IPHONEOS_VERSION_MIN", "15.0")
            << " " << trim(linkerFlags + " " + (flagDebugMode ? linkerDebugFlags : ""))
            << " -o " << lib
          ;

          if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
            log(compileExtensionLibraryCommand.str());
          }

          do {
            auto r = exec(compileExtensionLibraryCommand.str());

            if (r.exitCode != 0) {
              log("Unable to build extension (" + extension + ")");
              log(r.output);
              exit(r.exitCode);
            }
          } while (0);


          if (!flagDebugMode) {
            for (const auto& object: parseStringList(objects.str(), ' ')) {
              fs::remove_all(object);
            }
          }

          // mostly random build IDs and refs
          auto id = String("17D835592A262D7900") + std::to_string(rand64()).substr(0, 6);
          auto ref = String("17D835592A262D7900") + std::to_string(rand64()).substr(0, 6);

          xCodeProjectVariables["__ios_native_extensions_build_context_sections"] +=
            id + " /* " + lib.filename().string() + " */ = {"                     +
              ("isa = PBXBuildFile; ")                                            +
              ("fileRef = " + ref+ ";")                                           +
            "};\n"
          ;

          xCodeProjectVariables["__ios_native_extensions_build_context_refs"]        +=
            ref + " /* " + lib.filename().string() + " */ = {"                       +
              ("isa = PBXFileReference; ")                                           +
              ("name = \"" + lib.filename().string() +  "\"; ")                      +
              ("path = \"ui/socket/extensions/" + lib.filename().string() + "\"; ")  +
              ("sourceTree = \"<group>\"; ")                                         +
            "};\n"
          ;

          xCodeProjectVariables["__ios_native_extensions_build_ids"] += id + ",\n";
          xCodeProjectVariables["__ios_native_extensions_build_refs"] += ref + ",\n";
        }
      }

      if (settings.contains("ios_info_plist_file")) {
        const auto files = parseStringList(settings["ios_info_plist_file"]);
        for (const auto file : files) {
          settings["ios_info_plist_data"] += readFile(file);
        }
      }

      if (!settings.contains("ios_info_plist_data")) {
        settings["ios_info_plist_data"] = "";
      }

      settings["ios_info_plist_data"] += (
        "  <key>UIBackgroundModes</key>\n"
        "  <array>\n"
        "    <string>fetch</string>\n"
        "    <string>processing</string>\n"
      );

      if (settings["permissions_allow_bluetooth"] != "false") {
        settings["ios_info_plist_data"] += (
          "    <string>bluetooth-central</string>\n"
          "    <string>bluetooth-peripheral</string>\n"
        );
      }

      if (settings["permissions_allow_geolocation"] != "false") {
        settings["ios_info_plist_data"] += (
          "    <string>location</string>\n"
        );
      }

      if (settings["permissions_allow_push_notifications"] == "true") {
        settings["ios_info_plist_data"] += (
          "    <string>remote-notification</string>\n"
        );
      }

      settings["ios_info_plist_data"] += (
        "  </array>\n"
      );

      settings["ios_info_plist_data"] += (
        "  <key>UIRequiredDeviceCapabilities</key>\n"
        "  <array>\n"
      );

      if (settings["permissions_allow_bluetooth"] != "false") {
        settings["ios_info_plist_data"] += (
          "     <string>bluetooth-le</string>\n"
        );
      }

      if (settings["permissions_allow_geolocation"] != "false") {
        settings["ios_info_plist_data"] += (
          "     <string>gps</string>\n"
          "     <string>location-services</string>\n"
        );
      }

      if (settings["permissions_allow_sensors"] != "false") {
        settings["ios_info_plist_data"] += (
          "     <string>accelerometer</string>\n"
          "     <string>gyroscope</string>\n"
        );
      }

      if (settings["permission_allow_user_media"] != "false") {
        if (settings["permissions_allow_microphone"] != "false") {
          settings["ios_info_plist_data"] += (
            "     <string>microphone</string>\n"
          );
        }

        if (settings["permissions_allow_camera"] != "false") {
          settings["ios_info_plist_data"] += (
            "     <string>video-camera</string>\n"
          );
        }
      }

      settings["ios_info_plist_data"] += (
        "  </array>\n"
      );

      writeFile(paths.platformSpecificOutputPath / "exportOptions.plist", tmpl(gXCodeExportOptions, settings));
      writeFile(paths.platformSpecificOutputPath / "Info.plist", tmpl(gIOSInfoPList, settings));
      writeFile(pathToProject / "project.pbxproj", tmpl(gXCodeProject, xCodeProjectVariables));
      writeFile(pathToScheme / schemeName, tmpl(gXCodeScheme, settings));

      pathResources = paths.platformSpecificOutputPath / "ui";
      fs::create_directories(pathResources);
    }

    //
    // Linux Package Prep
    // ---
    //
    if (platform.linux && isForDesktop) {
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
    if (platform.win && isForDesktop) {
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
        for (String libString : split(Env::get("WIN_DEBUG_LIBS"), ',')) {
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

    handleBuildPhaseForUserScript(
      settings,
      targetPlatform,
      pathResourcesRelativeToUserBuild,
      oldCwd,
      additionalBuildArgs,
      true
    );

    auto copyMapFiles = handleBuildPhaseForCopyMappedFiles(
      settings,
      targetPlatform,
      pathResourcesRelativeToUserBuild
    );

    log("package prepared");

    auto SOCKET_HOME_API = Env::get("SOCKET_HOME_API");

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
      // SOCKET_HOME_API directory if distributed with npm
      fs::remove_all(pathResources / "socket" / "node_modules");
    }

    if (flagBuildForIOS) {
      if (flagBuildForSimulator && settings["ios_simulator_device"].size() == 0) {
        log("ERROR: [ios] simulator_device option is empty");
        exit(1);
      }

      if (flagBuildForSimulator) {
        log("building for iOS Simulator");
      } else {
        log("building for iOS");
      }

      auto pathToDist = oldCwd / paths.platformSpecificOutputPath;

      fs::create_directories(pathToDist);
      fs::create_directories(pathToDist / "core");
      fs::current_path(pathToDist);

      //
      // Copy and or create the source files we need for the build.
      //
      fs::copy(trim(prefixFile("src/init.cc")), pathToDist);
      fs::copy(trim(prefixFile("src/core/config.hh")), pathToDist / "core");
      fs::copy(trim(prefixFile("src/core/string.hh")), pathToDist / "core");
      fs::copy(trim(prefixFile("src/core/types.hh")), pathToDist / "core");
      fs::copy(trim(prefixFile("src/core/ini.hh")), pathToDist / "core");

      auto pathBase = pathToDist / "Base.lproj";
      fs::create_directories(pathBase);

      writeFile(pathBase / "LaunchScreen.storyboard", gStoryboardLaunchScreen);

      Map entitlementSettings;
      extendMap(entitlementSettings, settings);

      if (flagDebugMode) {
        entitlementSettings["configured_entitlements"] += (
          "  <key>get-task-allow</key>\n"
          "  <true/>\n "
        );

        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.security.cs.debugger</key>\n"
          "  <true/>\n "
        );
      }

      if (settings["permission_allow_user_media"] != "false") {
        if (settings["permission_allow_camera"] != "false") {
          entitlementSettings["configured_entitlements"] += (
              "  <key>com.apple.security.device.camera</key>\n"
              "  <true/>\n"
              );
        }

        if (settings["permission_allow_microphone"] != "false") {
          entitlementSettings["configured_entitlements"] += (
              "  <key>com.apple.security.device.microphone</key>\n"
              "  <true/>\n"
              );
        }
      }

      if (settings["permissions_allow_bluetooth"] != "false") {
        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.security.device.bluetooth</key>\n"
          "  <true/>\n"
        );
      }

      if (settings["permissions_allow_push_notifications"] == "true") {
        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.developer.usernotifications.filtering</key>\n"
          "  <true/>\n"
        );
        
        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.developer.location.push</key>\n"
          "  <true/>\n"
        );
      }

      if (settings["permissions_allow_geolocation"] != "false") {
        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.security.personal-information.location</key>\n"
          "  <true/>\n"
        );
      }

      if (settings["ios_sandbox"] != "false") {
        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.security.app-sandbox</key>\n"
          "  <true/>\n"
        );
      }

      writeFile(
        pathToDist / "socket.entitlements",
        tmpl(gXcodeEntitlements, entitlementSettings)
      );

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
        if (noDevice != String::npos) {
          log("ERROR: [ios] simulator_device " + settings["ios_simulator_device"] + " from your socket.ini was not found");
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

      if (flagShouldPackage && !flagBuildForSimulator) {
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
          if (flagVerboseMode) {
            log(rExport.output);
          }
          fs::current_path(oldCwd);
          exit(1);
        }

        log("exported archive");
      }

      log("completed");
      fs::current_path(oldCwd);
    }

    if (flagBuildForAndroid) {
      auto cwd = fs::current_path();
      auto app = paths.platformSpecificOutputPath / "app";
      auto androidHome = getAndroidHome();

      fs::current_path(paths.platformSpecificOutputPath);
      StringStream sdkmanager;
      StringStream packages;
      StringStream gradlew;
      String ndkVersion = "26.0.10792818";
      String androidPlatform = "android-34";

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

      if (Env::get("SSC_SKIP_ANDROID_SDK_MANAGER").size() == 0) {
        if (debugEnv || verboseEnv) {
          log(sdkmanager.str());
        }

        if (std::system(sdkmanager.str().c_str()) != 0) {
          log("WARNING: failed to initialize Android SDK (sdkmanager)");
        }
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
        auto src = app / "src";
        auto _main = src / "main";
        auto app_mk = _main / "jni" / "Application.mk";
        auto jni = _main / "jni";
        auto jniLibs = _main / "jniLibs";
        auto libs = _main / "libs";
        auto obj = _main / "obj";

        if (fs::exists(obj)) {
          fs::remove_all(obj);
        }

        // TODO(mribbons) - Copy specific abis
        fs::create_directories(libs);
        int androidStaticLibCount = 0;
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
              androidStaticLibCount++;
            } catch (fs::filesystem_error &e) {
              std::cerr << "Unable to copy android lib: " << fs::exists(dest) << ": " << e.what() << std::endl;
              throw;
            }
          }
        }

        if (androidStaticLibCount == 0) {
          log("ERROR: No android static libs copied, app won't build. Check " + prefixFile() + "lib");
          exit(1);
        }

        auto buildJniLibs = true;
        if (fs::exists(jniLibs)) {
          int androidSharedLibCount = 0;
          for (auto const& dir_entry : fs::recursive_directory_iterator(jniLibs)) {
            if (dir_entry.path().stem() == "libsocket-runtime.so") {
              // Count the number of libsocket-runtime.so's in jniLibs, there will be one for each android architecture
              androidSharedLibCount++;
            }
          }

          // if we have found libsocket-runtime.so, we don't need to recompile, unless:
          // libsocket-runtime.so count doesn't match the static lib count - There should be one libuv.a and libsocket-runtime.a for each android architecture
          if (androidSharedLibCount > 0 && androidSharedLibCount != (androidStaticLibCount / 2)) {
            buildJniLibs = true;
            log("WARNING: Android Shared Lib Count is incorrect, forcing rebuild.");
            fs::remove_all(jniLibs);
          } else {
            buildJniLibs = false;
          }
        }

        fs::create_directories(jniLibs);

        // don't build unless we're sure it is required, ndkbuild errors out if jniLibs is already populated
        if (buildJniLibs) {
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
      if (Env::get("SSC_CI").size() > 0) {
        StringStream gradlew;
        gradlew << localDirPrefix << "gradlew build";

        if (std::system(gradlew.str().c_str()) != 0) {
          log("ERROR: failed to invoke `gradlew build` command");
          exit(1);
        }

        exit(0);
      }

      // Check for gradle in pwd. Don't fail, this is just for support.
      if (!fs::exists(String("gradlew") + (platform.win ? ".bat" : ""))) {
        log("WARNING: gradlew script not in pwd: " + fs::current_path().string());
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

      androidState.androidHome = androidHome;
      androidState.verbose = debugEnv || verboseEnv;
      androidState.devNull = devNull;
      androidState.platform = androidPlatform;
      androidState.appPath = app;
      androidState.quote = quote;
      androidState.slash = slash;
      fs::current_path(cwd);
    }

    auto extraFlags = flagDebugMode
      ? settings.count("debug_flags") ? settings["debug_flags"] : ""
      : settings.count("build_flags") ? settings["build_flags"] : "";

    quote = "";
    if (platform.win && Env::get("CXX").find(" ") != SSC::String::npos) {
      quote = "\"";
    }

    // build desktop extension
    if (isForDesktop) {
      static const auto IN_GITHUB_ACTIONS_CI = Env::get("GITHUB_ACTIONS_CI").size() == 0;
      auto oldCwd = fs::current_path();
      fs::current_path(targetPath);

      StringStream compileCommand;

      fs::create_directories(paths.pathResourcesRelativeToUserBuild / "socket" / "extensions");

      auto extensions = Vector<String>();
      for (const auto& tuple : settings) {
        auto& key = tuple.first;
        if (tuple.second.size() == 0) continue;
        if (key.starts_with("build_extensions_")) {
          if (key.find("compiler_flags") != String::npos) continue;
          if (key.find("compiler_debug_flags") != String::npos) continue;
          if (key.find("linker_flags") != String::npos) continue;
          if (key.find("linker_debug_flags") != String::npos) continue;
          if (key.find("configure_script") != String::npos) continue;
          if (key.find("build_script") != String::npos) continue;
          if (key.starts_with("build_extensions_ios_")) continue;
          if (key.starts_with("build_extensions_android_")) continue;
          if (key.ends_with("_path")) continue;

          if (!platform.win && key.starts_with("build_extensions_win_")) continue;
          if (!platform.mac && key.starts_with("build_extensions_mac_")) continue;
          if (!platform.linux && key.starts_with("build_extensions_linux_")) continue;

          String extension;
          auto os = replace(replace(platform.os, "win32", "win"), "macos", "mac");;
          if (key.starts_with("build_extensions_" + os)) {
            extension = replace(key, "build_extensions_" + os + "_", "");
          } else {
            extension = replace(key, "build_extensions_", "");
          }

          extension = split(extension, '_')[0];

          auto source = settings["build_extensions_" + extension + "_source"];

          if (source.size() == 0) {
            source = settings["build_extensions_" + extension];
            if (source.size() > 0) {
              if (fs::is_directory(source)) {
                settings["build_extensions_" + extension + "_path"] = (targetPath / source).string();
                settings["build_extensions_" + extension] = "";
              }
            }
          }

          if (source.size() > 0) {
            Path target;
            if (fs::exists(source)) {
              target = targetPath / source;
            } else if (source.ends_with(".git")) {
              auto path = Path { source };
              target = paths.platformSpecificOutputPath / "extensions" / replace(path.filename().string(), ".git", "");

              if (!fs::exists(target)) {
                auto exitCode = std::system(("git clone " + source + " " + target.string()).c_str());

                if (exitCode) {
                  exit(exitCode);
                }
              }
            }

            if (fs::exists(target) && fs::is_directory(target)) {
              target = fs::canonical(target);

              auto configFile = target / "socket.ini";
              auto config = INI::parse(fs::exists(configFile) ? readFile(configFile) : "");
              settings["build_extensions_" + extension + "_path"] = target.string();
              fs::current_path(target);

              for (const auto& entry : config) {
                if (entry.first.starts_with("extension_sources")) {
                  const auto sources = parseStringList(entry.second, ' ');
                  Vector<String> canonical;
                  for (const auto& source : sources) {
                    try {
                      canonical.push_back(fs::canonical(target / source).string());
                    } catch (const std::filesystem::filesystem_error& e) {
                      canonical.push_back((target / source).string());
                    }
                  }

                  settings["build_extensions_" + extension] = join(canonical, " ");
                } else if (entry.first.starts_with("extension_")) {
                  auto key = replace(entry.first, "extension_", extension + "_");
                  auto value = entry.second;
                  if (key.ends_with("_flags")) {
                    // Replace all $(…) with evaluated stdout.
                    // E.g. $(pkg-config --libs --cflags libssl) => -lssl
                    auto match = std::smatch{};
                    while (std::regex_search(value, match, std::regex("\\$\\((.*?)\\)"))) {
                      auto subcommand = match[1].str();
                      if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
                        log("Running subcommand: " + subcommand);
                      }
                      auto proc = exec(subcommand);
                      if (proc.exitCode != 0) {
                        log("ERROR: failed to run subcommand: " + subcommand);
                        exit(proc.exitCode);
                      }
                      auto output = trim(replace(proc.output, "\n", " "));
                      value = value.replace(match[0].first, match[0].second, output);
                    }
                    // Replace all $\w+ with env var.
                    // E.g. $CXX => clang++
                    match = std::smatch{};
                    while (std::regex_search(value, match, std::regex("\\$(\\w+)"))) {
                      auto envVar = match[1].str();
                      auto envValue = envVar == "PWD"
                        ? target.string() // Use the extension root.
                        : Env::get(envVar);
                      if (envValue.size() == 0) {
                        log("ERROR: failed to find env var: " + envVar);
                        exit(1);
                      }
                      value = value.replace(match[0].first, match[0].second, envValue);
                    }
                    // Replace all ./ and ../ with absolute paths.
                    match = std::smatch{};
                    while (std::regex_search(value, match, std::regex("\\.\\.?/([^ ]|\\\\ )+"))) {
                      auto relativePath = match[0].str();
                      auto absolutePath = fs::absolute(target / relativePath);
                      try {
                        absolutePath = fs::canonical(absolutePath);
                        value = value.replace(
                          match[0].first,
                          match[0].second,
                          absolutePath.string()
                        );
                      } catch (const std::filesystem::filesystem_error& e) {
                        if (e.code() == std::errc::no_such_file_or_directory) {
                          log("WARNING: path not found: " + absolutePath.string());
                          break;
                        } else {
                          throw e;
                        }
                      }
                    }
                  }
                  auto index = "build_extensions_" + key;
                  if (settings[index].size() > 0) {
                    settings[index] += " " + value;
                  } else {
                    settings[index] = value;
                  }
                }
              }

              if (settings["build_extensions_" + extension].size() == 0) {
                log(
                  "\033[33mWARN\033[0m " + key + " has no sources, ignoring: " +
                  fs::canonical(configFile).string()
                );
              }
            }
          }

          auto configure = settings["build_extensions_" + extension + "_configure_script"];
          auto build = settings["build_extensions_" + extension + "_build_script"];
          auto copy = settings["build_extensions_" + extension + "_build_copy"];
          auto path = settings["build_extensions_" + extension + "_path"];

          auto sources = parseStringList(
            trim(settings["build_extensions_" + extension] + " " + settings["build_extensions_" + extension + "_" + os]),
            ' '
          );

          auto objects = StringStream();
          auto lib = (paths.pathResourcesRelativeToUserBuild / "socket" / "extensions" / extension / (extension + RUNTIME_EXTENSION_FILE_EXT));

          fs::create_directories(lib.parent_path());

          if (path.size() > 0) {
            fs::current_path(targetPath);
            fs::current_path(path);
          } else {
            fs::current_path(targetPath);
          }

          if (build.size() > 0) {
            auto exitCode = std::system((build + argvForward).c_str());
            if (exitCode) {
              exit(exitCode);
            }
          }

          if (configure.size() > 0) {
            SSC::ExecOutput result = exec(configure + argvForward);
            if (result.exitCode != 0) {
              log("ERROR: failed to configure extension: " + extension);
              log(result.output);
              exit(result.exitCode);
            }
            auto output = replace(result.output, "\n", " ");
            if (output.size() > 0) {
              for (const auto& source : parseStringList(output, ' ')) {
                sources.push_back(source);
              }
            }
          }

          if (copy.size() > 0) {
            auto pathResources = paths.pathResourcesRelativeToUserBuild;
            for (const auto& file : parseStringList(copy, ' ')) {
              auto parts = split(file, ':');
              auto target = parts[0];
              auto destination = parts.size() == 2 ? pathResources / parts[1] : pathResources;
              fs::copy(
                target,
                destination,
                fs::copy_options::update_existing | fs::copy_options::recursive
              );
            }
          }

          if (sources.size() == 0) {
            continue;
          }

          if (std::find(extensions.begin(), extensions.end(), extension) == extensions.end()) {
            log("Building extension: " + extension + " (" + platform.os +  "-" + platform.arch + ")");
            extensions.push_back(extension);
          }

          sources.push_back(
            trim(prefixFile("src/init.cc"))
          );

          for (auto source : sources) {
            if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
              log("extension source: " + source);
            }

            auto compilerFlags = (
              settings["build_extensions_compiler_flags"] + " " +
              settings["build_extensions_" + os + "_compiler_flags"] + " " +
              settings["build_extensions_" + extension + "_compiler_flags"] + " " +
              settings["build_extensions_" + extension + "_" + os + "_compiler_flags"] + " "
            );

            auto compilerDebugFlags = (
              settings["build_extensions_compiler_debug_flags"] + " " +
              settings["build_extensions_" + os + "_compiler_debug_flags"] + " " +
              settings["build_extensions_" + extension + "_compiler_debug_flags"] + " " +
              settings["build_extensions_" + extension + "_" + os + "_compiler_debug_flags"] + " "
            );

            if (platform.mac) {
              compilerFlags += " -framework UniformTypeIdentifiers";
              compilerFlags += " -framework CoreBluetooth";
              compilerFlags += " -framework CoreLocation";
              compilerFlags += " -framework Network";
              compilerFlags += " -framework UserNotifications";
              compilerFlags += " -framework WebKit";
              compilerFlags += " -framework Cocoa";
              compilerFlags += " -framework OSLog";
            }

            if (platform.win && debugBuild) {
              compilerDebugFlags += "-D_DEBUG";
            }

            auto CXX = Env::get("CXX");
            auto CC = Env::get("CC");
            String compiler;

            if (source.ends_with(".hh") || source.ends_with(".h")) {
              continue;
            } else if (source.ends_with(".cc") || source.ends_with(".cpp") || source.ends_with(".c++") || source.ends_with(".mm")) {
              compiler = CXX.size() > 0 && !IN_GITHUB_ACTIONS_CI ? CXX : "clang++";
              compilerFlags += " -v";
              compilerFlags += " -std=c++2a -v";
              if (platform.mac) {
                compilerFlags += " -ObjC++";
              } else if (platform.win) {
                compilerFlags += " -stdlib=libstdc++";
              }

              if (platform.win || platform.linux) {
                compilerFlags += " -Wno-unused-command-line-argument";
              }

              if (compiler.ends_with("clang++")) {
                compiler = compiler.substr(0, compiler.size() - 2);
              } else if (compiler.ends_with("clang++.exe")) {
                compiler = compiler.substr(0, compiler.size() - 6) + ".exe";
              } else if (compiler.ends_with("g++")) {
                compiler = compiler.substr(0, compiler.size() - 2) + "cc";
              } else if (compiler.ends_with("g++.exe")) {
                compiler = compiler.substr(0, compiler.size() - 6) + "cc.exe";
              }
            } else if (source.ends_with(".o") || source.ends_with(".a")) {
              objects << source << " ";
              continue;
            } else if (source.ends_with(".c") || source.ends_with(".m")) {
              compiler = CC.size() > 0 ? CC : "clang";
              if (platform.mac) {
                compilerFlags += " -ObjC -v";
              }
            } else {
              continue;
            }

            if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
              log("extension source: " + source);
            }

            auto objectFile = source;
            objectFile = replace(objectFile, "\\.mm", ".o");
            objectFile = replace(objectFile, "\\.m", ".o");
            objectFile = replace(objectFile, "\\.cc", ".o");
            objectFile = replace(objectFile, "\\.c", ".o");

            auto object = Path(objectFile);

            objects << (quote + object.string() + quote) << " ";
            auto compileExtensionObjectCommand = StringStream();
            compileExtensionObjectCommand
              << quote // win32 - quote the entire command
              << quote // win32 - quote the binary path
              << compiler
              << quote // win32 - quote the binary path
              << " -I" + Path(paths.platformSpecificOutputPath / "include").string()
              << (" -I" + quote + trim(prefixFile("include")) + quote)
              << (" -I" + quote + trim(prefixFile("src")) + quote)
              << (" -L" + quote + trim(prefixFile("lib")) + quote)
            #if defined(_WIN32)
              << (" -L" + quote + trim(prefixFile("lib\\" + platform.arch + "-desktop")) + quote)
              << " -D_MT"
              << " -D_DLL"
              << " -DWIN32"
              << " -DWIN32_LEAN_AND_MEAN"
              << " -Xlinker /NODEFAULTLIB:libcmt"
              << " -Wno-nonportable-include-path"
            #else
              << (" -L" + quote + trim(prefixFile("lib/" + platform.arch + "-desktop")) + quote)
            #endif
              << " -fvisibility=hidden"
              << " -DIOS=0"
              // << " -U__CYGWIN__"
              << " -DANDROID=0"
              << " -DDEBUG=" << (flagDebugMode ? 1 : 0)
              << " -DHOST=" << devHost
              << " -DPORT=" << devPort
              << " -DSSC_VERSION=" << SSC::VERSION_STRING
              << " -DSSC_VERSION_HASH=" << SSC::VERSION_HASH_STRING
            #if !defined(_WIN32)
              << " -fPIC"
            #endif
              << " " << trim(compilerFlags + " " + (flagDebugMode ? compilerDebugFlags : ""))
              << " -c " << (quote + source + quote)
              << " -o " << (quote + object.string() + quote)
              << quote
            ;

            struct stat sourceStats;
            struct stat objectStats;
            struct stat libraryStats;

            if (fs::exists(object)) {
              if (stat(convertWStringToString(source).c_str(), &sourceStats) == 0) {
                if (stat(convertWStringToString(object).c_str(), &objectStats) == 0) {
                  if (objectStats.st_mtime > sourceStats.st_mtime) {
                    continue;
                  }
                }
              }

              if (stat(convertWStringToString(source).c_str(), &sourceStats) == 0) {
                if (stat(convertWStringToString(lib).c_str(), &libraryStats) == 0) {
                  if (libraryStats.st_mtime > sourceStats.st_mtime) {
                    continue;
                  }
                }
              }
            }

            if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
              log(compileExtensionObjectCommand.str());
            }

            do {
              auto r = exec(compileExtensionObjectCommand.str());

              if (r.exitCode != 0) {
                log("Unable to build extension object (" + object.string() + ")");
                log(r.output);
                exit(r.exitCode);
              }
            } while (0);
          }

          auto linkerFlags = (
            settings["build_extensions_linker_flags"] + " " +
            settings["build_extensions_" + os + "_linker_flags"] + " " +
            settings["build_extensions_" + extension + "_linker_flags"] + " " +
            settings["build_extensions_" + extension + "_" + os + "_linker_flags"] + " "
          );

          auto linkerDebugFlags = (
            settings["build_extensions_linker_debug_flags"] + " " +
            settings["build_extensions_" + platform.os + "_linker_debug_flags"] + " " +
            settings["build_extensions_" + extension + "_linker_debug_flags"] + " " +
            settings["build_extensions_" + extension + "_" + os + "_linker_debug_flags"] + " "
          );

          if (platform.win && debugBuild) {
            linkerDebugFlags += "-D_DEBUG";
            for (String libString : split(Env::get("WIN_DEBUG_LIBS"), ',')) {
              if (libString.size() > 0) {
                if (libString[0] == '\"' && libString[libString.size()-2] == '\"') {
                  libString = libString.substr(1, libString.size()-2);
                }

                Path lib(libString);
                if (!fs::exists(lib)) {
                  log("WARNING: WIN_DEBUG_LIBS: File doesn't exist, aborting build: " + lib.string());
                  exit(1);
                } else {
                  linkerDebugFlags += " " + lib.string();
                }
              }
            }
          }

        #if defined(_WIN32)
          auto d = String(debugBuild ? "d" : "");
          auto static_uv = prefixFile("lib" + d + "\\" + platform.arch + "-desktop\\libuv.lib");
          auto static_runtime = trim(prefixFile("lib" + d + "\\" + platform.arch + "-desktop\\libsocket-runtime" + d + ".a"));
        #else
          auto d = "";
          auto static_uv = "";
          auto static_runtime = "";
        #endif

          auto compileExtensionLibraryCommand = StringStream();
          compileExtensionLibraryCommand
            << quote // win32 - quote the entire command
            << quote // win32 - quote the binary path
            << Env::get("CXX")
            << quote // win32 - quote the binary path
            << " " << static_runtime
            << " " << static_uv
            << " " << objects.str()
          #if defined(_WIN32)
            << (" -L" + quote + trim(prefixFile("lib\\" + platform.arch + "-desktop")) + quote)
            << " -D_MT"
            << " -D_DLL"
            << " -DWIN32"
            << " -DWIN32_LEAN_AND_MEAN"
            << " -Xlinker /NODEFAULTLIB:libcmt"
            << " -Wno-nonportable-include-path"
          #else
            << " " << flags
            << " " << extraFlags
          #if defined(__linux__)
            << " -luv"
            << " -lsocket-runtime"
          #endif
            << (" -L" + quote + trim(prefixFile("lib/" + platform.arch + "-desktop")) + quote)
          #endif
            << " -fvisibility=hidden"
            << " " << trim(linkerFlags + " " + (flagDebugMode ? linkerDebugFlags : ""))
            << (" -I" + quote + trim(prefixFile("include")) + quote)
            << (" -I" + quote + trim(prefixFile("src")) + quote)
            << (" -L" + quote + trim(prefixFile("lib")) + quote)
            << " -shared"
            << " -std=c++2a"
            <<" -I" + Path(paths.platformSpecificOutputPath / "include").string()
          #if defined(__linux__)
            << " -fPIC"
          #endif
            << " -o " << (quote + lib.string() + quote)
            << quote // win32 - quote the entire command
          ;

          if (platform.mac) {
            if (isForDesktop) {
              settings["mac_codesign_paths"] += (paths.pathResourcesRelativeToUserBuild / "socket" / "extensions" / extension / (extension + RUNTIME_EXTENSION_FILE_EXT)).string() + ";";
            }
          }

          if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1") {
            log(compileExtensionLibraryCommand.str());
          }

          fs::create_directories(lib.parent_path());
          do {
            auto r = exec(compileExtensionLibraryCommand.str());

            if (r.exitCode != 0) {
              log("Unable to build extension (" + extension + ")");
              log(r.output);
              exit(r.exitCode);
            }
          } while (0);

          if (!flagDebugMode) {
            for (const auto& object: parseStringList(objects.str(), ' ')) {
              fs::remove_all(object);
            }
          }
        }
      }

      fs::current_path(oldCwd);
    }

    if (flagRunUserBuildOnly == false && isForDesktop && !isForExtensionOnly) {
      StringStream compileCommand;

      // windows / spaces in bin path - https://stackoverflow.com/a/27976653/3739540
      compileCommand
        << quote // win32 - quote the entire command
        << quote // win32 - quote the binary path
        << Env::get("CXX")
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

      if (Env::get("DEBUG") == "1" || Env::get("VERBOSE") == "1")
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
    if (flagShouldPackage && platform.linux && isForDesktop) {
      auto packageFormat = settings["build_linux_package_format"];

      if (optionsWithValue["--package-format"].size()) {
        packageFormat = optionsWithValue["--package-format"];
      }

      if (packageFormat.size() == 0) {
        packageFormat = "deb";
      }

      if (packageFormat == "deb") {
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

        if (debugEnv || verboseEnv) {
          log(archiveCommand.str());
        }

        auto r = exec(archiveCommand.str());

        if (r.exitCode != 0) {
          log("ERROR: Build packaging failed for Linux");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }
      } else if (packageFormat == "zip") {
        StringStream zipCommand;

        zipCommand
          << "zip -r"
          << " " << (paths.platformSpecificOutputPath / (settings["build_name"] + ".zip")).string()
          << " " << pathResourcesRelativeToUserBuild.string()
          ;

        if (debugEnv || verboseEnv) {
          log(zipCommand.str());
        }

        auto r = exec(zipCommand.str());

        if (r.exitCode != 0) {
          log("ERROR: Build packaging failed for Linux");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }
      } else {
        log("ERROR: Unknown package format given in '[build.linux.package] format = \"" + packageFormat + "\"");
        exit(1);
      }
    }

    //
    // MacOS Stripping
    //
    if (platform.mac && isForDesktop && !isForExtensionOnly) {
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
    if (flagCodeSign && platform.mac && isForDesktop) {
      //
      // https://www.digicert.com/kb/code-signing/mac-os-codesign-tool.htm
      // https://developer.apple.com/forums/thread/128166
      // https://wiki.lazarus.freepascal.org/Code_Signing_for_macOS
      //
      Path pathBase = "Contents";
      StringStream signCommand;
      String entitlements = "";

      Map entitlementSettings;
      extendMap(entitlementSettings, settings);

      if (settings["permission_allow_user_media"] != "false") {
        if (settings["permission_allow_camera"] != "false") {
          entitlementSettings["configured_entitlements"] += (
            "  <key>com.apple.security.device.camera</key>\n"
            "  <true/>\n"
          );
        }

        if (settings["permission_allow_microphone"] != "false") {
          entitlementSettings["configured_entitlements"] += (
            "  <key>com.apple.security.device.microphone</key>\n"
            "  <true/>\n"
            "  <key>com.apple.security.device.audio-input</key>\n"
            "  <true/>\n"
          );
        }
      }

      if (settings["permissions_allow_bluetooth"] != "false") {
        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.security.device.bluetooth</key>\n"
          "  <true/>\n"
        );
      }

      if (settings["permissions_allow_geolocation"] != "false") {
        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.security.personal-information.location</key>\n"
          "  <true/>\n"
        );
      }

      if (settings["apple_team_identifier"].size() > 0) {
        auto identifier = (
          settings["apple_team_identifier"] +
          "." +
          settings["meta_bundle_identifier"]
        );

        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.application-identifier</key>\n"
          "  <string>" + identifier  + "</string>\n"
        );
      }

      if (settings["mac_sandbox"] != "false") {
        entitlementSettings["configured_entitlements"] += (
          "  <key>com.apple.security.app-sandbox</key>\n"
          "  <true/>\n"
        );
      }

      writeFile(
        paths.platformSpecificOutputPath / "socket.entitlements",
        tmpl(gXcodeEntitlements, entitlementSettings)
      );

      if (settings.count("mac_codesign_identity") == 0) {
        log("'[mac.codesign] identity' key/value is required");
        exit(1);
      }

      StringStream commonFlags;

      commonFlags
        << " --force"
        << " --options runtime"
        << " --timestamp"
        << " --entitlements " << (paths.platformSpecificOutputPath / "socket.entitlements").string()
        << " --identifier '" << settings["meta_bundle_identifier"] << "'"
        << " --sign '" << settings["mac_codesign_identity"] << "'"
        << " "
      ;

      if (settings["mac_codesign_paths"].size() > 0) {
        auto paths = split(settings["mac_codesign_paths"], ';');

        for (int i = 0; i < paths.size(); i++) {
          String prefix = (i > 0) ? ";" : "";

          signCommand
            << prefix
            << "codesign "
            << commonFlags.str()
            << (pathResources / paths[i]).string()
          ;
        }
        signCommand << "&& ";
      }

      signCommand
        << "codesign"
        << commonFlags.str()
        << binaryPath.string()

        << " && codesign"
        << commonFlags.str()
        << paths.pathPackage.string();

      if (flagDebugMode) {
        log(signCommand.str());
      }

      auto r = exec(signCommand.str());

      if (r.output.size() > 0) {
        if (r.exitCode != 0) {
          log("ERROR: Unable to sign application with 'codesign'");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }
      }

      if (flagVerboseMode) {
        log(r.output);
      }

      log("Successfully code signed app with 'codesign'");
    }

    if (platform.mac && isForDesktop) {
      auto packageFormat = settings["build_mac_package_format"];

      if (optionsWithValue["--package-format"].size()) {
        packageFormat = optionsWithValue["--package-format"];
      }

      if (packageFormat.size() == 0) {
        packageFormat = "zip";
        settings["build_mac_package_format"] = packageFormat;
      }

      pathToArchive = paths.platformSpecificOutputPath / (settings["build_name"] + "." + packageFormat);
      if (!flagShouldPackage && flagShouldNotarize && !fs::exists(pathToArchive)) {
        flagShouldPackage = true;
      }
    }

    //
    // MacOS Packaging
    // ---
    //
    if (flagShouldPackage && platform.mac && isForDesktop) {
      auto packageFormat = settings["build_mac_package_format"];

      if (optionsWithValue["--package-format"].size()) {
        packageFormat = optionsWithValue["--package-format"];
      }

      if (packageFormat == "zip") {
        StringStream zipCommand;
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

        auto r = exec(zipCommand.str());

        if (r.exitCode != 0) {
          log("ERROR: Build packaging fails for macOS");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }
      } else if (packageFormat == "pkg") {
        StringStream productBuildCommand;
        auto identity = settings["mac_productbuild_identity"];
        auto productBuildInstallPath = settings["mac_productbuild_install_path"];

        if (identity.size() == 0) {
          log("ERROR: Missing '[mac.productbuild] identity = ...' in 'socket.ini'");
          exit(1);
        }

        if (productBuildInstallPath.size() == 0) {
          productBuildInstallPath = "/Applications";
        }

        auto productBuildOutput = (paths.platformSpecificOutputPath / Path(settings["build_name"] + ".pkg")).string();
        productBuildCommand
          << "xcrun productbuild"
          << " --sign '" << identity << "'"
          << " --version '" << settings["meta_version"] << "'"
          << " --component '" << paths.pathPackage.string() << "'" << " " << productBuildInstallPath
          << " --identifier '" << settings["meta_bundle_identifier"] << "'"
          << " --timestamp"
          << " " << productBuildOutput;

        if (verboseEnv) {
          log(productBuildCommand.str());
        }

        auto r = exec(productBuildCommand.str());

        if (r.exitCode != 0) {
          log("ERROR: Failed to package macOS application with 'productbuild'");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }

        if (verboseEnv) {
          log(r.output);
        }

        r = exec(String("xcrun pkgutil --check-signature ") + productBuildOutput);

        if (r.exitCode != 0) {
          log("ERROR: Failed to verify macOS package signature with 'pkgutil'");
          if (flagDebugMode) {
            log(r.output);
          }
          exit(r.exitCode);
        }

        if (verboseEnv) {
          log(r.output);
        }
      } else {
        log("ERROR: Unknown package format given in '[build.mac.package] format = \"" + packageFormat + "\"");
        exit(1);
      }

      log("created package artifact");
    }

    //
    // MacOS Notarization
    // ---
    //
    if (flagShouldNotarize && platform.mac && isForDesktop) {
      StringStream notarizeCommand;
      String username = Env::get("APPLE_ID");
      String password = Env::get("APPLE_ID_PASSWORD");

      if (username.size() == 0) {
        username = settings["apple_identifier"];
      }

      if (username.size() == 0) {
        log(
          "ERROR: AppleID identifier could not be determined. "
          "Please set '[apple] identifier = ...' or 'APPLE_ID' environment variable."
        );
        exit(1);
      }

      if (password.size() == 0) {
        log(
          "ERROR: AppleID identifier could not be determined. "
          "Please set the 'APPLE_ID_PASSWORD' environment variable."
        );
        exit(1);
      }

      if (!fs::exists(pathToArchive)) {
        log(
          "ERROR: Cannot notarize application: Package archive does not exists."
        );
        exit(1);
      }

      notarizeCommand
        << "xcrun"
        << " notarytool submit"
        << " --wait"
        << " --apple-id \"" << username << "\""
        << " --password \"" << password << "\"";

      if (settings["apple_team_identifier"].size() > 0) {
        notarizeCommand << " --team-id " << settings["apple_team_identifier"];
      }

      notarizeCommand << " \"" << pathToArchive.string() << "\"";

      if (flagDebugMode) {
        log(notarizeCommand.str());
      }

      auto r = exec(notarizeCommand.str().c_str());

      if (r.exitCode != 0) {
        log("Unable to notarize macOS application");

        if (flagDebugMode) {
          log(r.output);
        }

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
    if (flagShouldPackage && platform.win && isForDesktop) {
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

      WString appx(SSC::convertStringToWString(paths.pathPackage.string()) + L".appx");

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
                log("mimetype?: " + convertWStringToString(mime));
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
        String msg = convertWStringToString( err.ErrorMessage() );
        log("Unable to save package; " + msg);

        exit(1);
      }

      #endif
    }


    //
    // Windows Code Signing
    //
    if (flagCodeSign && platform.win && isForDesktop) {
      //
      // https://www.digicert.com/kb/code-signing/signcode-signtool-command-line.htm
      //
      auto sdkRoot = Path("C:\\Program Files (x86)\\Windows Kits\\10\\bin");
      auto pathToSignTool = Env::get("SIGNTOOL");

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
      String password = Env::get("CSC_KEY_PASSWORD");

      if (password.size() == 0) {
        log("ERROR: Env variable 'CSC_KEY_PASSWORD' is empty!");
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

    if (flagShouldWatch) {
      Vector<String> sources;

      if (settings.contains("build_watch_sources")) {
        sources = parseStringList(trim(settings["build_watch_sources"]));
      } else if (copyMapFiles.size() > 0) {
        for (const auto& file : copyMapFiles) {
          sources.push_back(file.string());
        }
      }

      // allow changes to 'socket.ini' to be observed
      sources.push_back("socket.ini");

      FileSystemWatcher* sourcesWatcher = new FileSystemWatcher(sources);
      auto watchingSources = sourcesWatcher->start([=](
        const String& path,
        const Vector<FileSystemWatcher::Event>& events,
        const FileSystemWatcher::Context& context
      ) {
        auto settingsForSourcesWatcher = settings;
        extendMap(
          settingsForSourcesWatcher,
          INI::parse(readFile(targetPath / "socket.ini"))
        );

        handleBuildPhaseForUserScript(
          settingsForSourcesWatcher,
          targetPlatform,
          pathResourcesRelativeToUserBuild,
          targetPath,
          additionalBuildArgs,
          false
        );

        handleBuildPhaseForCopyMappedFiles(
          settingsForSourcesWatcher,
          targetPlatform,
          pathResourcesRelativeToUserBuild
        );

        log("File '" + path + "' did change");
      });

      if (!watchingSources) {
        log("Unable to start watching");
        exit(1);
      }

      log("Watching for changes in: " + join(sources, ","));
    }

    int exitCode = 0;
    if (flagShouldRun) {
      run(targetPlatform, settings, paths, flagDebugMode, flagRunHeadless, argvForward, androidState);
    }

    exit(exitCode);
  });

  createSubcommand("run", runOptions, true, [&](Map optionsWithValue, std::unordered_set<String> optionsWithoutValue) -> void {
    String argvForward = "";
    bool flagRunHeadless = optionsWithoutValue.find("--headless") != optionsWithoutValue.end();
    bool flagTest = optionsWithoutValue.find("--test") != optionsWithoutValue.end() || optionsWithValue["--test"].size() > 0;
    String targetPlatform = optionsWithValue["--platform"];
    String testFile = optionsWithValue["--test"];
    bool isForIOS = false;
    bool isForAndroid = false;

    if (flagTest && testFile.size() == 0) {
      log("ERROR: --test value is required.");
      exit(1);
    }

    if (testFile.size() > 0) {
      argvForward += " --test=" + testFile;
    }

    if (flagRunHeadless) {
      argvForward += " --headless";
    }

    if (targetPlatform.size() > 0) {
      if (targetPlatform == "ios" || targetPlatform == "ios-simulator") {
        isForIOS = true;
      } else if (targetPlatform == "android" || targetPlatform == "android-emulator") {
        isForAndroid = true;
      } else {
        log("Unknown platform: " + targetPlatform);
        exit(1);
      }
    } else {
      targetPlatform = platform.os;
    }

    String devHost = "localhost";
    if (optionsWithValue.count("--host") > 0) {
      devHost = optionsWithValue["--host"];
    } else {
      if (isForIOS || isForAndroid) {
        auto r = exec((!platform.win)
          ? "ifconfig | grep -w 'inet' | awk '!match($2, \"^127.\") {print $2; exit}' | tr -d '\n'"
          : "PowerShell -Command ((Get-NetIPAddress -AddressFamily IPV4).IPAddress ^| Select-Object -first 1)"
        );

        if (r.exitCode == 0) {
          devHost = r.output;
        }
      }
    }
    settings.insert(std::make_pair("host", devHost));

    String devPort = "0";
    if (optionsWithValue.count("--port") > 0) {
      devPort = optionsWithValue["--port"];
    }
    settings.insert(std::make_pair("port", devPort));

    const bool debugEnv = (
      Env::get("SSC_DEBUG").size() > 0 ||
      Env::get("DEBUG").size() > 0
    );

    const bool verboseEnv = (
      Env::get("SSC_VERBOSE").size() > 0 ||
      Env::get("VERBOSE").size() > 0
    );

    Paths paths = getPaths(targetPlatform);

    auto devNull = ">" + SSC::String((!platform.win) ? "/dev/null" : "NUL") + (" 2>&1");
    String quote = !platform.win ? "'" : "\"";
    String slash = !platform.win ? "/" : "\\";

    auto androidPlatform = "android-34";
    AndroidCliState androidState;
    androidState.androidHome = getAndroidHome();
    androidState.verbose = debugEnv || verboseEnv;
    androidState.devNull = devNull;
    androidState.targetPlatform = targetPlatform;
    androidState.platform = androidPlatform;
    androidState.appPath = paths.platformSpecificOutputPath / "app";
    androidState.quote = quote;
    androidState.slash = slash;

    run(targetPlatform, settings, paths, false, flagRunHeadless, argvForward, androidState);
  });

  // first flag indicating whether option is optional
  // second flag indicating whether option should be followed by a value
  Options setupOptions = {
    { { "--platform" }, true, true },
    { { "--yes", "-y" }, true, false },
    { { "--quiet", "-q" }, true, false },
    { { "--debug", "-D" }, true, false },
    { { "--verbose", "-V" }, true, false },
  };
  createSubcommand("setup", setupOptions, false, [&](Map optionsWithValue, std::unordered_set<String> optionsWithoutValue) -> void {
    auto help = false;
    auto yes = optionsWithoutValue.find("--yes") != optionsWithoutValue.end();
    String yesArg;

    String targetPlatform = optionsWithValue["--platform"];
    auto targetAndroid = false;
    auto targetLinux = false;
    auto targetWindows = false;

    // Note that multiple --platforms aren't supported by createSubcommand()
    if (equal(targetPlatform, "android")) {
      targetAndroid = true;
    } else if (equal(targetPlatform, "windows")) {
      targetWindows = true;
    } else if (equal(targetPlatform, "linux")) {
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

    if (optionsWithoutValue.find("--quiet") != optionsWithoutValue.end() || equal(rc["build_quiet"], "true")) {
      flagQuietMode = true;
    }

    log("Running setup for platform '" + targetPlatform + "' in " + "SOCKET_HOME (" + prefixFile() + ")");
    String command = scriptHost + " \"" + script.string() + "\" " + argument + " " + yesArg;
    auto r = std::system(command.c_str());

    exit(r);
  });

  createSubcommand("env", {}, true, [&](Map optionsWithValue, std::unordered_set<String> optionsWithoutValue) -> void {
    auto envs = Map();

    envs["DEBUG"] = Env::get("DEBUG");
    envs["VERBOSE"] = Env::get("VERBOSE");

    // runtime variables
    envs["SOCKET_HOME"] = getSocketHome(false);
    envs["SOCKET_HOME_API"] = Env::get("SOCKET_HOME_API");

    // platform OS variables
    envs["PWD"] = Env::get("PWD");
    envs["HOME"] = Env::get("HOME");
    envs["LANG"] = Env::get("LANG");
    envs["USER"] = Env::get("USER");
    envs["SHELL"] = Env::get("SHELL");
    envs["HOMEPATH"] = Env::get("HOMEPATH");
    envs["LOCALAPPDATA"] = Env::get("LOCALAPPDATA");
    envs["XDG_DATA_HOME"] = Env::get("XDG_DATA_HOME");

    // compiler variables
    envs["CXX"] = Env::get("CXX");
    envs["PREFIX"] = Env::get("PREFIX");
    envs["CXXFLAGS"] = Env::get("CXXFLAGS");

    // locale variables
    envs["LC_ALL"] = Env::get("LC_ALL");
    envs["LC_CTYPE"] = Env::get("LC_CTYPE");
    envs["LC_TERMINAL"] = Env::get("LC_TERMINAL");
    envs["LC_TERMINAL_VERSION"] = Env::get("LC_TERMINAL_VERSION");

    // platform dependency variables
    envs["JAVA_HOME"] = Env::get("JAVA_HOME");
    envs["GRADLE_HOME"] = Env::get("GRADLE_HOME");
    envs["ANDROID_HOME"] = getAndroidHome();
    envs["ANDROID_SUPPORTED_ABIS"] = Env::get("ANDROID_SUPPORTED_ABIS");

    // apple specific platform variables
    envs["APPLE_ID"] = Env::get("APPLE_ID");
    envs["APPLE_ID_PASSWORD"] = Env::get("APPLE_ID");

    // windows specific platform variables
    envs["SIGNTOOL"] = Env::get("SIGNTOOL");
    envs["WIN_DEBUG_LIBS"] = Env::get("WIN_DEBUG_LIBS");
    envs["CSC_KEY_PASSWORD"] = Env::get("CSC_KEY_PASSWORD");

    // ssc variables
    envs["SSC_CI"] = Env::get("SSC_CI");
    envs["SSC_RC"] = Env::get("SSC_RC");
    envs["SSC_RC_FILENAME"] = Env::get("SSC_RC_FILENAME");
    envs["SSC_ENV_FILENAME"] = Env::get("SSC_ENV_FILENAME");

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
          value = trim(Env::get(key));
        }

        envs[key] = value;
      } else if (entry.first == "env") {
        for (const auto& key : parseStringList(entry.second)) {
          auto value = trim(Env::get(key));
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
          value = trim(Env::get(key));
        }

        envs[key] = value;
      } else if (entry.first == "env") {
        for (const auto& key : parseStringList(entry.second)) {
          auto value = trim(Env::get(key));
          envs[key] = value;
        }
      }
    }

    // print all environment variables that have values
    for (const auto& entry : envs) {
      auto& key = entry.first;
      auto value = trim(entry.second);

      if (value.size() == 0) {
        value = trim(Env::get(key));
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
