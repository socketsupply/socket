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
  //
  auto cwd = app.getCwd(argv[0]);
  appData = parseConfig(replace(_settings, "%%", "\n"));

  std::stringstream argvArray;
  std::stringstream argvForward;

  argvForward << " --version=" << appData["version"];

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
    .title = appData["title"],
    .preload = PreloadOptions {
      .index = 0,
      .debug = _debug,
      .title = appData["title"],
      .executable = appData["executable"],
      .version = appData["version"],
      .argv = argvArray.str()
    }.toString()
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
    .preload = PreloadOptions {
      .index = 1,
      .debug = _debug,
      .title = appData["title"],
      .executable = appData["executable"],
      .version = appData["version"],
      .argv = argvArray.str()
    }.toString()
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

      if (cmd.name == "title") {
        auto s = decodeURIComponent(cmd.args["value"]);
        w.setTitle(s);
        return;
      }

      if (cmd.name == "show") {
        w.show();
        return;
      }

      if (cmd.name == "hide") {
        w.hide();
        return;
      }

      if (cmd.name == "navigate") {
        auto s = decodeURIComponent(cmd.args["url"]);
        w.navigate(s);
        return;
      }

      if (cmd.name == "menu") {
        auto s = decodeURIComponent(cmd.args["value"]);
        w.setSystemMenu(s);
        return;
      }

      if (cmd.name == "respond") {
        auto seq = cmd.args["seq"];
        auto status = cmd.args["status"];
        auto value = cmd.args["value"];

        w.eval(w.resolve(seq, status, value));
        return;
      }

      if (cmd.name == "send") {
        auto event = decodeURIComponent(cmd.args["event"]);
        auto value = cmd.args["value"];

        w.eval(w.emit(event, value));
        return;
      }

      if (cmd.name == "stdout") {
        std::string value = decodeURIComponent(cmd.args["value"]);
        std::cout << value << std::endl;
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
      auto s = decodeURIComponent(cmd.args["value"]);
      w.setTitle(s);
      return;
    }

    if (cmd.name == "external") {
      w.openExternal(decodeURIComponent(cmd.args["value"]));
      return;
    }

    if (cmd.name == "dialog") {
      bool isSave = false;
      bool allowDirs = false;
      bool allowFiles = false;
      std::string defaultPath = "";
      std::string title = "";

      if (cmd.args.count("type")) {
        isSave = cmd.args["type"].compare("save") == 1;
      }

      if (cmd.args.count("allowDirectories")) {
        allowDirs = cmd.args["allowDirectories"].compare("true") == 1;
      }

      if (cmd.args.count("allowFiles")) {
        allowDirs = cmd.args["allowFiles"].compare("true") == 1;
      }

      if (cmd.args.count("defaultPath")) {
        defaultPath = cmd.args["defaultPath"];
      }

      if (cmd.args.count("title")) {
        title = cmd.args["title"];
      }

      auto result = w.openDialog(
        isSave,
        allowDirs,
        allowFiles,
        defaultPath,
        title
      );

      w.eval(w.resolve(cmd.args["seq"], "0", result));
      return;
    }

    if (cmd.name == "context") {
      auto seq = cmd.args["seq"];
      auto value = decodeURIComponent(cmd.args["value"]);

      // send the seq, use it to resolve the promise
      w.setContextMenu(seq, value);

      return;
    }

    process.write(out);
  };

  w0.onMessage = onMessage;
  w1.onMessage = onMessage;

  //
  // # Exinting
  // When a window or the app wants to exit,
  // we clean up the windows and the main process.
  // TODO pass a real exit code?
  //
  auto onExit = [&] {
    w0.kill();
    w1.kill();
    process.kill(process.getPID());
    app.kill();
  };

  app.onExit = onExit;
  w0.onExit = onExit;
  w1.onExit = onExit;

  //
  // # Event Loop
  // start the platform specific event loop for the main
  // thread and run it until it returns a non-zero int.
  //
  while(app.run() == 0);
}