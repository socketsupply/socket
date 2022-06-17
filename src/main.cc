#include "http.hh"
#include "common.hh"
#include "process.hh"

#if defined(_WIN32)
  #include "win.hh"
#elif defined(__APPLE__)
  #include "mac.hh"
#elif defined(__linux__)
  #include "linux.hh"
#endif

#if defined(_WIN32)
  #include <io.h>
  #define ISATTY _isatty
  #define FILENO _fileno
#else
  #include <unistd.h>
  #include <sys/wait.h>
  #define ISATTY isatty
  #define FILENO fileno
#endif

#define InvalidWindowIndexError(index) \
  std::string("Invalid index given for window: ") + std::to_string(index)

using namespace SSC;

std::function<void(int)> shutdownHandler;
void signalHandler(int signal) { shutdownHandler(signal); }

//
// the MAIN macro provides a cross-platform program entry point.
// it makes argc and argv uniformly available. It provides "instanceId"
// which on windows is hInstance, on mac and linux this is just an int.
//
MAIN {
  App app(instanceId);
  WindowFactory<Window, App> windowFactory(app);

  //
  // SETTINGS and DEBUG are compile time variables provided by the compiler.
  //
  constexpr auto _settings = STR_VALUE(SETTINGS);
  constexpr auto _debug = DEBUG;
  constexpr auto _port = PORT;

  const std::string OK_STATE = "0";
  const std::string ERROR_STATE = "1";
  const std::string EMPTY_SEQ = std::string("");

  auto cwd = app.getCwd(argv[0]);
  appData = parseConfig(decodeURIComponent(_settings));

  std::string suffix = "";

  std::stringstream argvArray;
  std::stringstream argvForward;

  bool isCommandMode = false;
  bool isTest = false;

  int exitCode = 0;
  int c = 0;

  bool wantsVersion = false;
  bool wantsHelp = false;

  // TODO right now we forward a json parsable string as the args but this
  // isn't the most robust way of doing this. possible a URI-encoded query
  // string would be more in-line with how everything else works.
  for (auto const arg : std::span(argv, argc)) {
    auto s = std::string(arg);

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
      argvForward << " " << std::string(arg);
    }
  }

  #if DEBUG == 1
    appData["name"] += "-dev";
    appData["title"] += "-dev";
  #endif

  appData["name"] += suffix;
  appData["title"] += suffix;

  argvForward << " --version=" << appData["version"];
  argvForward << " --name=" << appData["name"];

  #if DEBUG == 1
    argvForward << " --debug=1";
  #endif

  std::stringstream env;
  for (auto const &envKey : split(appData["env"], ',')) {
    auto cleanKey = trim(envKey);
    auto envValue = getEnv(cleanKey.c_str());

    env << std::string(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  std::string cmd;
  if (platform.os == "win32") {
    cmd = appData["win_cmd"];
  } else {
    cmd = appData[platform.os + "_cmd"];
  }

  if (cmd[0] == '.') {
    auto index = cmd.find_first_of('.');
    auto executable = cmd.substr(0, index);
    auto absPath = fs::path(cwd) / fs::path(executable);
    cmd = absPath.string() + cmd.substr(index);
  }

  if (isCommandMode) {
    argvForward << " --op-current-directory=" << fs::current_path();

    Process process(
      cmd,
      argvForward.str(),
      cwd,
      [&](std::string const &out) {
        Parse cmd(out);

        if (cmd.name != "exit") {
          std::cout << decodeURIComponent(cmd.get("value")) << std::endl;
        } else if (cmd.name == "exit") {
          exitCode = stoi(cmd.get("value"));
          exit(exitCode);
        }
      },
      [](std::string const &out) { std::cerr << out; },
      [](std::string const &code){ exit(std::stoi(code)); }
    );

    shutdownHandler = [&](int signum) {
      auto pid = process.getPID();
      process.kill(pid);
      exit(signum);
    };

    #ifndef _WIN32
      signal(SIGHUP, signalHandler);
    #endif

    signal(SIGINT, signalHandler);

    return exitCode;
  }

  int height = appData["height"].size() > 0 ? std::stoi(appData["height"]) : 0;
  int width = appData["width"].size() > 0 ? std::stoi(appData["width"]) : 0;

  auto onStdErr = [&](auto err) {
    std::cerr << err << std::endl;
  };

  //
  // # Main -> Render
  // Launch the main process and connect callbacks to the stdio and stderr pipes.
  //
  auto onStdOut = [&](std::string const &out) {
    //
    // ## Dispatch
    // Messages from the main process may be sent to the render process. If they
    // are parsable commands, try to do something with them, otherwise they are
    // just stdout and we can write the data to the pipe.
    //
    app.dispatch([&, out] {
      Parse cmd(out);

      auto value = cmd.get("value");
      auto seq = cmd.get("seq");

      if (cmd.name == "stdout") {
        writeToStdout(decodeURIComponent(value));
        return;
      }

      if (cmd.index > 0 && cmd.name.size() == 0) {
        // @TODO: print warning
        return;
      }

      if (cmd.index > SOCKET_MAX_WINDOWS) {
        // @TODO: print warning
        return;
      }

      if (cmd.name == "show") {
        auto index = cmd.index < 0 ? 0 : cmd.index;
        auto options = WindowOptions {};
        auto status = windowFactory.getWindowStatus(index);
        auto window = windowFactory.getWindow(index);

        options.title = cmd.get("title");
        options.url = cmd.get("url");

        if (cmd.get("port").size() > 0) {
          options.port = std::stoi(cmd.get("port"));
        }

        if (cmd.get("width").size() > 0 && cmd.get("height").size() > 0) {
          options.width = std::stoi(cmd.get("width"));
          options.height = std::stoi(cmd.get("height"));
        }

        if (!window || status == WindowFactory<Window, App>::WindowStatus::WINDOW_NONE) {
          options.resizable = cmd.get("resizable") == "true" ? true : false;
          options.frameless = cmd.get("frameless") == "true" ? true : false;
          options.utility = cmd.get("utility") == "true" ? true : false;
          options.debug = cmd.get("debug") == "true" ? true : false;
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

      auto window = windowFactory.getOrCreateWindow(cmd.index);

      if (!window) {
        auto defaultWindow = windowFactory.getWindow(0);

        if (defaultWindow) {
          window = defaultWindow;
        }

        // @TODO: print warning
      }

      if (cmd.name == "heartbeat") {
        if (seq.size() > 0) {
          window->onMessage(resolveToMainProcess(seq, OK_STATE, "\"heartbeat\""));
        }

        return;
      }

      if (cmd.name == "title") {
        window->setTitle(seq, decodeURIComponent(value));
        return;
      }

      if (cmd.name == "restart") {
        app.restart();
        return;
      }

      if (cmd.name == "hide") {
        window->hide(seq);
        return;
      }

      if (cmd.name == "navigate") {
        window->navigate(seq, decodeURIComponent(value));
        return;
      }

      if (cmd.name == "size") {
        int width = std::stoi(cmd.get("width"));
        int height = std::stoi(cmd.get("height"));
        window->setSize(seq, width, height, 0);
        return;
      }

      if (cmd.name == "getScreenSize") {
        auto size = window->getScreenSize();

        std::string value(
          "{"
            "\"width\":" + std::to_string(size.width) + ","
            "\"height\":" + std::to_string(size.height) + ""
          "}"
        );

        window->onMessage(resolveToMainProcess(
          seq,
          OK_STATE,
          encodeURIComponent(value)
        ));

        return;
      }

      if (cmd.name == "menu") {
        window->setSystemMenu(seq, decodeURIComponent(value));
        return;
      }

      if (cmd.name == "menuItemEnabled") {
        const auto enabled = cmd.get("enabled").find("true") != -1;
        int indexMain = 0;
        int indexSub = 0;

        try {
          indexMain = std::stoi(cmd.get("indexMain"));
          indexSub = std::stoi(cmd.get("indexSub"));
        } catch (...) {
          window->onMessage(resolveToMainProcess(seq, OK_STATE, ""));
          return;
        }

        window->setSystemMenuItemEnabled(enabled, indexMain, indexSub);
        window->onMessage(resolveToMainProcess(seq, OK_STATE, ""));
        return;
      }

      if (cmd.name == "external") {
        window->openExternal(decodeURIComponent(value));
        if (seq.size() > 0) {
          window->onMessage(resolveToMainProcess(seq, OK_STATE, "null"));
        }
        return;
      }

      if (cmd.name == "exit") {
        try {
          exitCode = std::stoi(value);
        } catch (...) {
        }

        window->exit(exitCode);

        if (seq.size() > 0) {
          window->onMessage(resolveToMainProcess(seq, OK_STATE, "null"));
        }
        return;
      }

      if (cmd.name == "resolve") {
        window->eval(resolveToRenderProcess(seq, cmd.get("state"), value));
        return;
      }

      if (cmd.name == "send") {
        window->eval(emitToRenderProcess(
          decodeURIComponent(cmd.get("event")),
          value
        ));
        return;
      }

      if (cmd.name == "getConfig") {
        window->onMessage(resolveToMainProcess(seq, OK_STATE, _settings));
        return;
      }
    });
  };

  Process process(
    cmd,
    argvForward.str(),
    cwd,
    onStdOut,
    onStdErr,
    [&](std::string const &code) {
      for (auto& window : windowFactory.windows) {
        window->eval(emitToRenderProcess("main-exit", code));
      }
    }
  );

  //
  // # Render -> Main
  // Send messages from the render processes to the main process.
  // These may be similar to how we route the messages from the
  // main process but different enough that duplication is ok. This
  // callback doesnt need to dispatch because it's already in the
  // main thread.
  //
  auto onMessage = [&](auto out) {
    Parse cmd(out);

    auto window = windowFactory.getWindow(cmd.index);

    // the window must exist
    if (!window && cmd.index >= 0) {
      const auto seq = cmd.get("seq");
      auto defaultWindow = windowFactory.getWindow(0);

      if (defaultWindow) {
        window = defaultWindow;
      }
    }

    if (cmd.name == "reload") {
      process.reload();
      return;
    }

    if (cmd.name == "title") {
      window->setTitle(
        cmd.get("seq"),
        decodeURIComponent(cmd.get("value"))
      );
      return;
    }

    #if IOS == 0 && ANDROID == 0
      if (cmd.name == "bootstrap") {
        auto src = appData[platform.os + "_bootstrap_src"].c_str();
        auto dest = appData[platform.os + "_bootstrap_dest"].c_str();

        if (fs::exists(dest)) return;

        std::ofstream f(dest);

        if (!f) {
          std::cerr << "Failed to open " << dest << std::endl;
          return;
        }

        httplib::ContentReceiver onContent = [&](
            const char *data,
            size_t len) -> bool {
          f.write(data, len);
          return true;
        };

        auto onProgress = [&](uint64_t current, uint64_t total) -> bool {
          auto p = current * 100 / total;
          auto progress = "\"" + std::to_string(p) + "\"";
          window->eval(emitToRenderProcess("main-bootstrap-progress", progress));

          if (p != 1) return true;

          auto r = exec(appData[platform.os + "_bootstrap_post"]);

          if (r.exitCode == 0) {
            window->eval(emitToRenderProcess("main-bootstrap-success", progress));
          } else {
            auto msg = r.output.size() > 0 ? r.output : "\"Command failed\"";
            window->eval(emitToRenderProcess("main-bootstrap-failure", msg));
          }

          return true;
        };

        const httplib::Headers h;
        auto client = httplib::Client("https://go.microsoft.com");
        auto res = client.Get("/fwlink/p/?LinkId=2124703", onContent, onProgress);

        if (res->status != 200) {
          auto msg = "{\"status\":" + std::to_string(res->status) + "}";
          window->eval(emitToRenderProcess("main-bootstrap-failure", msg));
          return;
        }
      }
    #endif

    if (cmd.name == "exit") {
      try {
        exitCode = std::stoi(decodeURIComponent(cmd.get("value")));
      } catch (...) {
      }

      window->exit(exitCode);
      return;
    }

    if (cmd.name == "hide") {
      window->hide(EMPTY_SEQ);
      return;
    }

    if (cmd.name == "inspect") {
      window->showInspector();
      return;
    }

    if (cmd.name == "background") {
      int red = 0;
      int green = 0;
      int blue = 0;
      float alpha = 1;

      try {
        red = std::stoi(cmd.get("red"));
        green = std::stoi(cmd.get("green"));
        blue = std::stoi(cmd.get("blue"));
        alpha = std::stof(cmd.get("alpha"));
      } catch (...) {
      }

      window->setBackgroundColor(red, green, blue, alpha);
      return;
    }

    if (cmd.name == "size") {
      int width = std::stoi(cmd.get("width"));
      int height = std::stoi(cmd.get("height"));
      window->setSize(EMPTY_SEQ, width, height, 0);
      return;
    }

    if (cmd.name == "external") {
      window->openExternal(decodeURIComponent(cmd.get("value")));
      return;
    }

    if (cmd.name == "menuItemEnabled") {
      const auto seq = cmd.get("seq");
      const auto enabled = cmd.get("enabled").find("true") != -1;
      int indexMain = 0;
      int indexSub = 0;

      try {
        indexMain = std::stoi(cmd.get("indexMain"));
        indexSub = std::stoi(cmd.get("indexSub"));
      } catch (...) {
        window->onMessage(resolveToMainProcess(seq, OK_STATE, ""));
        return;
      }

      window->setSystemMenuItemEnabled(enabled, indexMain, indexSub);
      window->onMessage(resolveToMainProcess(seq, OK_STATE, ""));
      return;
    }

    if (cmd.name == "dialog") {
      bool isSave = cmd.get("type").compare("save") == 0;
      bool allowDirs = cmd.get("allowDirs").compare("true") == 0;
      bool allowFiles = cmd.get("allowFiles").compare("true") == 0;
      bool allowMultiple = cmd.get("allowMultiple").compare("true") == 0;
      std::string defaultName = decodeURIComponent(cmd.get("defaultName"));
      std::string defaultPath = decodeURIComponent(cmd.get("defaultPath"));
      std::string title = decodeURIComponent(cmd.get("title"));

      window->openDialog(
        cmd.get("seq"),
        isSave,
        allowDirs,
        allowFiles,
        allowMultiple,
        defaultPath,
        title,
        defaultName
      );

      return;
    }

    if (cmd.name == "context") {
      auto seq = cmd.get("seq");
      auto value = decodeURIComponent(cmd.get("value"));
      window->setContextMenu(seq, value);
      return;
    }

    if (cmd.name == "getConfig") {
      const auto seq = cmd.get("seq");
      auto wrapped = ("\"" + std::string(_settings) + "\"");
      window->eval(resolveToRenderProcess(seq, OK_STATE, encodeURIComponent(wrapped)));
      return;
    }

    //
    // Everything else can be forwarded to the main process.
    // The protocol requires messages must be terminated by a newline.
    //
    process.write(out);
  };

  //
  // # Exiting
  //
  // When a window or the app wants to exit,
  // we clean up the windows and the main process.
  //
  shutdownHandler = [&](int code) {
    auto pid = process.getPID();

    process.kill(pid);
    windowFactory.destroy();
    app.kill();

    exit(code);
  };

  app.onExit = shutdownHandler;

  windowFactory.configure(WindowFactoryOptions {
    .defaultHeight = height,
    .defaultWidth = width,
    .isTest = isTest,
    .argv = argvArray.str(),
    .cwd = cwd,
    .onMessage = onMessage,
    .onExit = shutdownHandler,
  });

  Window* defaultWindow = windowFactory.createDefaultWindow(WindowOptions { });

  // windowFactory.getOrCreateWindow(0);
  windowFactory.getOrCreateWindow(1);

  if (_port > 0 || cmd.size() == 0) {
    defaultWindow->setSystemMenu(EMPTY_SEQ, std::string(
      "Developer Mode: \n"
      "  Reload: r + CommandOrControl\n"
      "  Quit: q + CommandOrControl\n"
      ";"
    ));

    defaultWindow->show(EMPTY_SEQ);
    defaultWindow->setSize(EMPTY_SEQ, 1024, 720, 0);
  }

  if (_port > 0) {
    defaultWindow->navigate(EMPTY_SEQ, "http://localhost:" + std::to_string(_port));
  } else if (cmd.size() == 0) {
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
