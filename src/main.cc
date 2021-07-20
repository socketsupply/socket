#include "common.hh"
#include "process.hh"

#if defined(_WIN32)
  #include "win.hh"
#elif defined(__APPLE__)
  #include "mac.hh"
#elif defined(__linux__)
  #include "linux.hh"
#endif

using namespace Opkit;

//
// the MAIN macro provides a cross-platform program entry point.
// it makes argc and argv uniformly available. It provides "instanceId"
// which on windows is hInstance, on mac and linux this is just an int.
//
MAIN {

  App app(instanceId);

  //
  // SETTINGS and DEBUG are compile time variables provided by the compiler.
  //
  constexpr auto _settings = STR_VALUE(SETTINGS);
  constexpr auto _debug = DEBUG;

  //
  // Prepare to forward commandline arguments to the main and render processes.
  // TODO (@heapwolf): make this url encoded, this way is a little weird.
  //
  auto cwd = app.getCwd(argv[0]);
  appData = parseConfig(decodeURIComponent(_settings));

  #if DEBUG == 1
    appData["name"] += "-dev";
    appData["title"] += "-dev";
  #endif

  std::stringstream argvArray;
  std::stringstream argvForward;

  argvForward << " --version=" << appData["version"];
  argvForward << " --name=" << appData["name"];

  #if DEBUG == 1
    argvForward << " --debug=1";
  #endif

  int c = 0;

  // TODO right now we forward a json parsable string as the args but this
  // isn't the most robust way of doing this. possible a URI-encoded query
  // string would be more in-line with how everything else works.
  for (auto const arg : std::span(argv, argc)) {
    argvArray
      << "'"
      << replace(std::string(arg), "'", "\'")
      << (c++ < argc ? "', " : "'");

    if (c > 1) argvForward << " " << std::string(arg);
  }

  std::stringstream env;
  for (auto const &envKey : split(appData["env"], ',')) {
    auto cleanKey = trim(envKey);
    auto envValue = getEnv(cleanKey.c_str());

    env << std::string(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  //
  // # Windows
  //
  // The Window constructor takes the app instance as well as some static
  // variables used during setup, these options can all be overridden later.
  //
  Window w0(app, WindowOptions {
    .resizable = true,
    .frameless = false,
    .height = std::stoi(appData["height"]),
    .width = std::stoi(appData["width"]),
    .index = 0,
    .debug = _debug,
    .forwardConsole = appData["forwardConsole"] == "true",
    .executable = appData["executable"],
    .title = appData["title"],
    .version = appData["version"],
    .argv = argvArray.str(),
    .env = env.str()
  });

  //
  // The second window is used for showing previews or progress, so it can
  // be frameless and prevent resizing, etc. it gets the same preload so
  // that we can communicate with it from the main process.
  //
  Window w1(app, WindowOptions {
    .resizable = false,
    .frameless = true,
    .height = 120,
    .width = 350,
    .index = 1,
    .debug = _debug,
    .argv = argvArray.str()
  });

  //
  // # Main -> Render
  // Launch the main process and connect callbacks to the stdio and stderr pipes.
  //
  auto onStdOut = [&](auto out) {
    //
    // ## Dispatch
    // Messages from the main process may be sent to the render process. If they
    // are parsable commands, try to do something with them, otherwise they are
    // just stdout and we can write the data to the pipe.
    //
    app.dispatch([&, out] {
      Parse cmd(out);

      auto &w = cmd.index == 0 ? w0 : w1;
      auto seq = cmd.get("seq");
      auto value = cmd.get("value");

      if (cmd.name == "title") {
        w.setTitle(seq, decodeURIComponent(value));
        return;
      }

      if (cmd.name == "show") {
        w.show(seq);
        return;
      }

      if (cmd.name == "hide") {
        w.hide(seq);
        return;
      }

      if (cmd.name == "navigate") {
        w.navigate(seq, decodeURIComponent(value));
        return;
      }

      if (cmd.name == "size") {
        int width = std::stoi(cmd.get("width"));
        int height = std::stoi(cmd.get("height"));
        w.setSize(seq, width, height, 0);
        return;
      }

      if (cmd.name == "getScreenSize") {
        auto size = app.getScreenSize();

        std::string value(
          "{"
            "\"width\":" + std::to_string(size.width) + ","
            "\"height\":" + std::to_string(size.height) + ""
          "}"
        );

        w.resolveToMainProcess(seq, "0", encodeURIComponent(value));
        return;
      }

      if (cmd.name == "menu") {
        w.setSystemMenu(seq, decodeURIComponent(value));
        return;
      }

      if (cmd.name == "external") {
        w.openExternal(decodeURIComponent(value));
        if (seq.size() > 0) {
          w.resolveToMainProcess(seq, "0", "null");
        }
        return;
      }

      if (cmd.name == "exit") {
        w.exit();
        if (seq.size() > 0) {
          w.resolveToMainProcess(seq, "0", "null");
        }
        return;
      }

      if (cmd.name == "resolve") {
        w.resolveToRenderProcess(seq, cmd.get("state"), value);
        return;
      }

      if (cmd.name == "send") {
        w.emitToRenderProcess(
          decodeURIComponent(cmd.get("event")),
          value
        );
        return;
      }

      if (cmd.name == "stdout") {
        std::cout << decodeURIComponent(value) << std::endl;
      }
    });
  };

  auto onStdErr = [&](auto err) {
    std::cerr << err << std::endl;
  };

  Process process(
    appData["cmd"] + argvForward.str(),
    cwd,
    onStdOut,
    onStdErr
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

    auto &w = cmd.index == 0 ? w0 : w1;

    if (cmd.name == "title") {
      w.setTitle(
        cmd.get("seq"),
        decodeURIComponent(cmd.get("value"))
      );
      return;
    }

    if (cmd.name == "exit") {
      w.exit();
      return;
    }

    if (cmd.name == "external") {
      w.openExternal(decodeURIComponent(cmd.get("value")));
      return;
    }

    if (cmd.name == "dialog") {
      bool isSave = cmd.get("type").compare("save") == 0;
      bool allowDirs = cmd.get("allowDirs").compare("true") == 0;
      bool allowFiles = cmd.get("allowFiles").compare("true") == 0;
      bool allowMultiple = cmd.get("allowMultiple").compare("true") == 0;
      std::string defaultPath = cmd.get("defaultPath");
      std::string title = cmd.get("title");

      w.openDialog(
        cmd.get("seq"),
        isSave,
        allowDirs,
        allowFiles,
        allowMultiple,
        defaultPath,
        title
      );

      return;
    }

    if (cmd.name == "context") {
      w.setContextMenu(
        cmd.get("seq"),
        decodeURIComponent(cmd.get("value"))
      );

      return;
    }

    //
    // Everything else can be forwarded to the main process.
    // The protocol requires messages must be terminated by a newline.
    //
    process.write(out);
  };

  w0.onMessage = onMessage;
  w1.onMessage = onMessage;

  //
  // # Exiting
  // When a window or the app wants to exit,
  // we clean up the windows and the main process.
  // TODO pass a real exit code?
  //
  static auto onExit = [&] {
    w0.kill();
    w1.kill();
    process.kill(process.getPID());
    app.kill();
  };

  app.onExit = onExit;
  w0.onExit = onExit;
  w1.onExit = onExit;

  signal(SIGINT, +[](int signum) {
    onExit();
    exit(signum);
  });

  //
  // # Event Loop
  // start the platform specific event loop for the main
  // thread and run it until it returns a non-zero int.
  //
  while(app.run() == 0);
}
