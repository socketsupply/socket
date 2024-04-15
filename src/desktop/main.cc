#include "../app/app.hh"
#include "../cli/cli.hh"
#include "../ipc/ipc.hh"
#include "../core/core.hh"
#include "../core/ini.hh"
#include "../process/process.hh"
#include "../window/window.hh"

#if defined(__linux__)
#include <dbus/dbus.h>
#include <fcntl.h>
#endif

#include <iostream>
#include <ostream>
#include <chrono>
#include <regex>
#include <span>

//
// A cross platform MAIN macro that
// magically gives us argc and argv.
//
#if defined(_WIN32)
#define MAIN                         \
  static const int argc = __argc;    \
  static char** argv = __argv;       \
  int CALLBACK WinMain (             \
    _In_ HINSTANCE instanceId,       \
    _In_ HINSTANCE hPrevInstance,    \
    _In_ LPSTR lpCmdLine,            \
    _In_ int nCmdShow)
#else
#define MAIN                                   \
  const int instanceId = 0;                    \
  int main (int argc, char** argv)
#endif

#if defined(__APPLE__)
#include <os/log.h>
#endif

#define InvalidWindowIndexError(index) \
  SSC::String("Invalid index given for window: ") + std::to_string(index)

static void installSignalHandler (int signum, void (*handler)(int)) {
#if SSC_PLATFORM_LINUX
  struct sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_handler = handler;
  action.sa_flags = SA_NODEFER;
  sigaction(signum, &action, NULL);
#else
  signal(signum, handler);
#endif
}


using namespace SSC;

static App *app_ptr = nullptr;

static Function<void(int)> shutdownHandler;

// propagate signals to the default window which will use the
// 'socket.runtime.signal' broadcast channel to propagate to all
// other windows who may be subscribers
static void defaultWindowSignalHandler (int signal) {
  auto app = App::instance();
  if (app != nullptr && app->windowManager != nullptr) {
    auto defaultWindow = app->windowManager->getWindow(0);
    if (defaultWindow != nullptr) {
      if (defaultWindow->status < WindowManager::WindowStatus::WINDOW_CLOSING) {
        const auto json = JSON::Object {
          JSON::Object::Entries {
            {"signal", signal}
          }
        };

        defaultWindow->eval(getEmitToRenderProcessJavaScript("signal", json.str()));
      }
    }
  }
}

void signalHandler (int signal) {
  static auto app = App::instance();
  static auto userConfig = SSC::getUserConfig();
  static const auto signalsDisabled = userConfig["application_signals"] == "false";
  static const auto signals = parseStringList(userConfig["application_signals"]);
  String name;

  #if defined(__APPLE__)
    name = String(sys_signame[signal]);
  #elif defined(__linux__)
    name = strsignal(signal);
  #endif

  if (!signalsDisabled || std::find(signals.begin(), signals.end(), name) != signals.end()) {
    defaultWindowSignalHandler(signal);
  }

  if (shutdownHandler != nullptr) {
    app->dispatch([signal] () {
      shutdownHandler(signal);
    });
  }
}

SSC::String getNavigationError (const String &cwd, const String &value) {
  auto url = value.substr(7);

  if (!value.starts_with("socket://") &&  !value.starts_with("socket://")) {
    return SSC::String("only socket:// protocol is allowed for the file navigation. Got url ") + value;
  }

  if (url.empty()) {
    return SSC::String("empty url");
  }

  return SSC::String("");
}

inline const Vector<int> splitToInts (const String& s, const char& c) {
  Vector<int> result;
  String token;
  std::istringstream ss(s);

  while (std::getline(ss, token, c)) {
    result.push_back(std::stoi(token));
  }
  return result;
}

#if defined(__linux__)
static void handleApplicationURLEvent (const String url) {
  SSC::JSON::Object json = SSC::JSON::Object::Entries {{
    "url", url
  }};

  if (app_ptr != nullptr && app_ptr->windowManager != nullptr) {
    for (auto window : app_ptr->windowManager->windows) {
      if (window != nullptr) {
        if (window->index == 0) {
          gtk_widget_show_all(GTK_WIDGET(window->window));
          gtk_widget_grab_focus(GTK_WIDGET(window->webview));
          gtk_widget_grab_focus(GTK_WIDGET(window->window));
          gtk_window_activate_focus(GTK_WINDOW(window->window));
          gtk_window_present(GTK_WINDOW(window->window));
        }

        window->bridge->router.emit("applicationurl", json.str());
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
  handleApplicationURLEvent(String(hint));
}

static DBusHandlerResult onDBusMessage (
  DBusConnection* connection,
  DBusMessage* message,
  void* userData
) {
  static auto userConfig = SSC::getUserConfig();
  static auto bundleIdentifier = userConfig["meta_bundle_identifier"];
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
#elif defined(_WIN32)
BOOL registerWindowsURISchemeInRegistry () {
  static auto userConfig = SSC::getUserConfig();
  HKEY shellKey;
  HKEY key;
  LONG result;

  auto protocol = userConfig["meta_application_protocol"];

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

//
// the MAIN macro provides a cross-platform program entry point.
// it makes argc and argv uniformly available. It provides "instanceId"
// which on windows is hInstance, on mac and linux this is just an int.
//
MAIN {
#if defined(__linux__)
  // use 'SIGPWR' instead of the default 'SIGUSR1' handler
  // see https://github.com/WebKit/WebKit/blob/2fd8f81aac4e867ffe107c0e1b3e34b1628c0953/Source/WTF/wtf/posix/ThreadingPOSIX.cpp#L185
  Env::set("JSC_SIGNAL_FOR_GC", "30");
  gtk_init(&argc, &argv);
#endif

  // Singletons should be static to remove some possible race conditions in
  // their instantiation and destruction.
  static App app(instanceId);
  static WindowManager windowManager(app);
  static auto userConfig = SSC::getUserConfig();

  // TODO(trevnorris): Since App is a singleton, follow the CppCoreGuidelines
  // better in how it's handled in the future.
  // For now make a pointer reference since there is some member variable name
  // collision in the call to shutdownHandler when it's being called from the
  // windowManager instance.
  app_ptr = &app;

  app.setWindowManager(&windowManager);
  const String _host = getDevHost();
  const auto _port = getDevPort();

  const SSC::String OK_STATE = "0";
  const SSC::String ERROR_STATE = "1";
  const SSC::String EMPTY_SEQ = SSC::String("");

  auto cwd = app.getcwd();
  app.userConfig = userConfig;

  SSC::String suffix = "";

  SSC::StringStream argvArray;
  SSC::StringStream argvForward;

  bool isCommandMode = false;
  bool isReadingStdin = false;
  bool isHeadless = userConfig["build_headless"] == "true" ? true : false;
  bool isTest = false;

  int exitCode = 0;
  int c = 0;

  bool wantsVersion = false;
  bool wantsHelp = false;

  auto bundleIdentifier = userConfig["meta_bundle_identifier"];

#if defined(__linux__)
  static const auto TMPDIR = Env::get("TMPDIR", "/tmp");
  static const auto appInstanceLock = fs::path(TMPDIR) / (bundleIdentifier + ".lock");
  auto appInstanceLockFd = open(appInstanceLock.c_str(), O_CREAT | O_EXCL, 0600);
  auto appProtocol = userConfig["meta_application_protocol"];
  auto dbusError = DBusError {}; dbus_error_init(&dbusError);
  auto connection = dbus_bus_get(DBUS_BUS_SESSION, &dbusError);
  auto dbusBundleIdentifier = replace(bundleIdentifier, "-", "_");

  // instance is running if fd was acquired
  if (appInstanceLockFd != -1) {
    auto filter = (
      String("type='method_call',") +
      String("interface='") + dbusBundleIdentifier + String("',") +
      String("member='handleApplicationURLEvent'")
    );

    dbus_bus_add_match(connection, filter.c_str(), &dbusError);

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

    static Function<void()> pollForMessage = [connection]() {
      Thread thread([connection] () {
        while (dbus_connection_read_write_dispatch(connection, 100));
        app_ptr->dispatch(pollForMessage);
      });

      thread.detach();
    };

    app_ptr->dispatch(pollForMessage);
    if (appProtocol.size() > 0 && argc > 1 && String(argv[1]).starts_with(appProtocol + ":")) {
      const auto uri = String(argv[1]);
      app_ptr->dispatch([uri]() {
        app_ptr->core->dispatchEventLoop([uri]() {
          handleApplicationURLEvent(uri);
        });
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
    auto appName = userConfig["build_name"];
    auto appDescription = userConfig["meta_description"];
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
#elif defined(_WIN32)
  HANDLE hMutex = CreateMutex(NULL, TRUE, bundleIdentifier.c_str());
  auto lastWindowsError = GetLastError();
  auto appProtocol = userConfig["meta_application_protocol"];

  if (appProtocol.size() > 0 && argc > 1 && String(argv[1]).starts_with(appProtocol)) {
    HWND hWnd = FindWindow(
      userConfig["meta_bundle_identifier"].c_str(),
      userConfig["meta_title"].c_str()
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
      app_ptr->dispatch([hWnd, lpCmdLine]() {
        Window::WndProc(
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
    auto s = SSC::String(arg);

    argvArray
      << "'"
      << replace(s, "'", "\'")
      << (c++ < argc ? "', " : "'");

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
      userConfig["build_headless"] = "true";
    }

    // launched from the `ssc` cli
    app.wasLaunchedFromCli = s.find("--from-ssc") == 0 ? true : false;

    #ifdef _WIN32
    if (!app.w32ShowConsole && s.find("--w32-console") == 0) {
      app.w32ShowConsole = true;
      app.ShowConsole();
    }
    #endif

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
      argvForward << " " << SSC::String(arg);
    }
  }

  if (isDebugEnabled()) {
    app.userConfig["build_name"] += "-dev";
  }

  app.userConfig["build_name"] += suffix;

  argvForward << " --ssc-version=v" << SSC::VERSION_STRING;
  argvForward << " --version=v" << app.userConfig["meta_version"];
  argvForward << " --name=" << app.userConfig["build_name"];

  if (isDebugEnabled()) {
    argvForward << " --debug=1";
  }

  SSC::StringStream env;
  for (auto const &envKey : parseStringList(app.userConfig["build_env"])) {
    auto cleanKey = trim(envKey);

    if (!Env::has(cleanKey)) {
      continue;
    }

    auto envValue = Env::get(cleanKey.c_str());

    env << SSC::String(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  SSC::String cmd;
  if (platform.os == "win32") {
    cmd = app.userConfig["win_cmd"];
  } else {
    cmd = app.userConfig[platform.os + "_cmd"];
  }

  if (cmd[0] == '.') {
    auto index = cmd.find_first_of('.');
    auto executable = cmd.substr(0, index);
    auto absPath = fs::path(cwd) / fs::path(executable);
    cmd = absPath.string() + cmd.substr(index);
  }

  static Process* process = nullptr;
  static Function<void(bool)> createProcess;

  auto killProcess = [&](Process* processToKill) {
    if (processToKill != nullptr) {
      processToKill->kill();
      processToKill->wait();

      if (processToKill == process) {
        process = nullptr;
      }

      delete processToKill;
    }
  };

  auto createProcessTemplate = [&]<class... Args>(Args... args) {
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
      [&](SSC::String const &out) {
        IPC::Message message(out);

        if (message.name == "exit") {
          exitCode = stoi(message.get("value"));
          exit(exitCode);
        } else {
          IO::write(message.get("value"), false);
        }
      },
      [](SSC::String const &out) { IO::write(out, true); },
      [](SSC::String const &code){ exit(std::stoi(code)); }
    );

    if (cmd.size() == 0) {
      IO::write("No 'cmd' is provided for '" + platform.os + "' in socket.ini", true);
      exit(1);
    }

    createProcess(true);

    shutdownHandler = [&](int signum) {
    #if defined(__linux__)
      unlink(appInstanceLock.c_str());
    #endif
      if (process != nullptr) {
        process->kill();
      }
      exit(signum);
    };

    #ifndef _WIN32
      installSignalHandler(SIGHUP, signalHandler);
    #endif

    installSignalHandler(SIGINT, signalHandler);
    installSignalHandler(SIGTERM, signalHandler);

    return exitCode;
  }

  #if defined(__APPLE__)
  static auto SSC_OS_LOG_BUNDLE = os_log_create(
    bundleIdentifier.c_str(),
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      "socket.runtime.mobile"
    #else
      "socket.runtime.desktop"
    #endif
  );
  #endif

  auto onStdErr = [&](auto err) {
  #if defined(__APPLE__)
    os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_ERROR, "%{public}s", err.c_str());
  #endif
    std::cerr << "\033[31m" + err + "\033[0m";

    for (auto w : windowManager.windows) {
      if (w != nullptr) {
        auto window = windowManager.getWindow(w->opts.index);
        window->eval(getEmitToRenderProcessJavaScript("process-error", err));
      }
    }
  };

  //
  // # Backend -> Main
  // Launch the backend process and connect callbacks to the stdio and stderr pipes.
  //
  auto onStdOut = [&](SSC::String const &out) {
    IPC::Message message(out);

    if (message.index > 0 && message.name.size() == 0) {
      // @TODO: print warning
      return;
    }

    if (message.index > SSC_MAX_WINDOWS) {
      // @TODO: print warning
      return;
    }

    auto value = message.get("value");

    if (message.name == "stdout") {
    #if defined(__APPLE__)
      dispatch_async(dispatch_get_main_queue(), ^{
        os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_DEFAULT, "%{public}s", value.c_str());
      });
    #endif
      std::cout << value;
      return;
    }

    if (message.name == "stderr") {
    #if defined(__APPLE__)
      dispatch_async(dispatch_get_main_queue(), ^{
        os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_ERROR, "%{public}s", value.c_str());
      });
    #endif
      std::cerr << "\033[31m" + value + "\033[0m";
      return;
    }

    //
    // ## Dispatch
    // Messages from the backend process may be sent to the render process. If they
    // are parsable commands, try to do something with them, otherwise they are
    // just stdout and we can write the data to the pipe.
    //
    app.dispatch([&, message, value] {
      auto seq = message.get("seq");

      if (message.name == "send") {
        SSC::String script = getEmitToRenderProcessJavaScript(
          message.get("event"),
          value
        );
        if (message.index >= 0) {
          auto window = windowManager.getWindow(message.index);
          if (window) {
            window->eval(script);
          }
        } else {
          for (auto w : windowManager.windows) {
            if (w != nullptr) {
              auto window = windowManager.getWindow(w->opts.index);
              window->eval(script);
            }
          }
        }
        return;
      }

      auto window = windowManager.getOrCreateWindow(message.index);

      if (!window) {
        auto defaultWindow = windowManager.getWindow(0);

        if (defaultWindow) {
          window = defaultWindow;
        }

        // @TODO: print warning
      }

      if (message.name == "heartbeat") {
        if (seq.size() > 0) {
          auto result = SSC::IPC::Result(message.seq, message, "heartbeat");
          window->resolvePromise(seq, OK_STATE, result.str());
        }

        return;
      }

      if (message.name == "resolve") {
        window->resolvePromise(seq, message.get("state"), encodeURIComponent(value));
        return;
      }

      if (message.name == "config") {
        auto key = message.get("key");
        window->resolvePromise(seq, OK_STATE, app.userConfig[key]);
        return;
      }

      if (message.name == "process.exit") {
        for (auto w : windowManager.windows) {
          if (w != nullptr) {
            auto window = windowManager.getWindow(w->opts.index);
            window->resolvePromise(message.seq, OK_STATE, value);
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
    [&](SSC::String const &code) {
      for (auto w : windowManager.windows) {
        if (w != nullptr) {
          auto window = windowManager.getWindow(w->opts.index);
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
  auto onMessage = [&](auto out) {
    // debug("onMessage %s", out.c_str());
    IPC::Message message(out, true);

    auto window = windowManager.getWindow(message.index);
    auto value = message.get("value");

    // the window must exist
    if (!window && message.index >= 0) {
      auto defaultWindow = windowManager.getWindow(0);

      if (defaultWindow) {
        window = defaultWindow;
      }
    }

    if (message.name == "process.open") {
      auto seq = message.get("seq");
      auto force = message.get("force") == "true" ? true : false;
      if (cmd.size() > 0) {
        if (process == nullptr || force) {
          createProcess(force);
          process->open();
        }
      #ifdef _WIN32
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
        window->resolvePromise(seq, OK_STATE, json);
        return;
      }
      window->resolvePromise(seq, ERROR_STATE, SSC::JSON::null);
      return;
    }

    if (message.name == "process.kill") {
      auto seq = message.get("seq");

      if (cmd.size() > 0 && process != nullptr) {
        killProcess(process);
      }

      window->resolvePromise(seq, OK_STATE, SSC::JSON::null);
      return;
    }

    if (message.name == "process.write") {
      auto seq = message.get("seq");
      if (cmd.size() > 0 && process != nullptr) {
        process->write(out);
      }
      window->resolvePromise(seq, OK_STATE, SSC::JSON::null);
      return;
    }

    if (message.name == "window.send") {
      const auto event = message.get("event");
      const auto value = message.get("value");
      const auto targetWindowIndex = message.get("targetWindowIndex").size() >= 0 ? std::stoi(message.get("targetWindowIndex")) : -1;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);
      const auto currentWindow = windowManager.getWindow(message.index);
      if (targetWindow) {
        targetWindow->eval(getEmitToRenderProcessJavaScript(event, value));
      }
      const auto seq = message.get("seq");
      currentWindow->resolvePromise(seq, OK_STATE, SSC::JSON::null);
      return;
    }

    if (message.name == "application.exit") {
      try {
        exitCode = std::stoi(value);
      } catch (...) {
      }

    #if defined(__APPLE__)
      if (app.wasLaunchedFromCli) {
        debug("__EXIT_SIGNAL__=%d", exitCode);
        CLI::notify();
      }
    #endif
      window->exit(exitCode);
      return;
    }

    if (message.name == "application.getScreenSize") {
      const auto seq = message.get("seq");
      const auto index = message.index;
      const auto window = windowManager.getWindow(index);
      if (window) {
        const auto screenSize = window->getScreenSize();
        const JSON::Object json = JSON::Object::Entries {
          { "width", screenSize.width },
          { "height", screenSize.height }
        };
        window->resolvePromise(seq, OK_STATE, json);
      }
      return;
    }

    if (message.name == "application.getWindows") {
      const auto index = message.index;
      const auto window = windowManager.getWindow(index);
      auto indices = splitToInts(value, ',');
      if (indices.size() == 0) {
        for (auto w : windowManager.windows) {
          if (w != nullptr) {
            indices.push_back(w->opts.index);
          }
        }
      }
      const auto result = windowManager.json(indices).str();
      window->resolvePromise(message.get("seq"), OK_STATE, result);
      return;
    }

    if (message.name == "window.create") {
      const auto seq = message.get("seq");

      auto currentWindowIndex = message.index < 0 ? 0 : message.index;
      auto currentWindow = windowManager.getWindow(currentWindowIndex);

      auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : 0;
      targetWindowIndex = targetWindowIndex < 0 ? 0 : targetWindowIndex;

      if (targetWindowIndex >= SSC_MAX_WINDOWS && message.get("headless") != "true" && message.get("debug") != "true") {
        const JSON::Object json = JSON::Object::Entries {
          { "err", String("Cannot create window with an index beyond ") + std::to_string(SSC_MAX_WINDOWS) }
        };
        currentWindow->resolvePromise(seq, ERROR_STATE, json);
        return;
      }

      auto targetWindow = windowManager.getWindow(targetWindowIndex);
      auto targetWindowStatus = windowManager.getWindowStatus(targetWindowIndex);

      if (targetWindow && targetWindowStatus != WindowManager::WindowStatus::WINDOW_NONE) {
        const JSON::Object json = JSON::Object::Entries {
          { "err", "Window with index " + std::to_string(targetWindowIndex) + " already exists" }
        };
        currentWindow->resolvePromise(seq, ERROR_STATE, json);
        return;
      }

      SSC::String error = getNavigationError(cwd, message.get("url"));
      if (error.size() > 0) {
        const JSON::Object json = SSC::JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", error}
          }}
        };
        currentWindow->resolvePromise(seq, ERROR_STATE, json);
        return;
      }

      // fill the options
      auto options = WindowOptions {};

      options.title = message.get("title");
      options.url = message.get("url");

      if (message.get("port").size() > 0) {
        options.port = std::stoi(message.get("port"));
      }

      if (message.get("radius").size() > 0) {
        options.radius = std::stof(message.get("radius"));
      }

      if (message.get("margin").size() > 0) {
        options.margin = std::stof(message.get("margin"));
      }

      auto screen = currentWindow->getScreenSize();

      options.width = message.get("width").size() ? currentWindow->getSizeInPixels(message.get("width"), screen.width) : 0;
      options.height = message.get("height").size() ? currentWindow->getSizeInPixels(message.get("height"), screen.height) : 0;

      options.minWidth = message.get("minWidth").size() ? currentWindow->getSizeInPixels(message.get("minWidth"), screen.width) : 0;
      options.minHeight = message.get("minHeight").size() ? currentWindow->getSizeInPixels(message.get("minHeight"), screen.height) : 0;
      options.maxWidth = message.get("maxWidth").size() ? currentWindow->getSizeInPixels(message.get("maxWidth"), screen.width) : screen.width;
      options.maxHeight = message.get("maxHeight").size() ? currentWindow->getSizeInPixels(message.get("maxHeight"), screen.height) : screen.height;

      options.canExit = message.get("canExit") == "true" ? true : false;

      options.headless = userConfig["build_headless"] == "true";

      if (message.get("headless") == "true") {
        options.headless = true;
      } else if (message.get("headless") == "false") {
        options.headless = false;
      }

      options.resizable = message.get("resizable") == "true" ? true : false;
      options.frameless = message.get("frameless") == "true" ? true : false;
      options.closable = message.get("closable") == "true" ? true : false;
      options.maximizable = message.get("maximizable") == "true" ? true : false;
      options.minimizable = message.get("minimizable") == "true" ? true : false;
      options.aspectRatio = message.get("aspectRatio");
      options.titlebarStyle = message.get("titlebarStyle");
      options.title = message.get("title");
      options.windowControlOffsets = message.get("windowControlOffsets");
      options.backgroundColorLight = message.get("backgroundColorLight");
      options.backgroundColorDark = message.get("backgroundColorDark");
      options.utility = message.get("utility") == "true" ? true : false;
      options.debug = message.get("debug") == "true" ? true : false;
      options.userScript = message.get("userScript");
      options.index = targetWindowIndex;
      options.runtimePrimordialOverrides = message.get("__runtime_primordial_overrides__");
      options.userConfig = INI::parse(message.get("config"));

      if (options.index >= SSC_MAX_WINDOWS) {
        options.preloadCommonJS = false;
      }

      targetWindow = windowManager.createWindow(options);

      targetWindow->show(EMPTY_SEQ);

      if (options.url.size() > 0) {
        targetWindow->navigate(seq, options.url);
      }

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() },
      };

      auto result = SSC::IPC::Result(message.seq, message, json);
      currentWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );
      return;
    }

    if (message.name == "window.close") {
      const auto index = message.index;
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : index;
      const auto window = windowManager.getWindow(index);
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);
      auto targetWindowStatus = windowManager.getWindowStatus(targetWindowIndex);

      if (targetWindow) {
        if (targetWindow->opts.canExit) {
          targetWindow->exit(0);
        } else {
          targetWindow->close(0);
        }

        JSON::Object json = JSON::Object::Entries {
          { "data", targetWindow->json()},
        };

        auto result = SSC::IPC::Result(message.seq, message, json);
        window->resolvePromise(
          message.seq,
          OK_STATE,
          result.json()
        );
      }
      return;
    }

    if (message.name == "window.show") {
      auto targetWindowIndex = std::stoi(message.get("targetWindowIndex"));
      targetWindowIndex = targetWindowIndex < 0 ? 0 : targetWindowIndex;
      auto index = message.index < 0 ? 0 : message.index;
      auto targetWindow = windowManager.getWindow(targetWindowIndex);
      auto targetWindowStatus = windowManager.getWindowStatus(targetWindowIndex);
      auto resolveWindow = windowManager.getWindow(index);

      if (!targetWindow || targetWindowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
        JSON::Object json = JSON::Object::Entries {
          { "err", targetWindowStatus }
        };

        auto result = SSC::IPC::Result(message.seq, message, json);
        resolveWindow->resolvePromise(
          message.seq,
          ERROR_STATE,
          result.json()
        );
        return;
      }

      targetWindow->show(EMPTY_SEQ);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() }
      };

      auto result = SSC::IPC::Result(message.seq, message, json);
      resolveWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );
      return;
    }

    if (message.name == "window.hide") {
      auto targetWindowIndex = std::stoi(message.get("targetWindowIndex"));
      targetWindowIndex = targetWindowIndex < 0 ? 0 : targetWindowIndex;
      auto index = message.index < 0 ? 0 : message.index;
      auto targetWindow = windowManager.getWindow(targetWindowIndex);
      auto targetWindowStatus = windowManager.getWindowStatus(targetWindowIndex);
      auto resolveWindow = windowManager.getWindow(index);

      if (!targetWindow || targetWindowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
        JSON::Object json = JSON::Object::Entries {
          { "err", targetWindowStatus }
        };

        auto result = SSC::IPC::Result(message.seq, message, json);
        resolveWindow->resolvePromise(
          message.seq,
          ERROR_STATE,
          result.json()
        );
        return;
      }

      targetWindow->hide(EMPTY_SEQ);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() }
      };

      auto result = SSC::IPC::Result(message.seq, message, json);
      resolveWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );

      return;
    }

    if (message.name == "window.setTitle") {
      const auto seq = message.seq;
      const auto currentIndex = message.index;
      const auto currentWindow = windowManager.getWindow(currentIndex);
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : currentIndex;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);

      targetWindow->setTitle(value);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() },
      };

      auto result = SSC::IPC::Result(message.seq, message, json);
      currentWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );

      return;
    }

    if (message.name == "window.navigate") {
      const auto seq = message.seq;
      const auto currentIndex = message.index;
      const auto currentWindow = windowManager.getWindow(currentIndex);
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : currentIndex;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);
      const auto url = message.get("url");
      const auto error = getNavigationError(cwd, url);

      if (error.size() > 0) {
        JSON::Object json = JSON::Object::Entries {
          { "err", error }
        };

        currentWindow->resolvePromise(
          message.seq,
          ERROR_STATE,
          json.str()
        );

        return;
      }

      targetWindow->navigate(seq, url);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() },
      };

      auto result = SSC::IPC::Result(message.seq, message, json);
      currentWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );

      return;
    }

    if (message.name == "window.showInspector") {
      const auto seq = message.seq;
      const auto currentIndex = message.index;
      const auto currentWindow = windowManager.getWindow(currentIndex);
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : currentIndex;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);

      targetWindow->showInspector();

      JSON::Object json = JSON::Object::Entries {
        { "data", true },
      };

      auto result = SSC::IPC::Result(message.seq, message, json);
      currentWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );

      return;
    }

    if (message.name == "window.setBackgroundColor") {
      int red = 0;
      int green = 0;
      int blue = 0;
      float alpha = 1;

      try {
        red = std::stoi(message.get("red"));
        green = std::stoi(message.get("green"));
        blue = std::stoi(message.get("blue"));
        alpha = std::stof(message.get("alpha"));
      } catch (...) {
      }

      const auto seq = message.seq;
      const auto currentIndex = message.index;
      const auto currentWindow = windowManager.getWindow(currentIndex);
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : currentIndex;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);

      targetWindow->setBackgroundColor(red, green, blue, alpha);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() },
      };

      auto result = SSC::IPC::Result(message.seq, message, json);
      currentWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );
      return;
    }

    if (message.name == "window.getBackgroundColor") {
      const auto seq = message.seq;
      const auto currentIndex = message.index;
      const auto currentWindow = windowManager.getWindow(currentIndex);
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : currentIndex;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);

      auto color = targetWindow->getBackgroundColor();

      JSON::Object json = JSON::Object::Entries {
        { "data", color },
      };

      auto result = SSC::IPC::Result(message.seq, message, json);

      currentWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );
      return;
    }

    if (message.name == "window.setSize") {
      const auto seq = message.seq;
      const auto currentIndex = message.index;
      const auto currentWindow = windowManager.getWindow(currentIndex);
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : currentIndex;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);

      auto screen = currentWindow->getScreenSize();

      float width = currentWindow->getSizeInPixels(message.get("width"), screen.width);
      float height = currentWindow->getSizeInPixels(message.get("height"), screen.height);

      targetWindow->setSize(width, height, 0);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() },
      };

      auto result = SSC::IPC::Result(message.seq, message, json);
      currentWindow->resolvePromise(
        message.seq,
        OK_STATE,
        result.json()
      );
      return;
    }

    if (message.name == "application.setTrayMenu") {
      const auto seq = message.get("seq");
      window->setTrayMenu(seq, value);
      return;
    }

    if (message.name == "application.setSystemMenu") {
      const auto seq = message.get("seq");
      window->setSystemMenu(seq, message.value);
      return;
    }

    if (message.name == "application.setSystemMenuItemEnabled") {
      const auto seq = message.get("seq");
      const auto enabled = message.get("enabled").find("true") != -1;
      int indexMain = 0;
      int indexSub = 0;

      try {
        indexMain = std::stoi(message.get("indexMain"));
        indexSub = std::stoi(message.get("indexSub"));
      } catch (...) {
        window->resolvePromise(
          message.seq,
          OK_STATE,
          SSC::JSON::null
        );
        return;
      }

      window->setSystemMenuItemEnabled(enabled, indexMain, indexSub);
      window->resolvePromise(
        message.seq,
        OK_STATE,
        SSC::JSON::null
      );
      return;
    }

    bool isMaximize = message.name == "window.maximize";
    bool isMinimize = message.name == "window.minimize";
    bool isRestore = message.name == "window.restore";

    if (isMaximize || isMinimize || isRestore) {
      const auto currentIndex = message.index;
      const auto currentWindow = windowManager.getWindow(currentIndex);
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0
        ? std::stoi(message.get("targetWindowIndex"))
        : currentIndex;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);

      if (isMaximize) {
        targetWindow->maximize();
        window->resolvePromise(message.seq, OK_STATE, SSC::JSON::null);
      }

      if (isMinimize) {
        targetWindow->minimize();
        window->resolvePromise(message.seq, OK_STATE, SSC::JSON::null);
      }

      if (isRestore) {
        targetWindow->restore();
        window->resolvePromise(message.seq, OK_STATE, SSC::JSON::null);
      }

      return;
    }

    if (message.name == "window.setContextMenu") {
      auto seq = message.get("seq");
      window->setContextMenu(seq, value);
      window->resolvePromise(
        message.seq,
        OK_STATE,
        SSC::JSON::null
      );
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
        {"url", out}
      }}
    };

    auto result = SSC::IPC::Result(message.seq, message, err);
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
  shutdownHandler = [&](int code) {
  #if defined(__linux__)
    unlink(appInstanceLock.c_str());
  #endif
    if (process != nullptr) {
      process->kill();
    }

    windowManager.destroy();

  #if defined(__APPLE__)
    if (app_ptr->wasLaunchedFromCli) {
      debug("__EXIT_SIGNAL__=%d", 0);
      CLI::notify();
    }
  #endif

    app_ptr->kill();
    exit(code);
  };

  app.onExit = shutdownHandler;

#if defined(__linux__)
  Thread mainThread([&]() {
#endif
    Vector<String> properties = {
      "window_width", "window_height",
      "window_min_width", "window_min_height",
      "window_max_width", "window_max_height"
    };

    auto setDefaultValue = [&](String property) {
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
      if (app.userConfig[property].size() > 0) {
        auto value = app.userConfig[property];
        if (!std::regex_match(value, validPattern)) {
          app.userConfig[property] = setDefaultValue(property);
          debug("Invalid value for %s: \"%s\". Setting it to \"%s\"", property.c_str(), value.c_str(), app.userConfig[property].c_str());
        }
      // set default value if it's not set in socket.ini
      } else {
        app.userConfig[property] = setDefaultValue(property);
      }
    }

    auto getProperty = [&](String property) {
      if (userConfig.count(property) > 0) {
        return userConfig[property];
      }

      return String("");
    };

    windowManager.configure(WindowManagerOptions {
      .defaultHeight = getProperty("window_height"),
      .defaultWidth = getProperty("window_width"),
      .defaultMinWidth = getProperty("window_min_width"),
      .defaultMinHeight = getProperty("window_min_height"),
      .defaultMaxWidth = getProperty("window_max_width"),
      .defaultMaxHeight = getProperty("window_max_height"),
      .isTest = isTest,
      .argv = argvArray.str(),
      .cwd = cwd,
      .userConfig = app.userConfig,
      .onMessage = onMessage,
      .onExit = shutdownHandler
    });

    auto isMaximizable = getProperty("window_maximizable");
    auto isMinimizable = getProperty("window_minimizable");
    auto isClosable = getProperty("window_closable");

    auto defaultWindow = windowManager.createDefaultWindow(WindowOptions {
      .resizable = getProperty("window_resizable") == "false" ? false : true,
      .minimizable = (isMinimizable == "" || isMinimizable == "true") ? true : false,
      .maximizable = (isMaximizable == "" || isMaximizable == "true") ? true : false,
      .closable = (isClosable == "" || isClosable == "true") ? true : false,
      .frameless = getProperty("window_frameless") == "true" ? true : false,
      .utility = getProperty("window_utility") == "true" ? true : false,
      .canExit = true,
      .titlebarStyle = getProperty("window_titlebar_style"),
      .windowControlOffsets = getProperty("mac_window_control_offsets"),
      .backgroundColorLight = getProperty("window_background_color_light"),
      .backgroundColorDark = getProperty("window_background_color_dark"),
      .userConfig = userConfig,
      .onExit = shutdownHandler
    });

    if (
      userConfig["webview_service_worker_mode"] != "hybrid" &&
      userConfig["permissions_allow_service_worker"] != "false"
    ) {
      auto serviceWorkerUserConfig = userConfig;
      auto screen = defaultWindow->getScreenSize();
      serviceWorkerUserConfig["webview_watch_reload"] = "false";
      auto serviceWorkerWindow = windowManager.createWindow({
        .canExit = false,
        .width = defaultWindow->getSizeInPixels("80%", screen.width),
        .height = defaultWindow->getSizeInPixels("80%", screen.height),
        .minWidth = defaultWindow->getSizeInPixels("40%", screen.width),
        .minHeight = defaultWindow->getSizeInPixels("30%", screen.height),
        .index = SSC_SERVICE_WORKER_CONTAINER_WINDOW_INDEX,
        .headless = Env::get("SOCKET_RUNTIME_SERVICE_WORKER_DEBUG").size() == 0,
        .userConfig = serviceWorkerUserConfig,
        .preloadCommonJS = false
      });

      app.core->serviceWorker.init(serviceWorkerWindow->bridge);
      serviceWorkerWindow->show(EMPTY_SEQ);
      serviceWorkerWindow->navigate(
        EMPTY_SEQ,
        "socket://" + userConfig["meta_bundle_identifier"] + "/socket/service-worker/index.html"
      );
    } else if (userConfig["webview_service_worker_mode"] == "hybrid") {
      app.core->serviceWorker.init(defaultWindow->bridge);
    }

    defaultWindow->show(EMPTY_SEQ);

    if (_port > 0) {
      defaultWindow->navigate(EMPTY_SEQ, _host + ":" + std::to_string(_port));
      defaultWindow->setSystemMenu(EMPTY_SEQ, String(
        "Developer Mode: \n"
        "  Reload: r + CommandOrControl\n"
        "  Quit: q + CommandOrControl\n"
        ";"
      ));
    } else {
      if (app.userConfig["webview_root"].size() != 0) {
        defaultWindow->navigate(
          EMPTY_SEQ,
          "socket://" + app.userConfig["meta_bundle_identifier"] + app.userConfig["webview_root"]
        );
      } else {
        defaultWindow->navigate(
          EMPTY_SEQ,
          "socket://" + app.userConfig["meta_bundle_identifier"] + "/index.html"
        );
      }
    }

    if (isReadingStdin) {
      String value;
      std::getline(std::cin, value);

      Thread t([&](String value) {
        auto app = App::instance();

        while (!app->core->domReady) {
          std::this_thread::sleep_for(std::chrono::milliseconds(128));
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
    while (app.run() == 0);
#if defined(__linux__)
  });
#endif

  //
  // If this is being run in a terminal/multiplexer
  //
#ifndef _WIN32
  installSignalHandler(SIGHUP, signalHandler);
#endif

#if defined(SIGUSR1)
  installSignalHandler(SIGUSR1, signalHandler);
#endif

  installSignalHandler(SIGINT, signalHandler);
  installSignalHandler(SIGTERM, signalHandler);

  const auto signalsDisabled = userConfig["application_signals"] == "false";
  const auto signals = parseStringList(userConfig["application_signals"]);

#define SET_DEFAULT_WINDOW_SIGNAL_HANDLER(sig) {                               \
  const auto name = String(CONVERT_TO_STRING(sig));                            \
  if (                                                                         \
    !signalsDisabled ||                                                        \
    std::find(signals.begin(), signals.end(), name) != signals.end()           \
  ) {                                                                          \
    installSignalHandler(sig, defaultWindowSignalHandler);                      \
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

#if defined(__linux__)
  if (mainThread.joinable()) {
    mainThread.join();
  }

  dbus_connection_unref(connection);
#endif

  exit(exitCode);
}
