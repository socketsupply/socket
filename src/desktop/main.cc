#include "../app/app.hh"
#include "../process/process.hh"
#include "../window/window.hh"
#include "../ipc/ipc.hh"

//
// A cross platform MAIN macro that
// magically gives us argc and argv.
//
#if defined(_WIN32)
#define MAIN                         \
  static const int argc = __argc;    \
  static const char** argv = __argv; \
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

#define InvalidWindowIndexError(index) \
  SSC::String("Invalid index given for window: ") + std::to_string(index)

#ifndef PORT
#define PORT 0
#endif

using namespace SSC;

std::function<void(int)> shutdownHandler;
void signalHandler (int signal) {
  if (shutdownHandler != nullptr) {
    shutdownHandler(signal);
  }
}

//
// the MAIN macro provides a cross-platform program entry point.
// it makes argc and argv uniformly available. It provides "instanceId"
// which on windows is hInstance, on mac and linux this is just an int.
//
MAIN {
  App app(instanceId);
  WindowFactory windowFactory(app);

  app.setWindowFactory(&windowFactory);

  //
  // SSC_SETTINGS and DEBUG are compile time variables provided by the compiler.
  //
  constexpr auto _settings = STR_VALUE(SSC_SETTINGS);
  constexpr auto _debug = DEBUG;
  constexpr auto _port = PORT;

  const SSC::String OK_STATE = "0";
  const SSC::String ERROR_STATE = "1";
  const SSC::String EMPTY_SEQ = SSC::String("");

  auto cwd = app.getCwd();
  app.appData = parseConfig(decodeURIComponent(_settings));

  SSC::String suffix = "";

  SSC::StringStream argvArray;
  SSC::StringStream argvForward;

  bool isCommandMode = false;
  bool isHeadless = false;
  bool isTest = false;

  int exitCode = 0;
  int c = 0;

  bool wantsVersion = false;
  bool wantsHelp = false;
  bool fromSSC = false; // launched from the `ssc` cli

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

    if (helpRequested) {
      wantsHelp = true;
    }

    if (versionRequested) {
      wantsVersion = true;
    }

    if (s.find("--headless") == 0) {
      isHeadless = true;
    }

    if (s.find("--from-ssc") == 0) {
      fromSSC = true;
      app.fromSSC = true;
    }

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

  #if DEBUG == 1
    app.appData["name"] += "-dev";
    app.appData["title"] += "-dev";
  #endif

  app.appData["name"] += suffix;
  app.appData["title"] += suffix;

  argvForward << " --version=v" << app.appData["version"];
  argvForward << " --name=" << app.appData["name"];

  #if DEBUG == 1
    argvForward << " --debug=1";
  #endif

  SSC::StringStream env;
  for (auto const &envKey : split(app.appData["env"], ',')) {
    auto cleanKey = trim(envKey);
    auto envValue = getEnv(cleanKey.c_str());

    env << SSC::String(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  SSC::String cmd;
  if (platform.os == "win32") {
    cmd = app.appData["win_cmd"];
  } else {
    cmd = app.appData[platform.os + "_cmd"];
  }

  if (cmd[0] == '.') {
    auto index = cmd.find_first_of('.');
    auto executable = cmd.substr(0, index);
    auto absPath = fs::path(cwd) / fs::path(executable);
    cmd = absPath.string() + cmd.substr(index);
  }

  static Process* process = nullptr;
  static std::function<void(bool)> createProcess;

  auto killProcess = [&](Process* process) {
    if (process != nullptr) {
        auto pid = process->getPID();
        process->kill(pid);
        delete process;
        process = nullptr;
    }
  };

  auto createProcessTemplate = [&]<class... Args>(Args... args) {
    return [=](bool force) {
      if (process != nullptr) {
        killProcess(process);
      }
      process = new Process(args...);
    };
  };

  if (isCommandMode) {
    argvForward << " --cwd=" << fs::current_path();

    createProcess = createProcessTemplate(
      cmd,
      argvForward.str(),
      cwd,
      [&](SSC::String const &out) {
        IPC::Message message(out);

        if (message.name != "exit") {
          stdWrite(decodeURIComponent(message.get("value")), false);
        } else if (message.name == "exit") {
          exitCode = stoi(message.get("value"));
          exit(exitCode);
        }
      },
      [](SSC::String const &out) { stdWrite(out, true); },
      [](SSC::String const &code){ exit(std::stoi(code)); }
    );

    if (cmd.size() == 0) {
      stdWrite("No " + platform.os + "_cmd provided in ssc.config", true);
      exit(1);
    }

    createProcess(true);

    shutdownHandler = [&](int signum) {
      if (process != nullptr) {
        auto pid = process->getPID();
        process->kill(pid);
      }
      exit(signum);
    };

    #ifndef _WIN32
      signal(SIGHUP, signalHandler);
    #endif

    signal(SIGINT, signalHandler);

    return exitCode;
  }

  String initialHeight = app.appData["height"].size() > 0 ? app.appData["height"] : "100%";
  String initialWidth = app.appData["width"].size() > 0 ? app.appData["width"] : "100%";

  bool isHeightInPercent = initialHeight.back() == '%';
  bool isWidthInPercent = initialWidth.back() == '%';

  if (isHeightInPercent) initialHeight.pop_back();
  if (isWidthInPercent) initialWidth.pop_back();

  auto height = std::stof(initialHeight);
  auto width = std::stof(initialWidth);

  if (height < 0) height = 0;
  if (width < 0) width = 0;

  auto onStdErr = [&](auto err) {
    for (auto& window : windowFactory.windows) {
      if (window != nullptr) {
        window->eval(getEmitToRenderProcessJavaScript("process-error", err));
      }
    }
  };

  //
  // # Main -> Render
  // Launch the main process and connect callbacks to the stdio and stderr pipes.
  //
  auto onStdOut = [&](SSC::String const &out) {
    //
    // ## Dispatch
    // Messages from the main process may be sent to the render process. If they
    // are parsable commands, try to do something with them, otherwise they are
    // just stdout and we can write the data to the pipe.
    //
    app.dispatch([&, out] {
      IPC::Message message(out);

      auto value = message.get("value");
      auto seq = message.get("seq");

      if (message.name == "log" || message.name == "stdout") {
        stdWrite(decodeURIComponent(value), false);
        return;
      }

      if (message.name == "stderr") {
        stdWrite(decodeURIComponent(value), true);
        return;
      }

      if (message.index > 0 && message.name.size() == 0) {
        // @TODO: print warning
        return;
      }

      if (message.index > SSC_MAX_WINDOWS) {
        // @TODO: print warning
        return;
      }

      if (message.name == "show") {
        auto index = message.index < 0 ? 0 : message.index;
        auto options = WindowOptions {};
        auto status = windowFactory.getWindowStatus(index);
        auto window = windowFactory.getWindow(index);

        options.title = message.get("title");
        options.url = message.get("url");

        if (message.get("port").size() > 0) {
          options.port = std::stoi(message.get("port"));
        }

        if (message.get("width").size() > 0 && message.get("height").size() > 0) {
          options.width = std::stoi(message.get("width"));
          options.height = std::stoi(message.get("height"));
        }

        if (!window || status == WindowFactory::WindowStatus::WINDOW_NONE) {
          options.resizable = message.get("resizable") == "true" ? true : false;
          options.frameless = message.get("frameless") == "true" ? true : false;
          options.utility = message.get("utility") == "true" ? true : false;
          options.debug = message.get("debug") == "true" ? true : false;
          options.index = index;

          window = windowFactory.createWindow(options);
          window->show(seq);
        } else {
          window->show(seq);
        }

        if (window) {
          if (options.width > 0 && options.height > 0) {
            window->setSize(EMPTY_SEQ, options.width, options.height, 0);
          }

          if (options.title.size() > 0) {
            window->setTitle(EMPTY_SEQ, options.title);
          }

          if (options.url.size() > 0) {
            window->openExternal(options.url);
          }
        }

        return;
      }

      auto window = windowFactory.getOrCreateWindow(message.index);

      if (!window) {
        auto defaultWindow = windowFactory.getWindow(0);

        if (defaultWindow) {
          window = defaultWindow;
        }

        // @TODO: print warning
      }

      if (message.name == "heartbeat") {
        if (seq.size() > 0) {
          window->resolvePromise(seq, OK_STATE, "\"heartbeat\"");
        }

        return;
      }

      if (message.name == "title") {
        window->setTitle(seq, decodeURIComponent(value));
        return;
      }

      if (message.name == "restart") {
        app.restart();
        return;
      }

      if (message.name == "hide") {
        window->hide(seq);
        return;
      }

      if (message.name == "navigate") {
        window->navigate(seq, decodeURIComponent(value));
        return;
      }

      if (message.name == "size") {
        int width = std::stoi(message.get("width"));
        int height = std::stoi(message.get("height"));
        window->setSize(seq, width, height, 0);
        return;
      }

      if (message.name == "getScreenSize") {
        auto size = window->getScreenSize();

        SSC::String value(
          "{"
            "\"width\":" + std::to_string(size.width) + ","
            "\"height\":" + std::to_string(size.height) + ""
          "}"
        );

        window->resolvePromise(
          seq,
          OK_STATE,
          encodeURIComponent(value)
        );

        return;
      }

      if (message.name == "menu") {
        window->setSystemMenu(seq, decodeURIComponent(value));
        return;
      }

      if (message.name == "menuItemEnabled") {
        const auto enabled = message.get("enabled").find("true") != -1;
        int indexMain = 0;
        int indexSub = 0;

        try {
          indexMain = std::stoi(message.get("indexMain"));
          indexSub = std::stoi(message.get("indexSub"));
        } catch (...) {
          window->resolvePromise(seq, OK_STATE, "");
          return;
        }

        window->setSystemMenuItemEnabled(enabled, indexMain, indexSub);
        window->resolvePromise(seq, OK_STATE, "");
        return;
      }

      if (message.name == "external") {
        window->openExternal(decodeURIComponent(value));
        if (seq.size() > 0) {
          window->resolvePromise(seq, OK_STATE, "null");
        }
        return;
      }

      if (message.name == "exit") {
        try {
          exitCode = std::stoi(value);
        } catch (...) {
        }

        window->exit(exitCode);

        if (seq.size() > 0) {
          window->resolvePromise(seq, OK_STATE, "null");
        }
        return;
      }

      if (message.name == "resolve") {
        window->resolvePromise(seq, message.get("state"), encodeURIComponent(value));
        return;
      }

      if (message.name == "send") {
        window->eval(getEmitToRenderProcessJavaScript(
          decodeURIComponent(message.get("event")),
          value
        ));
        return;
      }

      if (message.name == "getConfig") {
        window->resolvePromise(seq, OK_STATE, _settings);
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
      for (auto& window : windowFactory.windows) {
        window->eval(getEmitToRenderProcessJavaScript("main-exit", code));
      }
    }
  );

  if (cmd.size() > 0) {
    createProcess(true);
  }

  //
  // # Render -> Main
  // Send messages from the render processes to the main process.
  // These may be similar to how we route the messages from the
  // main process but different enough that duplication is ok. This
  // callback doesnt need to dispatch because it's already in the
  // main thread.
  //
  auto onMessage = [&](auto out) {
    IPC::Message message(out);

    auto window = windowFactory.getWindow(message.index);
    auto value = message.get("value");

    // the window must exist
    if (!window && message.index >= 0) {
      auto defaultWindow = windowFactory.getWindow(0);

      if (defaultWindow) {
        window = defaultWindow;
      }
    }

    if (message.name == "process.open") {
      if (cmd.size() > 0) {
        auto force = message.get("force") == "true" ? true : false;
        createProcess(force);
        process->open();
      }
      return;
    }

    if (message.name == "process.kill") {
      if (cmd.size() > 0 && process != nullptr) {
        killProcess(process);
      }
      return;
    }

    if (message.name == "title") {
      window->setTitle(
        message.seq,
        decodeURIComponent(value)
      );
      return;
    }

    if (message.name == "log" || message.name == "stdout") {
      stdWrite(decodeURIComponent(value), false);
      return;
    }

    if (message.name == "stderr") {
      stdWrite(decodeURIComponent(value), true);
      return;
    }

    if (message.name == "exit") {
      try {
        exitCode = std::stoi(decodeURIComponent(value));
      } catch (...) {
      }

      window->exit(exitCode);
      return;
    }

    if (message.name == "hide") {
      window->hide(EMPTY_SEQ);
      return;
    }

    if (message.name == "navigate") {
      if (!value.starts_with("file://")) {
        debug("Navigation error: only file:// protocol is allowed. Got path %s", value.c_str());
        return;
      }
      if (!value.substr(7).starts_with(cwd)) {
        debug("Navigation error: only files in the current directory are allowed. Got path %s", value.c_str());
        return;
      }
      if (!value.ends_with(".html")) {
        debug("Navigation error: only .html files are allowed. Got path %s", value.c_str());
        return;
      }
      if (value.find("/../") != std::string::npos) {
        debug("Navigation error: relative paths are not allowed. Got path %s", value.c_str());
        return;
      }
      const auto seq = message.get("seq");
      window->navigate(seq, decodeURIComponent(value));
      return;
    }

    if (message.name == "inspect") {
      window->showInspector();
      return;
    }

    if (message.name == "background") {
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

      window->setBackgroundColor(red, green, blue, alpha);
      return;
    }

    if (message.name == "size") {
      int width = std::stoi(message.get("width"));
      int height = std::stoi(message.get("height"));
      window->setSize(EMPTY_SEQ, width, height, 0);
      return;
    }

    if (message.name == "external") {
      window->openExternal(decodeURIComponent(value));
      return;
    }

    if (message.name == "menu") {
      const auto seq = message.get("seq");
      window->setSystemMenu(seq, decodeURIComponent(value));
      return;
    }

    if (message.name == "menuItemEnabled") {
      const auto seq = message.get("seq");
      const auto enabled = message.get("enabled").find("true") != -1;
      int indexMain = 0;
      int indexSub = 0;

      try {
        indexMain = std::stoi(message.get("indexMain"));
        indexSub = std::stoi(message.get("indexSub"));
      } catch (...) {
        window->resolvePromise(seq, OK_STATE, "");
        return;
      }

      window->setSystemMenuItemEnabled(enabled, indexMain, indexSub);
      window->resolvePromise(seq, OK_STATE, "");
      return;
    }

    if (message.name == "dialog") {
      bool bSave = message.get("type").compare("save") == 0;
      bool bDirs = message.get("allowDirs").compare("true") == 0;
      bool bFiles = message.get("allowFiles").compare("true") == 0;
      bool bMulti = message.get("allowMultiple").compare("true") == 0;
      SSC::String defaultName = decodeURIComponent(message.get("defaultName"));
      SSC::String defaultPath = decodeURIComponent(message.get("defaultPath"));
      SSC::String title = decodeURIComponent(message.get("title"));

      window->openDialog(message.get("seq"), bSave, bDirs, bFiles, bMulti, defaultPath, title, defaultName);
      return;
    }

    if (message.name == "context") {
      auto seq = message.get("seq");
      window->setContextMenu(seq, decodeURIComponent(value));
      return;
    }

    if (message.name == "getConfig") {
      const auto seq = message.get("seq");
      auto wrapped = ("\"" + String(_settings) + "\"");
      window->resolvePromise(seq, OK_STATE, encodeURIComponent(wrapped));
      return;
    }

    //
    // Everything else can be forwarded to the main process.
    // The protocol requires messages must be terminated by a newline.
    //
    if (process != nullptr) {
      process->write(out);
    }
  };

  //
  // # Exiting
  //
  // When a window or the app wants to exit,
  // we clean up the windows and the main process.
  //
  shutdownHandler = [&](int code) {
    if (process != nullptr) {
      auto pid = process->getPID();
      process->kill(pid);
    }
    windowFactory.destroy();
    app.kill();

    exit(code);
  };

  app.onExit = shutdownHandler;

  windowFactory.configure(WindowFactoryOptions {
    .defaultHeight = height,
    .defaultWidth = width,
    .isHeightInPercent = isHeightInPercent,
    .isWidthInPercent = isWidthInPercent,
    .headless = isHeadless,
    .isTest = isTest,
    .argv = argvArray.str(),
    .cwd = cwd,
    .appData = app.appData,
    .onMessage = onMessage,
    .onExit = shutdownHandler
  });

  Window* defaultWindow = windowFactory.createDefaultWindow(WindowOptions { });

  // windowFactory.getOrCreateWindow(0);
  windowFactory.getOrCreateWindow(1);

  defaultWindow->show(EMPTY_SEQ);

  if (_port > 0) {
    defaultWindow->navigate(EMPTY_SEQ, "http://localhost:" + std::to_string(_port));
    defaultWindow->setSystemMenu(EMPTY_SEQ, String(
      "Developer Mode: \n"
      "  Reload: r + CommandOrControl\n"
      "  Quit: q + CommandOrControl\n"
      ";"
    ));
  } else {
    defaultWindow->navigate(EMPTY_SEQ, "file://" + (fs::path(cwd) / "index.html").string());
  }

  //
  // If this is being run in a terminal/multiplexer
  //
  #ifndef _WIN32
    signal(SIGHUP, signalHandler);
  #endif

  signal(SIGINT, signalHandler);

  //
  // # Event Loop
  // start the platform specific event loop for the main
  // thread and run it until it returns a non-zero int.
  //
  while(app.run() == 0);

  exit(exitCode);
}
