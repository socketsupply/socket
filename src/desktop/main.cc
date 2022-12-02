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

void navigate (Window* window, const String &cwd, const String &seq, const String &value) {
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
  window->navigate(seq, value);
  return;
}

//
// the MAIN macro provides a cross-platform program entry point.
// it makes argc and argv uniformly available. It provides "instanceId"
// which on windows is hInstance, on mac and linux this is just an int.
//
MAIN {
  App app(instanceId);
  WindowManager windowManager(app);

  app.setWindowManager(&windowManager);

  const auto _settings = SSC::getSettingsSource();
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

  if (isDebugEnabled()) {
    app.appData["name"] += "-dev";
  }

  app.appData["name"] += suffix;

  argvForward << " --version=v" << app.appData["version"];
  argvForward << " --name=" << app.appData["name"];

  if (isDebugEnabled()) {
    argvForward << " --debug=1";
  }

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
    //
    // ## Dispatch
    // Messages from the backend process may be sent to the render process. If they
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

      if (message.name == "window.show") {
        auto index = message.index < 0 ? 0 : message.index;
        auto options = WindowOptions {};
        auto status = windowManager.getWindowStatus(index);
        auto window = windowManager.getWindow(index);

        options.title = message.get("title");
        options.url = message.get("url");

        if (message.get("port").size() > 0) {
          options.port = std::stoi(message.get("port"));
        }

        if (message.get("width").size() > 0 && message.get("height").size() > 0) {
          options.width = std::stoi(message.get("width"));
          options.height = std::stoi(message.get("height"));
        }

        if (!window || status == WindowManager::WindowStatus::WINDOW_NONE) {
          options.resizable = message.get("resizable") == "true" ? true : false;
          options.frameless = message.get("frameless") == "true" ? true : false;
          options.utility = message.get("utility") == "true" ? true : false;
          options.debug = message.get("debug") == "true" ? true : false;
          options.index = index;

          window = windowManager.createWindow(options);
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

      if (message.name == "send") {
        if (message.index >= 0) {
          auto window = windowManager.getWindow(message.index);
          if (window) {
            window->eval(getEmitToRenderProcessJavaScript(
              decodeURIComponent(message.get("event")),
              value
            ));
          }
        } else {
          for (auto w : windowManager.windows) {
            if (w != nullptr) {
              auto window = windowManager.getWindow(w->opts.index);
              window->eval(getEmitToRenderProcessJavaScript(
                decodeURIComponent(message.get("event")),
                value
              ));
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
          window->resolvePromise(seq, OK_STATE, "\"heartbeat\"");
        }

        return;
      }

      if (message.name == "window.setTitle") {
        window->setTitle(seq, decodeURIComponent(value));
        return;
      }

      if (message.name == "restart") {
        app.restart();
        return;
      }

      if (message.name == "window.hide") {
        window->hide(seq);
        return;
      }

      if (message.name == "window.navigate") {
        window->navigate(seq, decodeURIComponent(value));
        return;
      }

      if (message.name == "window.setSize") {
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
          window->resolvePromise(seq, OK_STATE, "null");
          return;
        }

        window->setSystemMenuItemEnabled(enabled, indexMain, indexSub);
        window->resolvePromise(seq, OK_STATE, "null");
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
      for (auto w : windowManager.windows) {
        if (w != nullptr) {
          auto window = windowManager.getWindow(w->opts.index);
          window->eval(getEmitToRenderProcessJavaScript("backend-exit", code));
        }
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
  // backend process but different enough that duplication is ok. This
  // callback doesnt need to dispatch because it's already in the
  // main thread.
  //
  auto onMessage = [&](auto out) {
    debug("onMessage %s", out.c_str());
    IPC::Message message(out);

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
      if (cmd.size() > 0) {
        auto force = message.get("force") == "true" ? true : false;
        createProcess(force);
        process->open();
        window->resolvePromise(seq, OK_STATE, "null");
        return;
      }
      window->resolvePromise(seq, ERROR_STATE, "null");
      return;
    }

    if (message.name == "process.kill") {
      auto seq = message.get("seq");
      if (cmd.size() > 0 && process != nullptr) {
        killProcess(process);
      }
      // TODO: crashes the app with a segfault `libc++abi: terminating with uncaught exception of type std::__1::system_error: mutex lock failed: Invalid argument`
      window->resolvePromise(seq, OK_STATE, "null");
      return;
    }

    if (message.name == "window.setTitle") {
      const auto currentIndex = message.index;
      const auto index = message.get("window").size() > 0 ? std::stoi(message.get("window")) : currentIndex;
      const auto window = windowManager.getWindow(index);
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

    if (message.name == "getWindows") {
      const auto index = message.index;
      const auto showTitle = message.get("title") == "true" ? true : false;
      const auto showSize = message.get("size") == "true" ? true : false;
      const auto showStatus = message.get("status") == "true" ? true : false;
      const auto window = windowManager.getWindow(index);
      StringStream ss;
      bool first = true;
      ss << "[";
      for (auto windowWithMetadata : windowManager.windows) {
        if (windowWithMetadata != nullptr) {
          if (!first) {
            ss << ",";
          } else {
            first = false;
          }
          if (!showTitle && !showSize && !showStatus) {
            ss << windowWithMetadata->opts.index;
          } else {
            const auto w = windowManager.getWindow(windowWithMetadata->opts.index);
            ss << "{";
            if (showTitle) {
              const auto title = w->getTitle();
              ss << "\"title\":\"" << title << "\",";
            }
            if (showSize) {
              const auto size = w->getSize();
              ss << "\"width\":" << size.width << ",";
              ss << "\"height\":" << size.height << ",";
            }
            if (showStatus) {
              const auto status = windowWithMetadata->status;
              ss << "\"status\":" << status << ",";
            }
            ss << "\"index\":" << windowWithMetadata->opts.index;
            ss << "}";
          }
        }
      }
      ss << "]";

      auto result = ss.str();
      window->resolvePromise(message.get("seq"), OK_STATE, encodeURIComponent(result));
      return;
    }

    // if (message.name == "window") {
    //   const auto index = message.index;
    //   auto window = windowManager.getWindow(index);
    //   if (window) {
    //     const auto size = window->getSize();
    //     const auto title = window->getTitle();
    //     JSON::Object value = JSON::Object::Entries {
    //       {"index", index},
    //       {"title", title},
    //       {"width", size.width},
    //       {"height", size.height},
    //       {"status", window->status},
    //     };
    //     const auto seq = message.get("seq");
    //     window->resolvePromise(
    //       seq,
    //       OK_STATE,
    //       JSON::Object(value).str()
    //     );
    //   } else {
    //     window->resolvePromise(
    //       message.get("seq"),
    //       OK_STATE,
    //       "null"
    //     );
    //   }
    //   return;
    // }

    if (message.name == "window.getStatus") {
      const auto index = message.index;
      const auto windowIndexToShow = message.get("window").size() > 0 ? std::stoi(message.get("window")) : index;
      const auto window = windowManager.getWindow(index);
      const auto windowToShow = windowManager.getWindow(windowIndexToShow);
      if (windowToShow) {
        JSON::Object value = JSON::Object::Entries {
          {"index", index},
          {"status", windowToShow->status}
        };
        const auto seq = message.get("seq");
        window->resolvePromise(
          seq,
          OK_STATE,
          JSON::Object(value).str()
        );
      } else {
        window->resolvePromise(
          message.get("seq"),
          OK_STATE,
          "null"
        );
      }
      return;
    }

    if (message.name == "window.getSize") {
      const auto index = message.index;
      const auto windowIndexToShow = message.get("window").size() > 0 ? std::stoi(message.get("window")) : index;
      const auto window = windowManager.getWindow(index);
      const auto windowToShow = windowManager.getWindow(windowIndexToShow);
      if (window) {
        const auto size = windowToShow->getSize();
        JSON::Object value = JSON::Object::Entries {
          {"index", index},
          {"width", size.width},
          {"height", size.height}
        };
        const auto seq = message.get("seq");
        window->resolvePromise(
          seq,
          OK_STATE,
          JSON::Object(value).str()
        );
      } else {
        window->resolvePromise(
          message.get("seq"),
          OK_STATE,
          "null"
        );
      }
      return;
    }

    if (message.name == "window.getTitle") {
      const auto index = message.index;
      const auto windowIndexToShow = message.get("window").size() > 0 ? std::stoi(message.get("window")) : index;
      const auto window = windowManager.getWindow(index);
      const auto windowToShow = windowManager.getWindow(windowIndexToShow);
      if (window) {
        const auto title = windowToShow->getTitle();
        JSON::Object value = JSON::Object::Entries {
          {"index", index},
          {"title", title}
        };
        const auto seq = message.get("seq");
        window->resolvePromise(
          seq,
          OK_STATE,
          JSON::Object(value).str()
        );
      } else {
        window->resolvePromise(
          message.get("seq"),
          OK_STATE,
          "null"
        );
      }
      return;
    }

    if (message.name == "window.show") {
      auto windowIndexToShow = std::stoi(message.get("window"));
      windowIndexToShow = windowIndexToShow < 0 ? 0 : windowIndexToShow;
      auto index = message.index < 0 ? 0 : message.index;
      auto options = WindowOptions {};
      auto status = windowManager.getWindowStatus(windowIndexToShow);
      auto window = windowManager.getWindow(windowIndexToShow);

      options.title = message.get("title");
      options.url = message.get("url");

      if (message.get("port").size() > 0) {
        options.port = std::stoi(message.get("port"));
      }

      if (message.get("width").size() > 0 && message.get("height").size() > 0) {
        options.width = std::stoi(message.get("width"));
        options.height = std::stoi(message.get("height"));
      }

      const auto seq = message.get("seq");
      if (!window || status == WindowManager::WindowStatus::WINDOW_NONE) {
        options.resizable = message.get("resizable") == "true" ? true : false;
        options.frameless = message.get("frameless") == "true" ? true : false;
        options.utility = message.get("utility") == "true" ? true : false;
        options.debug = message.get("debug") == "true" ? true : false;
        options.index = windowIndexToShow;

        window = windowManager.createWindow(options);
        window->show(EMPTY_SEQ);
      } else {
        window->show(EMPTY_SEQ);
      }

      if (window) {
        if (options.width > 0 && options.height > 0) {
          window->setSize(EMPTY_SEQ, options.width, options.height, 0);
        }

        if (options.title.size() > 0) {
          window->setTitle(EMPTY_SEQ, options.title);
        }

        if (options.url.size() > 0) {
          navigate(window, cwd, EMPTY_SEQ, decodeURIComponent(options.url));
        }
      }

      auto resolveWindow = windowManager.getWindow(index);
      if (resolveWindow) {
        resolveWindow->resolvePromise(seq, OK_STATE, std::to_string(index));
      }
      return;
    }

    if (message.name == "window.hide") {
      auto windowIndexToShow = std::stoi(message.get("window"));
      windowIndexToShow = windowIndexToShow < 0 ? 0 : windowIndexToShow;
      auto index = message.index < 0 ? 0 : message.index;
      auto window = windowManager.getWindow(windowIndexToShow);
      window->hide(EMPTY_SEQ);

      auto resolveWindow = windowManager.getWindow(index);
      if (resolveWindow) {
        resolveWindow->resolvePromise(message.get("seq"), OK_STATE, std::to_string(index));
      }
      return;
    }

    if (message.name == "window.navigate") {
      auto windowIndexToShow = std::stoi(message.get("window"));
      windowIndexToShow = windowIndexToShow < 0 ? 0 : windowIndexToShow;
      auto index = message.index < 0 ? 0 : message.index;
      auto window = windowManager.getWindow(windowIndexToShow);
      auto url = message.get("url");
      navigate(window, cwd, EMPTY_SEQ, decodeURIComponent(url));

      auto resolveWindow = windowManager.getWindow(index);
      if (resolveWindow) {
        resolveWindow->resolvePromise(message.get("seq"), OK_STATE, std::to_string(index));
      }
      return;
    }

    if (message.name == "inspect") {
      window->showInspector();
      return;
    }

    if (message.name == "window.setBackground") {
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

      const auto currentIndex = message.index;
      const auto index = message.get("window").size() > 0 ? std::stoi(message.get("window")) : currentIndex;
      const auto window = windowManager.getWindow(index);

      window->setBackgroundColor(red, green, blue, alpha);
      return;
    }

    if (message.name == "window.setSize") {
      int width = std::stoi(message.get("width"));
      int height = std::stoi(message.get("height"));

      const auto currentIndex = message.index;
      const auto index = message.get("window").size() > 0 ? std::stoi(message.get("window")) : currentIndex;
      const auto window = windowManager.getWindow(index);
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
        window->resolvePromise(seq, OK_STATE, "null");
        return;
      }

      window->setSystemMenuItemEnabled(enabled, indexMain, indexSub);
      window->resolvePromise(seq, OK_STATE, "null");
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

    //
    // No need to send resolve message to the backend process
    //
    if (message.name == "resolve") return;

    // window->resolvePromise(message.get("seq"), ERROR_STATE, "null");
    //
    // Everything else can be forwarded to the backend process.
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
  // we clean up the windows and the backend process.
  //
  shutdownHandler = [&](int code) {
    if (process != nullptr) {
      auto pid = process->getPID();
      process->kill(pid);
    }
    windowManager.destroy();
    app.kill();

    exit(code);
  };

  app.onExit = shutdownHandler;

  windowManager.configure(WindowManagerOptions {
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

  auto defaultWindow = windowManager.createDefaultWindow(WindowOptions { });

  // windowManager.getOrCreateWindow(0);
  windowManager.getOrCreateWindow(1);

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
