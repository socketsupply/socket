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

#define InvalidWindowIndexError(index) \
  SSC::String("Invalid index given for window: ") + std::to_string(index)

using namespace SSC;

std::function<void(int)> shutdownHandler;
void signalHandler (int signal) {
  if (shutdownHandler != nullptr) {
    shutdownHandler(signal);
  }
}

SSC::String getNavigationError (const String &cwd, const String &value) {
  if (!value.starts_with("file://")) {
    return SSC::String("only file:// protocol is allowed for the file navigation. Got url ") + value;
  }
  auto url = value.substr(7);
  if (url.empty()) {
    return SSC::String("empty url");
  }
  if (!url.starts_with(cwd)) {
    return SSC::String("only files in the current working directory and its subfolders are allowed. Got url ") + value;
  }
  if (!url.ends_with(".html")) {
    return SSC::String("only .html files are allowed. Got url ") + value;
  }
  if (url.find("/../") != std::string::npos) {
    return SSC::String("relative urls are not allowed. Got url ") + value;
  }
  if (!fs::exists(url)) {
    return SSC::String("file does not exist. Got url ") + value;
  }
  return SSC::String("");
}

//
// the MAIN macro provides a cross-platform program entry point.
// it makes argc and argv uniformly available. It provides "instanceId"
// which on windows is hInstance, on mac and linux this is just an int.
//
MAIN {
  // Singletons should be static to remove some possible race conditions in
  // their instantiation and destruction.
  static App app(instanceId);
  static WindowManager windowManager(app);

  // TODO(trevnorris): Since App is a singleton, follow the CppCoreGuidelines
  // better in how it's handled in the future.
  // For now make a pointer reference since there is some member variable name
  // collision in the call to shutdownHandler when it's being called from the
  // windowManager instance.
  static App* app_ptr = &app;

  app.setWindowManager(&windowManager);
  constexpr auto _port = PORT;

  const SSC::String OK_STATE = "0";
  const SSC::String ERROR_STATE = "1";
  const SSC::String EMPTY_SEQ = SSC::String("");

  auto cwd = app.getCwd();
  app.appData = SSC::getUserConfig();

  SSC::String suffix = "";

  SSC::StringStream argvArray;
  SSC::StringStream argvForward;

  bool isCommandMode = false;
  bool isReadingStdin = false;
  bool isHeadless = false;
  bool isTest = false;

  int exitCode = 0;
  int c = 0;

  bool wantsVersion = false;
  bool wantsHelp = false;

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
    }

    // launched from the `ssc` cli
    app.fromSSC = s.find("--from-ssc") == 0 ? true : false;

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
    app.appData["build_name"] += "-dev";
  }

  app.appData["build_name"] += suffix;

  argvForward << " --ssc-version=v" << SSC::VERSION_STRING;
  argvForward << " --version=v" << app.appData["meta_version"];
  argvForward << " --name=" << app.appData["build_name"];

  if (isDebugEnabled()) {
    argvForward << " --debug=1";
  }

  SSC::StringStream env;
  for (auto const &envKey : parseStringList(app.appData["build_env"])) {
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

  auto killProcess = [&](Process* processToKill) {
    if (processToKill != nullptr) {
      auto pid = processToKill->getPID();
      processToKill->kill(pid);
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

  if (isCommandMode) {
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
          stdWrite(decodeURIComponent(message.get("value")), false);
        }
      },
      [](SSC::String const &out) { stdWrite(out, true); },
      [](SSC::String const &code){ exit(std::stoi(code)); }
    );

    if (cmd.size() == 0) {
      stdWrite("No 'cmd' is provided for '" + platform.os + "' in socket.ini", true);
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
    signal(SIGTERM, signalHandler);

    return exitCode;
  }

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

      if (message.index > 0 && message.name.size() == 0) {
        // @TODO: print warning
        return;
      }

      if (message.index > SSC_MAX_WINDOWS) {
        // @TODO: print warning
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

      if (message.name == "resolve") {
        window->resolvePromise(seq, message.get("state"), encodeURIComponent(value));
        return;
      }

      if (message.name == "config") {
        auto key = message.get("key");
        window->resolvePromise(seq, OK_STATE, app.appData[key]);
        return;
      }

      if (message.name == "process.exit") {
        for (auto w : windowManager.windows) {
          if (w != nullptr) {
            auto window = windowManager.getWindow(w->opts.index);
            window->eval(getEmitToRenderProcessJavaScript("process.exit", value));
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
      auto force = message.get("force") == "true" ? true : false;
      if (cmd.size() > 0) {
        if (process == nullptr || force) {
          createProcess(force);
          process->open();
        }
#ifdef _WIN32
        size_t last_pos = 0;
        while ((last_pos = process->path.find('\\', last_pos)) != std::string::npos) {
          process->path.replace(last_pos, 1, "\\\\\\\\");
          last_pos += 4;
        }
#endif
        const JSON::Object json = JSON::Object::Entries {
          { "cmd", cmd },
          { "argv", process->argv },
          { "path", process->path }
        };
        window->resolvePromise(seq, OK_STATE, encodeURIComponent(json.str()));
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

      window->resolvePromise(seq, OK_STATE, "null");
      return;
    }

    if (message.name == "process.write") {
      auto seq = message.get("seq");
      if (cmd.size() > 0 && process != nullptr) {
        process->write(out);
      }
      window->resolvePromise(seq, OK_STATE, "null");
      return;
    }

    if (message.name == "window.send") {
      const auto event = message.get("event");
      const auto value = decodeURIComponent(message.get("value"));
      const auto targetWindowIndex = message.get("targetWindowIndex").size() >= 0 ? std::stoi(message.get("targetWindowIndex")) : -1;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);
      const auto currentWindow = windowManager.getWindow(message.index);
      if (targetWindow) {
        targetWindow->eval(getEmitToRenderProcessJavaScript(event, value));
      }
      const auto seq = message.get("seq");
      currentWindow->resolvePromise(seq, OK_STATE, "null");
      return;
    }

    if (message.name == "application.exit") {
      try {
        exitCode = std::stoi(decodeURIComponent(value));
      } catch (...) {
      }

    #if defined(__APPLE__)
      if (app.fromSSC) {
        debug("__EXIT_SIGNAL__=%d", exitCode);
        notifyCli();
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
        window->resolvePromise(seq, OK_STATE, json.str());
      }
      return;
    }

    if (message.name == "application.getWindows") {
      const auto index = message.index;
      const auto window = windowManager.getWindow(index);
      auto indices = SSC::splitToInts(value, ',');
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
      auto targetWindow = windowManager.getWindow(targetWindowIndex);
      auto targetWindowStatus = windowManager.getWindowStatus(targetWindowIndex);

      if (targetWindow && targetWindowStatus != WindowManager::WindowStatus::WINDOW_NONE) {
        const JSON::Object json = JSON::Object::Entries {
          { "err", "Window with index " + std::to_string(targetWindowIndex) + " already exists" }
        };
        currentWindow->resolvePromise(seq, ERROR_STATE, json.str());
        return;
      }

      SSC::String error = getNavigationError(cwd, decodeURIComponent(message.get("url")));
      if (error.size() > 0) {
        const JSON::Object json = SSC::JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", error}
          }}
        };
        currentWindow->resolvePromise(seq, ERROR_STATE, json.str());
        return;
      }

      // fill the options
      auto options = WindowOptions {};

      options.title = message.get("title");
      options.url = message.get("url");

      if (message.get("port").size() > 0) {
        options.port = std::stoi(message.get("port"));
      }

      auto screen = currentWindow->getScreenSize();
      
      options.width = message.get("width").size() ? currentWindow->getSizeInPixels(message.get("width"), screen.width) : 0;
      options.height = message.get("height").size() ? currentWindow->getSizeInPixels(message.get("height"), screen.height) : 0;

      options.minWidth = message.get("minWidth").size() ? currentWindow->getSizeInPixels(message.get("minWidth"), screen.width) : 0;
      options.minHeight = message.get("minHeight").size() ? currentWindow->getSizeInPixels(message.get("minHeight"), screen.height) : 0;
      options.maxWidth = message.get("maxWidth").size() ? currentWindow->getSizeInPixels(message.get("maxWidth"), screen.width) : screen.width;
      options.maxHeight = message.get("maxHeight").size() ? currentWindow->getSizeInPixels(message.get("maxHeight"), screen.height) : screen.height;

      options.resizable = message.get("resizable") == "true" ? true : false;
      options.frameless = message.get("frameless") == "true" ? true : false;
      options.utility = message.get("utility") == "true" ? true : false;
      options.debug = message.get("debug") == "true" ? true : false;
      options.index = targetWindowIndex;

      targetWindow = windowManager.createWindow(options);

      targetWindow->show(EMPTY_SEQ);

      if (options.url.size() > 0) {
        targetWindow->navigate(seq, options.url);
      }

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() },
      };

      currentWindow->resolvePromise(seq, OK_STATE, json.str());
      return;
    }

    if (message.name == "window.close") {
      const auto index = message.index;
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : index;
      const auto window = windowManager.getWindow(index);
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);
      auto targetWindowStatus = windowManager.getWindowStatus(targetWindowIndex);
      if (targetWindow) {
        targetWindow->close(0);
        JSON::Object json = JSON::Object::Entries {
          { "data", targetWindow->json()},
        };
        window->resolvePromise(message.seq, OK_STATE, json.str());
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
        resolveWindow->resolvePromise(message.get("seq"), ERROR_STATE, json.str());
        return;
      }

      targetWindow->show(EMPTY_SEQ);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() }
      };
      resolveWindow->resolvePromise(message.get("seq"), OK_STATE, json.str());
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
        resolveWindow->resolvePromise(message.get("seq"), ERROR_STATE, json.str());
        return;
      }

      targetWindow->hide(EMPTY_SEQ);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() }
      };
      resolveWindow->resolvePromise(message.get("seq"), OK_STATE, json.str());
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
      currentWindow->resolvePromise(seq, OK_STATE, json.str());
      return;
    }

    if (message.name == "window.navigate") {
      const auto seq = message.seq;
      const auto currentIndex = message.index;
      const auto currentWindow = windowManager.getWindow(currentIndex);
      const auto targetWindowIndex = message.get("targetWindowIndex").size() > 0 ? std::stoi(message.get("targetWindowIndex")) : currentIndex;
      const auto targetWindow = windowManager.getWindow(targetWindowIndex);
      const auto url = message.get("url");
      const auto error = getNavigationError(cwd, decodeURIComponent(url));

      if (error.size() > 0) {
        JSON::Object json = JSON::Object::Entries {
          { "err", error }
        };
        currentWindow->resolvePromise(seq, ERROR_STATE, json.str());
        return;
      }

      targetWindow->navigate(seq, url);

      JSON::Object json = JSON::Object::Entries {
        { "data", targetWindow->json() },
      };

      currentWindow->resolvePromise(seq, OK_STATE, json.str());
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
      
      currentWindow->resolvePromise(seq, OK_STATE, json.str());
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
      currentWindow->resolvePromise(seq, OK_STATE, json.str());
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

      currentWindow->resolvePromise(seq, OK_STATE, json.str());
      return;
    }

    if (message.name == "application.setSystemMenu") {
      const auto seq = message.get("seq");
      window->setSystemMenu(seq, decodeURIComponent(value));
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

    if (message.name == "window.setContextMenu") {
      auto seq = message.get("seq");
      window->setContextMenu(seq, decodeURIComponent(value));
      window->resolvePromise(seq, OK_STATE, "null");
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

    window->resolvePromise(
      message.get("seq"),
      ERROR_STATE,
      encodeURIComponent(JSON::Object(err).str())
    );
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
    app_ptr->kill();
    exit(code);
  };

  app.onExit = shutdownHandler;

  String defaultWidth = app.appData["window_width"].size() > 0 ? app.appData["window_width"] : "100%";
  String defaultHeight = app.appData["window_height"].size() > 0 ? app.appData["window_height"] : "100%";
  String defaultMinWidth = app.appData["window_min_width"].size() > 0 ? app.appData["window_min_width"] : "0";
  String defaultMinHeight = app.appData["window_min_height"].size() > 0 ? app.appData["window_min_height"] : "0";
  String defaultMaxWidth = app.appData["window_max_width"].size() > 0 ? app.appData["window_max_width"] : "100%";
  String defaultMaxHeight = app.appData["window_max_height"].size() > 0 ? app.appData["window_max_height"] : "100%";

  windowManager.configure(WindowManagerOptions {
    .defaultHeight =  defaultHeight,
    .defaultWidth = defaultWidth,
    .defaultMinWidth = defaultMinWidth,
    .defaultMinHeight = defaultMinHeight,
    .defaultMaxWidth = defaultMaxWidth,
    .defaultMaxHeight = defaultMaxHeight,
    .headless = isHeadless,
    .isTest = isTest,
    .argv = argvArray.str(),
    .cwd = cwd,
    .appData = app.appData,
    .onMessage = onMessage,
    .onExit = shutdownHandler
  });

  auto defaultWindow = windowManager.createDefaultWindow(WindowOptions {
    .resizable = app.appData["window_resizable"] == "false" ? false : true,
    .frameless = app.appData["window_frameless"] == "true" ? true : false,
    .utility = app.appData["window_utility"] == "true" ? true : false
  });

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
  signal(SIGTERM, signalHandler);

  if (isReadingStdin) {
    std::string value;
    std::thread t([&]() {
      do {
        if (value.size() == 0) {
          std::cin >> value;
        }

        if (value.size() > 0 && defaultWindow->bridge->router.isReady) {
          defaultWindow->eval(getEmitToRenderProcessJavaScript("process.stdin", value));
          value.clear();
        }
      } while (true);
    });

    std::cin >> value;
    t.detach();
  }

  //
  // # Event Loop
  // start the platform specific event loop for the main
  // thread and run it until it returns a non-zero int.
  //
  while(app.run() == 0);

  exit(exitCode);
}
