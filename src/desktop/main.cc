#include <iostream>
#include <chrono>
#include <regex>
#include <span>

#include "../app.hh"
#include "../cli.hh"
#include "../runtime.hh"

#if SOCKET_RUNTIME_PLATFORM_APPLE
#include <os/log.h>
#elif SOCKET_RUNTIME_PLATFORM_LINUX
#include <dbus/dbus.h>
#include <fcntl.h>
#include "extension.hh"
#endif

//
// A cross platform MAIN macro that
// magically gives us argc and argv.
//
#if SOCKET_RUNTIME_PLATFORM_WINDOWS
#define MAIN                                                                   \
  static const int argc = __argc;                                              \
  static char** argv = __argv;                                                 \
  int CALLBACK WinMain (                                                       \
    _In_ HINSTANCE instanceId,                                                 \
    _In_ HINSTANCE hPrevInstance,                                              \
    _In_ LPSTR lpCmdLine,                                                      \
    _In_ int nCmdShow                                                          \
  )
#else
#define MAIN                                                                   \
  static const int instanceId = 0;                                             \
  int main (int argc, char** argv)
#endif

#define InvalidWindowIndexError(index) \
  String("Invalid index given for window: ") + std::to_string(index)

namespace window = ssc::runtime::window;
namespace JSON = ssc::runtime::JSON;
namespace env = ssc::runtime::env;
namespace ipc = ssc::runtime::ipc;
namespace io = ssc::runtime::io;

using namespace ssc::runtime::javascript;
using namespace ssc::runtime::config;
using namespace ssc::runtime::string;
using namespace ssc::runtime::types;
using namespace ssc;

using ssc::runtime::url::encodeURIComponent;
using ssc::runtime::window::Window;
using ssc::runtime::Process;
using ssc::runtime::getcwd;
using ssc::runtime::msleep;
using ssc::app::App;

static void installSignalHandler (int signum, void (*handler)(int)) {
#if SOCKET_RUNTIME_PLATFORM_LINUX
  struct sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  action.sa_flags = SA_NODEFER;
  sigaction(signum, &action, NULL);
#else
  signal(signum, handler);
#endif
}

static inline String readFile (const Path& path) {
  static String buffer;
  auto stream = InputFileStream(path.string());
  auto begin = InputStreamBufferIterator<char>(stream);
  auto end = InputStreamBufferIterator<char>();
  buffer.assign(begin, end);
  stream.close();
  return buffer;
}

static inline void writeFile (const Path& path, const String& source) {
  auto stream = OutputFileStream(path.string());
  stream << source;
  stream.close();
}

static Function<void(int)> shutdownHandler;

// propagate signals to the default window which will use the
// 'socket.runtime.signal' broadcast channel to propagate to all
// other windows who may be subscribers
static void defaultWindowSignalHandler (int signal) {
  auto app = App::sharedApplication();
  if (app != nullptr && app->runtime.services.platform.wasFirstDOMContentLoadedEventDispatched) {
    app->dispatch([=] () {
      auto defaultWindow = app->runtime.windowManager.getWindow(0);
      if (defaultWindow != nullptr) {
        if (defaultWindow->status < window::Manager::WindowStatus::WINDOW_CLOSING) {
          const auto json = JSON::Object {
            JSON::Object::Entries {
              {"signal", signal}
            }
          };

          defaultWindow->eval(getEmitToRenderProcessJavaScript("signal", json.str()));
        }
      }
    });
  }
}

void signalHandler (int signum) {
  auto app = App::sharedApplication();
  static const auto signalsDisabled = app->runtime.userConfig["application_signals"] == "false";
  static const auto signals = parseStringList(app->runtime.userConfig["application_signals"]);
  String name;

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    name = String(sys_signame[signum]);
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    name = strsignal(signum);
  #endif

  if (!signalsDisabled || std::find(signals.begin(), signals.end(), name) != signals.end()) {
    app->dispatch([signum]() { defaultWindowSignalHandler(signum); });
  }

  if (signum == SIGTERM || signum == SIGINT) {
    signal(signum, SIG_DFL);
    if (shutdownHandler != nullptr) {
      app->dispatch([signum] () {
        shutdownHandler(signum);
      });
    } else {
      raise(signum);
    }
  }
}

#if SOCKET_RUNTIME_PLATFORM_LINUX
static void handleApplicationURLEvent (const String& url) {
  auto app = App::sharedApplication();
  if (app != nullptr && url.size() > 0) {
    for (auto window : app->runtime.windowManager.windows) {
      if (window != nullptr) {
        window->handleApplicationURL(url);
      }
    }
  }
}

static void onGTKApplicationActivation (
  GtkApplication* app,
  GFile** files,
  gint n_files,
  const gchar* hint,
  gpointer userData
) {
  if (hint != nullptr) {
    handleApplicationURLEvent(String(hint));
  }
}

static DBusHandlerResult onDBusMessage (
  DBusConnection* connection,
  DBusMessage* message,
  void* userData
) {
  auto app = App::sharedApplication();
  static auto bundleIdentifier = app->runtime.userConfig["meta_bundle_identifier"];
  static auto dbusBundleIdentifier = replace(bundleIdentifier, "-", "_");

  // Check if the message is a method call and has the expected interface and method
  if (dbus_message_is_method_call(message, dbusBundleIdentifier.c_str(), "handleApplicationURLEvent")) {
    // Extract URI from the message
    const char *uri = nullptr;
    if (dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &uri, DBUS_TYPE_INVALID)) {
      handleApplicationURLEvent(String(uri));
    } else {
      fprintf(stderr, "error: dbus: Failed to extract URI message\n");
    }
  }

  return DBUS_HANDLER_RESULT_HANDLED;
}
#elif SOCKET_RUNTIME_PLATFORM_WINDOWS
BOOL registerWindowsURISchemeInRegistry () {
  auto app = App::sharedApplication();
  HKEY shellKey;
  HKEY key;
  LONG result;

  auto protocol = app->runtime.userConfig["meta_application_protocol"];

  if (protocol.size() == 0) {
    return FALSE;
  }

  auto scheme = protocol.c_str();
  wchar_t applicationPath[MAX_PATH];
  GetModuleFileNameW(NULL, applicationPath, MAX_PATH);

  // Create the registry key for the scheme
  result = RegCreateKeyEx(HKEY_CURRENT_USER, scheme, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
  if (result != ERROR_SUCCESS) {
    fprintf(stderr, "error: RegCreateKeyEx: Error creating registry key for scheme: %ld\n", result);
    return FALSE;
  }

  auto value = String("URL:") + protocol;
  // Set the default value for the scheme
  result = RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE*) value.c_str(), value.size());
  if (result != ERROR_SUCCESS) {
    fprintf(stderr, "error: RegSetValueEx: Error setting default value for scheme: %ld\n", result);
    RegCloseKey(key);
    return FALSE;
  }

  // Set the URL protocol value
  result = RegSetValueEx(key, "URL Protocol", 0, REG_SZ, (BYTE*)"", sizeof(""));
  if (result != ERROR_SUCCESS) {
    fprintf(stderr, "error: RegSetValueEx: Error setting URL protocol value: %ld\n", result);
    RegCloseKey(key);
    return FALSE;
  }

  // create the registry key for the shell
  result = RegCreateKeyEx(key, "shell\\open\\command", 0, NULL, 0, KEY_SET_VALUE, NULL, &shellKey, NULL);
  if (result != ERROR_SUCCESS) {
    fprintf(stderr, "error: RegCreateKeyEx: Error creating registry key for shell: %ld\n", result);
    RegCloseKey(key);
    return FALSE;
  }

  auto command = convertWStringToString(applicationPath) + String(" %1");
  result = RegSetValueEx(shellKey, NULL, 0, REG_SZ, (const BYTE*) command.c_str(), command.size());

  if (result != ERROR_SUCCESS) {
    fprintf(stderr, "error: kRegSetValueEx: Error setting command value: %ld\n", result);
    RegCloseKey(shellKey);
    RegCloseKey(key);
    return FALSE;
  }

  // close the registry keys
  RegCloseKey(shellKey);
  RegCloseKey(key);

  return TRUE;
}
#endif

static const String OK_STATE = "0";
static const String ERROR_STATE = "1";

//
// the MAIN macro provides a cross-platform program entry point.
// it makes argc and argv uniformly available. It provides "instanceId"
// which on windows is hInstance, on mac and linux this is just an int.
//
MAIN {
#if SOCKET_RUNTIME_PLATFORM_LINUX
  // use 'SIGPWR' instead of the default 'SIGUSR1' handler
  // see https://github.com/WebKit/WebKit/blob/2fd8f81aac4e867ffe107c0e1b3e34b1628c0953/Source/WTF/wtf/posix/ThreadingPOSIX.cpp#L185
  // env::set("JSC_SIGNAL_FOR_GC", "30");
  gtk_init(&argc, &argv);
#endif

  // Singletons should be static to remove some possible race conditions in
  // their instantiation and destruction.
  static App app({
    .instanceId = instanceId,
    .userConfig = getUserConfig()
  });

  const String devHost = getDevHost();
  const auto devPort = getDevPort();

  auto cwd = getcwd();

  String suffix = "";

  Vector<String> argvArray;
  StringStream argvForward;

  bool isCommandMode = false;
  bool isReadingStdin = false;
  bool isHeadless = app.runtime.userConfig["build_headless"] == "true" ? true : false;
  bool isTest = false;

  int exitCode = 0;
  int c = 0;

  bool wantsVersion = false;
  bool wantsHelp = false;

  auto bundleIdentifier = app.runtime.userConfig["meta_bundle_identifier"];

#if SOCKET_RUNTIME_PLATFORM_LINUX
  static const auto TMPDIR = env::get("TMPDIR", "/tmp");
  static const auto appInstanceLock = Path(TMPDIR) / (bundleIdentifier + ".lock");
  static const auto appInstancePID = Path(TMPDIR) / (bundleIdentifier + ".pid");
  static const auto appProtocol = app.runtime.userConfig["meta_application_protocol"];

  const auto existingPIDValue = readFile(appInstancePID);
  if (existingPIDValue.size() > 0) {
    try {
      const pid_t pid = std::stoi(existingPIDValue);
      if (kill(pid, 0) != 0) {
        throw std::runtime_error("");
      }
    } catch (...) {
      unlink(appInstanceLock.c_str());
      unlink(appInstancePID.c_str());
    }
  }

  // lock + pid files
  auto appInstanceLockFd = open(appInstanceLock.c_str(), O_CREAT | O_EXCL, 0600);

  // dbus
  auto dbusError = DBusError {}; dbus_error_init(&dbusError);
  static auto connection = dbus_bus_get(DBUS_BUS_SESSION, &dbusError);
  auto dbusBundleIdentifier = replace(bundleIdentifier, "-", "_");

  // instance is running if fd was acquired
  if (appInstanceLockFd > 0) {
    const auto pid = getpid();
    const auto filter = (
      String("type='method_call',") +
      String("interface='") + dbusBundleIdentifier + String("',") +
      String("member='handleApplicationURLEvent'")
    );

    dbus_bus_add_match(connection, filter.c_str(), &dbusError);
    writeFile(appInstancePID, std::to_string(pid));

    if (dbus_error_is_set(&dbusError)) {
      fprintf(stderr, "error: dbus: Failed to add match rule: %s\n", dbusError.message);
      dbus_error_free(&dbusError);
      dbus_connection_unref(connection);
      exit(EXIT_FAILURE);
    }

    if (
      DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != dbus_bus_request_name(
        connection,
        dbusBundleIdentifier.c_str(),
        DBUS_NAME_FLAG_REPLACE_EXISTING,
        &dbusError
      )
    ) {
      fprintf(stderr, "error: dbus: Failed to request name for: %s\n", dbusBundleIdentifier.c_str());
      exit(EXIT_FAILURE);
    }

    if (dbus_error_is_set(&dbusError)) {
      fprintf(stderr, "error: dbus: Failed to request name for: %s: %s\n", dbusBundleIdentifier.c_str(), dbusError.message);
      dbus_error_free(&dbusError);
    }

    dbus_connection_add_filter(connection, onDBusMessage, nullptr, nullptr);

    static Function<void()> pollForMessage = []() {
      Thread thread([] () {
        auto app = App::sharedApplication();
        while (dbus_connection_read_write_dispatch(connection, 256));
        app->dispatch(pollForMessage);
      });

      thread.detach();
    };

    app.dispatch(pollForMessage);
    if (appProtocol.size() > 0 && argc > 1 && String(argv[1]).starts_with(appProtocol + ":")) {
      const auto uri = String(argv[1]);
      app.dispatch([uri]() {
        handleApplicationURLEvent(uri);
      });
    }
  } else if (appProtocol.size() > 0 && argc > 1 && String(argv[1]).starts_with(appProtocol + ":")) {
    if (dbus_error_is_set(&dbusError)) {
      fprintf(stderr, "error: dbus: Connection error: %s\n", dbusError.message);
      dbus_error_free(&dbusError);
      exit(EXIT_FAILURE);
    }

    auto message = dbus_message_new_method_call(
      dbusBundleIdentifier.c_str(),
      (String("/") + replace(dbusBundleIdentifier, "\\.", "/")).c_str(),
      dbusBundleIdentifier.c_str(),
      "handleApplicationURLEvent"
    );

    if (message == NULL) {
      fprintf(stderr, "error: dbus: essage creation failed\n");
      exit(EXIT_FAILURE);
    }

    // Append the URI to the D-Bus message
    if (!dbus_message_append_args(message, DBUS_TYPE_STRING, &argv[1], DBUS_TYPE_INVALID)) {
      fprintf(stderr, "error: dbus: Failed to append URI to D-Bus message\n");
      exit(EXIT_FAILURE);
    }

    // Send the D-Bus message
    if (!dbus_connection_send(connection, message, NULL)) {
      fprintf(stderr, "error: dbus: Failed to send message\n");
      exit(EXIT_FAILURE);
    }

    dbus_connection_flush(connection);
    dbus_message_unref(message);
    dbus_connection_unref(connection);
    exit(EXIT_SUCCESS);
  } else {
    exit(EXIT_FAILURE);
  }

  atexit([]() {
    unlink(appInstanceLock.c_str());
  });

  auto gtkApp = gtk_application_new(
    bundleIdentifier.c_str(),
    G_APPLICATION_FLAGS_NONE
  );

  if (appProtocol.size() > 0) {
    GError* error = nullptr;
    auto appName = app.runtime.userConfig["build_name"];
    auto appDescription = app.runtime.userConfig["meta_description"];
    auto appContentType = String("x-scheme-handler/") + appProtocol;
    auto appinfo = g_app_info_create_from_commandline(
      appName.c_str(),
      appDescription.c_str(),
      G_APP_INFO_CREATE_SUPPORTS_URIS,
      NULL
    );

    g_app_info_set_as_default_for_type(
      appinfo,
      appContentType.c_str(),
      &error
    );

    if (error != nullptr) {
      fprintf(stderr, "error: g_app_info_set_as_default_for_type: %s\n", error->message);
      return 1;
    }

    g_signal_connect(gtkApp, "activate", G_CALLBACK(onGTKApplicationActivation), NULL);
  }
#elif SOCKET_RUNTIME_PLATFORM_WINDOWS
  HANDLE hMutex = CreateMutex(NULL, TRUE, bundleIdentifier.c_str());
  auto lastWindowsError = GetLastError();
  auto appProtocol = app.runtime.userConfig["meta_application_protocol"];

  if (appProtocol.size() > 0 && argc > 1 && String(argv[1]).starts_with(appProtocol)) {
    HWND hWnd = FindWindow(
      app.runtime.userConfig["meta_bundle_identifier"].c_str(),
      app.runtime.userConfig["meta_title"].c_str()
    );

    if (hWnd != NULL) {
      COPYDATASTRUCT data;
      data.dwData = WM_HANDLE_DEEP_LINK;
      data.cbData = strlen(lpCmdLine);
      data.lpData = lpCmdLine;
      SendMessage(
        hWnd,
        WM_COPYDATA,
        (WPARAM) nullptr,
        reinterpret_cast<LPARAM>(&data)
      );
    } else {
      app.dispatch([hWnd, lpCmdLine]() {
        ssc::app::onWindowProcMessage(
          hWnd,
          WM_HANDLE_DEEP_LINK,
          (WPARAM) strlen(lpCmdLine),
          (LPARAM) lpCmdLine
        );
      });
    }
  }

  if (lastWindowsError == ERROR_ALREADY_EXISTS) {
    // Application is already running, send the URI to the existing instance
    // Release the mutex and exit
    CloseHandle(hMutex);
    return 0;
  }

  registerWindowsURISchemeInRegistry();
#endif

  // TODO right now we forward a json parsable string as the args but this
  // isn't the most robust way of doing this. possible a URI-encoded query
  // string would be more in-line with how everything else works.
  for (auto const arg : std::span(argv, argc)) {
    auto s = String(arg);

    argvArray.push_back("'" + replace(s, "'", "\'") + "'");

    bool helpRequested = (
      (s.find("--help") == 0) ||
      (s.find("-help") == 0) ||
      (s.find("-h") == 0)
    );

    bool versionRequested = (
      (s.find("--version") == 0) ||
      (s.find("-version") == 0) ||
      (s.find("-v") == 0) ||
      (s.find("-V") == 0)
    );

    if (s.find("--stdin") == 0) {
      isReadingStdin = true;
    }

    if (helpRequested) {
      wantsHelp = true;
    }

    if (versionRequested) {
      wantsVersion = true;
    }

    if (s.find("--headless") == 0) {
      isHeadless = true;
      app.runtime.userConfig["build_headless"] = "true";
    }

    // launched from the `ssc` cli
    app.launchSource = s.find("--from-ssc") == 0
      ? App::LaunchSource::Tool
      : App::LaunchSource::Platform;

    if (s.find("--test") == 0) {
      suffix = "-test";
      isTest = true;
    } else if (c >= 2 && s.find("-") != 0) {
      isCommandMode = true;
    }

    if (helpRequested || versionRequested) {
      isCommandMode = true;
    }

    if (helpRequested) {
      argvForward << " " << "help --warn-arg-usage=" << s;
    } else if (versionRequested) {
      argvForward << " " << "version --warn-arg-usage=" << s;
    } else if (c > 1 || isCommandMode) {
      argvForward << " " << String(arg);
    }
  }

  if (isDebugEnabled()) {
    app.runtime.userConfig["build_name"] += "-dev";
  }

  app.runtime.userConfig["build_name"] += suffix;

  argvForward << " --ssc-version=v" << runtime::VERSION_STRING;
  argvForward << " --version=v" << app.runtime.userConfig["meta_version"];
  argvForward << " --name=" << app.runtime.userConfig["build_name"];

  if (isDebugEnabled()) {
    argvForward << " --debug=1";
  }

  String cmd;
  if (runtime::platform.os == "win32") {
    cmd = app.runtime.userConfig["win_cmd"];
  } else {
    cmd = app.runtime.userConfig[runtime::platform.os + "_cmd"];
  }

  if (cmd[0] == '.') {
    auto index = cmd.find_first_of('.');
    auto executable = cmd.substr(0, index);
    auto absPath = Path(cwd) / Path(executable);
    cmd = absPath.string() + cmd.substr(index);
  }

  static Process* process = nullptr;
  static Function<void(bool)> createProcess;

  auto killProcess = [](Process* processToKill) {
    if (processToKill != nullptr) {
      processToKill->kill();
      processToKill->wait();

      if (processToKill == process) {
        process = nullptr;
      }

      delete processToKill;
    }
  };

  auto createProcessTemplate = [killProcess]<class... Args>(Args... args) {
    return [=](bool force) {
      if (process != nullptr && force) {
        killProcess(process);
      }
      process = new Process(args...);
    };
  };

  if (isCommandMode && cmd.size() > 0) {
    argvForward << " --cwd=" << fs::current_path();

    createProcess = createProcessTemplate(
      cmd,
      argvForward.str(),
      cwd,
      [&exitCode](String const &out) mutable {
        ipc::Message message(out);

        if (message.name == "exit") {
          exitCode = stoi(message.get("value"));
          exit(exitCode);
        } else {
          io::write(message.get("value"), false);
        }
      },
      [](String const &out) { io::write(out, true); },
      [](String const &code){ exit(std::stoi(code)); }
    );

    if (cmd.size() == 0) {
      io::write("No 'cmd' is provided for '" + runtime::platform.os + "' in socket.ini", true);
      exit(1);
    }

    createProcess(true);

    shutdownHandler = [=](int signum) mutable {
      msleep(32);

    #if SOCKET_RUNTIME_PLATFORM_LINUX
      unlink(appInstanceLock.c_str());
    #endif
      if (process != nullptr) {
        process->kill(signum);
      }
      exit(signum);
    };

    #if !SOCKET_RUNTIME_PLATFORM_WINDOWS
      installSignalHandler(SIGHUP, signalHandler);
    #endif

    installSignalHandler(SIGINT, signalHandler);
    installSignalHandler(SIGTERM, signalHandler);

    return exitCode;
  }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
  static auto SOCKET_RUNTIME_OS_LOG_BUNDLE = os_log_create(
    bundleIdentifier.c_str(),
    "socket.runtime"
  );
  #endif

  const auto onStdErr = [](const auto& output) {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    os_log_with_type(SOCKET_RUNTIME_OS_LOG_BUNDLE, OS_LOG_TYPE_ERROR, "%{public}s", output.c_str());
  #endif
    std::cerr << "\033[31m" + output + "\033[0m";

    for (const auto& window : app.runtime.windowManager.windows) {
      if (window != nullptr) {
        window->bridge->emit("process-error", output);
      }
    }
  };

  //
  // # "Backend" -> Main
  // Launch the backend process and connect callbacks to the stdio and stderr pipes.
  //
  const auto onStdOut = [](const auto& output) {
    const auto message = ipc::Message(output);

    if (message.index > 0 && message.name.size() == 0) {
      // @TODO: print warning
      return;
    }

    if (message.index > SOCKET_RUNTIME_MAX_WINDOWS) {
      // @TODO: print warning
      return;
    }

    const auto value = message.value;

    if (message.name == "stdout") {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      dispatch_async(dispatch_get_main_queue(), ^{
        os_log_with_type(SOCKET_RUNTIME_OS_LOG_BUNDLE, OS_LOG_TYPE_DEFAULT, "%{public}s", value.c_str());
      });
    #endif
      io::write(value);
      return;
    }

    if (message.name == "stderr") {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      dispatch_async(dispatch_get_main_queue(), ^{
        os_log_with_type(SOCKET_RUNTIME_OS_LOG_BUNDLE, OS_LOG_TYPE_ERROR, "%{public}s", value.c_str());
      });
    #endif
      io::write(value, true);
      return;
    }

    //
    // ## Dispatch
    // Messages from the backend process may be sent to the render process. If they
    // are parsable commands, try to do something with them, otherwise they are
    // just stdout and we can write the data to the pipe.
    //
    app.dispatch([&, message, value]() {
      if (message.name == "send") {
        const auto event = message.get("event");
        if (message.index >= 0) {
          const auto window = app.runtime.windowManager.getWindow(message.index);
          if (window) {
            window->bridge->emit(event, value);
          }
        } else {
          for (const auto& window : app.runtime.windowManager.windows) {
            if (window) {
              window->bridge->emit(event, value);
            }
          }
        }
        return;
      }

      auto window = app.runtime.windowManager.getOrCreateWindow(message.index);

      if (!window) {
        auto defaultWindow = app.runtime.windowManager.getWindow(0);

        if (defaultWindow) {
          window = defaultWindow;
        }

        // @TODO: print warning
      }

      if (message.name == "heartbeat") {
        if (message.seq.size() > 0) {
          const auto result = ipc::Result(message.seq, message, "heartbeat");
          window->bridge->send(message.seq, result.json());
        }
        return;
      }

      if (message.name == "resolve") {
        window->resolvePromise(message.seq, message.get("state"), encodeURIComponent(value));
        return;
      }

      if (message.name == "process.exit") {
        for (const auto& window : app.runtime.windowManager.windows) {
          if (window) {
            window->bridge->emit("process-exit", message.value);
          }
        }
        return;
      }
    });
  };

  createProcess = createProcessTemplate(
    cmd,
    argvForward.str(),
    cwd,
    onStdOut,
    onStdErr,
    [&](String const &code) {
      for (auto w : app.runtime.windowManager.windows) {
        if (w != nullptr) {
          auto window = app.runtime.windowManager.getWindow(w->options.index);
          window->eval(getEmitToRenderProcessJavaScript("backend-exit", code));
        }
      }
    }
  );

  //
  // # Render -> Main
  // Send messages from the render processes to the main process.
  // These may be similar to how we route the messages from the
  // backend process but different enough that duplication is ok. This
  // callback doesnt need to dispatch because it's already in the
  // main thread.
  //
  const auto onMessage = [cmd, &killProcess](const auto& output) {
    const auto message = ipc::Message(output, true);

    auto window = app.runtime.windowManager.getWindow(message.index);
    auto value = message.value;

    // the window must exist
    if (!window && message.index >= 0) {
      auto defaultWindow = app.runtime.windowManager.getWindow(0);

      if (defaultWindow) {
        window = defaultWindow;
      }
    }

    if (message.name == "process.open") {
      auto force = message.get("force") == "true" ? true : false;
      if (cmd.size() > 0) {
        if (process == nullptr || force) {
          createProcess(force);
          process->open();
        }
      #ifdef SOCKET_RUNTIME_PLATFORM_WINDOWS
        size_t last_pos = 0;
        while ((last_pos = process->path.find('\\', last_pos)) != String::npos) {
          process->path.replace(last_pos, 1, "\\\\\\\\");
          last_pos += 4;
        }
      #endif
        const JSON::Object json = JSON::Object::Entries {
          { "cmd", cmd },
          { "argv", process->argv },
          { "path", process->path }
        };
        window->resolvePromise(message.seq, OK_STATE, json);
        return;
      }
      window->resolvePromise(message.seq, ERROR_STATE, JSON::null);
      return;
    }

    if (message.name == "process.kill") {
      if (cmd.size() > 0 && process != nullptr) {
        killProcess(process);
      }

      window->resolvePromise(message.seq, OK_STATE, JSON::null);
      return;
    }

    if (message.name == "process.write") {
      if (cmd.size() > 0 && process != nullptr) {
        process->write(output);
      }
      window->resolvePromise(message.seq, OK_STATE, JSON::null);
      return;
    }

    if (message.name == "resolve") {
      // TODO: pass it to the backend process
      // if (process != nullptr) {
      //   process->write(out);
      // }
      return;
    };

    auto err = JSON::Object::Entries {
      {"source", message.name},
      {"err", JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"},
        {"url", output}
      }}
    };

    auto result = ipc::Result(message.seq, message, err);
    window->resolvePromise(
      message.seq,
      ERROR_STATE,
      result.json()
    );
  };

  //
  // # Exiting
  //
  // When a window or the app wants to exit,
  // we clean up the windows and the backend process.
  //
  shutdownHandler = [](int code) {
  #if SOCKET_RUNTIME_PLATFORM_LINUX
    unlink(appInstanceLock.c_str());
  #endif
    if (process != nullptr) {
      process->kill(code);
      process = nullptr;
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    // launched from CLI
    if (app.launchSource == App::LaunchSource::Tool) {
      debug("__EXIT_SIGNAL__=%d", 0);
      ssc::cli::notify();
    }
  #endif

    app.stop();
    exit(code);
  };

  app.shutdownHandler = shutdownHandler;

  //
  // If this is being run in a terminal/multiplexer
  //
#if !SOCKET_RUNTIME_PLATFORM_WINDOWS
  installSignalHandler(SIGHUP, signalHandler);
#endif

#if defined(SIGUSR1)
  installSignalHandler(SIGUSR1, signalHandler);
#endif

  installSignalHandler(SIGINT, signalHandler);
  installSignalHandler(SIGTERM, signalHandler);

  const auto signalsDisabled = app.runtime.userConfig["application_signals"] == "false";
  const auto signals = parseStringList(app.runtime.userConfig["application_signals"]);

#define SET_DEFAULT_WINDOW_SIGNAL_HANDLER(sig) {                               \
  const auto name = String(CONVERT_TO_STRING(sig));                            \
  if (                                                                         \
    !signalsDisabled ||                                                        \
    std::find(signals.begin(), signals.end(), name) != signals.end()           \
  ) {                                                                          \
    installSignalHandler(sig, defaultWindowSignalHandler);                     \
  }                                                                            \
}

#if defined(SIGQUIT)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGQUIT)
#endif
#if defined(SIGILL)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGILL)
#endif
#if defined(SIGTRAP)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGTRAP)
#endif
#if defined(SIGABRT)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGABRT)
#endif
#if defined(SIGIOT)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGIOT)
#endif
#if defined(SIGBUS)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGBUS)
#endif
#if defined(SIGFPE)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGFPE)
#endif
#if defined(SIGKILL)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGKILL)
#endif
#if defined(SIGUSR2)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGUSR2)
#endif
#if defined(SIGPIPE)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGPIPE)
#endif
#if defined(SIGALRM)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGALRM)
#endif
#if defined(SIGCHLD)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGCHLD)
#endif
#if defined(SIGCONT)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGCONT)
#endif
#if defined(SIGSTOP)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGSTOP)
#endif
#if defined(SIGTSTP)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGTSTP)
#endif
#if defined(SIGTTIN)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGTTIN)
#endif
#if defined(SIGTTOU)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGTTOU)
#endif
#if defined(SIGURG)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGURG)
#endif
#if defined(SIGXCPU)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGXCPU)
#endif
#if defined(SIGXFSZ)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGXFSZ)
#endif
#if defined(SIGVTALRM)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGVTALRM)
#endif
#if defined(SIGPROF)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGPROF)
#endif
#if defined(SIGWINCH)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGWINCH)
#endif
#if defined(SIGIO)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGIO)
#endif
#if defined(SIGINFO)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGINFO)
#endif
#if defined(SIGSYS)
  SET_DEFAULT_WINDOW_SIGNAL_HANDLER(SIGSYS)
#endif

    Vector<String> properties = {
      "window_width", "window_height",
      "window_min_width", "window_min_height",
      "window_max_width", "window_max_height"
    };

    auto setDefaultValue = [](String property) {
      // for min values set 0
      if (property.find("min") != -1) {
        return "0";
      // for other values set 100%
      } else {
        return "100%";
      }
    };

    // Regular expression to match a float number or a percentage
    std::regex validPattern("^\\d*\\.?\\d+%?$");

    for (const auto& property : properties) {
      if (app.runtime.userConfig[property].size() > 0) {
        auto value = app.runtime.userConfig[property];
        if (!std::regex_match(value, validPattern)) {
          app.runtime.userConfig[property] = setDefaultValue(property);
          debug("Invalid value for %s: \"%s\". Setting it to \"%s\"", property.c_str(), value.c_str(), app.runtime.userConfig[property].c_str());
        }
      // set default value if it's not set in socket.ini
      } else {
        app.runtime.userConfig[property] = setDefaultValue(property);
      }
    }

    auto getProperty = [](String property) {
      if (app.runtime.userConfig.count(property) > 0) {
        return app.runtime.userConfig[property];
      }

      return String("");
    };

    auto windowManagerOptions = window::ManagerOptions {
      .defaultHeight = getProperty("window_height"),
      .defaultWidth = getProperty("window_width"),
      .defaultMinWidth = getProperty("window_min_width"),
      .defaultMinHeight = getProperty("window_min_height"),
      .defaultMaxWidth = getProperty("window_max_width"),
      .defaultMaxHeight = getProperty("window_max_height")
    };

    windowManagerOptions.features.useTestScript = isTest;
    windowManagerOptions.userConfig = app.runtime.userConfig;
    windowManagerOptions.argv = argvArray;
    windowManagerOptions.onMessage = onMessage;
    windowManagerOptions.onExit = shutdownHandler;

    app.runtime.windowManager.configure(windowManagerOptions);

    auto isMaximizable = getProperty("window_maximizable");
    auto isMinimizable = getProperty("window_minimizable");
    auto isClosable = getProperty("window_closable");

    auto defaultWindow = app.runtime.windowManager.createDefaultWindow(Window::Options {
      .minimizable = (isMinimizable == "" || isMinimizable == "true") ? true : false,
      .maximizable = (isMaximizable == "" || isMaximizable == "true") ? true : false,
      .resizable = getProperty("window_resizable") == "false" ? false : true,
      .closable = (isClosable == "" || isClosable == "true") ? true : false,
      .frameless = getProperty("window_frameless") == "true" ? true : false,
      .utility = getProperty("window_utility") == "true" ? true : false,
      .shouldExitApplicationOnClose = true,
      .titlebarStyle = getProperty("window_titlebar_style"),
      .windowControlOffsets = getProperty("mac_window_control_offsets"),
      .backgroundColorLight = getProperty("window_background_color_light"),
      .backgroundColorDark = getProperty("window_background_color_dark")
    });

    app.runtime.serviceWorkerManager.init("socket://" + app.runtime.userConfig["meta_bundle_identifier"], {
      .userConfig = app.runtime.userConfig
    });

    msleep(256);
    if (app.runtime.userConfig["build_headless"] != "true") {
      defaultWindow->show();
    } else {
      defaultWindow->hide();
    }

    if (devPort > 0) {
      defaultWindow->navigate(devHost + ":" + std::to_string(devPort));
      defaultWindow->setSystemMenu(String(
        "Developer Mode: \n"
        "  Reload: r + CommandOrControl\n"
        "  Quit: q + CommandOrControl\n"
        ";"
      ));
    } else {
      if (app.runtime.userConfig["webview_root"].size() != 0) {
        defaultWindow->navigate(
          "socket://" + app.runtime.userConfig["meta_bundle_identifier"] + app.runtime.userConfig["webview_root"]
        );
      } else {
        defaultWindow->navigate(
          "socket://" + app.runtime.userConfig["meta_bundle_identifier"] + "/index.html"
        );
      }
    }

    if (isReadingStdin) {
      String value;
      std::getline(std::cin, value);

      auto t = Thread([](String value) {
        auto app = App::sharedApplication();
        auto defaultWindow = app->runtime.windowManager.getWindow(0);

        while (!app->runtime.services.platform.wasFirstDOMContentLoadedEventDispatched) {
          msleep(128);
        }

        do {
          if (value.size() == 0) {
            std::getline(std::cin, value);
          }

          if (value.size() > 0) {
            defaultWindow->eval(getEmitToRenderProcessJavaScript("process.stdin", value));
            value.clear();
          }
        } while (true);
      }, value);

      t.detach();
    }

    //
    // # Event Loop
    // start the platform specific event loop for the main
    // thread and run it until it returns a non-zero int.
    //
    while (app.run(argc, argv) == 0);

#if SOCKET_RUNTIME_PLATFORM_LINUX
  dbus_connection_unref(connection);
#endif

  exit(exitCode);
}
