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

MAIN /* argv, argc, hInstance */ {

  #ifdef _WIN32
    App app(hInstance);
  #else
    App app;
  #endif

  constexpr auto _settings = STR_VALUE(SETTINGS);
  constexpr auto _debug = DEBUG;

  auto cwd = app.getCwd(argv[0]);
  appData = parseConfig(replace(_settings, "%%", "\n"));

  std::stringstream argvArray;
  std::stringstream argvForward;

  argvForward << " --version=" << appData["version"];

  #if DEBUG == 1
    argvForward << " --debug=1";
  #endif

  int c = 0;

  for (auto const arg : std::span(argv, argc)) {
    argvArray
      << "'"
      << replace(std::string(arg), "'", "\'")
      << (c++ < argc ? "', " : "'");

    if (c > 1) argvForward << " " << std::string(arg);
  }

  auto makePreload = [&](int index) {
    std::string preload(
      "(() => {"
      "  window.system = {};\n"
      "  window.process = {};\n"
      "  window.process.index = Number('" + std::to_string(index) + "');\n"
      "  window.process.title = '" + appData["title"] + "';\n"
      "  window.process.executable = '" + appData["executable"] + "';\n"
      "  window.process.version = '" + appData["version"] + "';\n"
      "  window.process.debug = " + std::to_string(_debug) + ";\n"
      "  window.process.bundle = '" + replace(cwd, "\\", "\\\\") + "';\n"
      "  window.process.argv = [" + argvArray.str() + "];\n"
      "  " + gPreload + "\n"
      "})()\n"
      "//# sourceURL=preload.js"
    );

    return preload;
  };

  Window w0(app, WindowOptions {
    .resizable = true,
    .frameless = false,
    .height = std::stoi(appData["height"]),
    .width = std::stoi(appData["width"]),
    .title = appData["title"],
    .preload = makePreload(0)
  });

  Window w1(app, WindowOptions {
    .resizable = false,
    .frameless = true,
    .height = 120,
    .width = 350,
    .preload = makePreload(1)
  });

  Process process(
    appData["cmd"] + argvForward.str(),
    cwd,
    [&](auto out) {
      //
      // Sends messages from the main process to the render process. If they
      // are "commands" try to do something with them, otherwise they are just
      // stdout and we can write the data to the pipe.
      //
      // - The dispatch function on mac sends the function to the main
      // event queue.
      //
      // - On Windows(TM), dispatch listens from the thread on which the
      // WebView was created (WebView2 is STA).
      //
      // - On linux dispatch is to the main thread.
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
    },
    [&](auto err) {
      std::cerr << err << std::endl;
    }
  );

  //
  // Send messages from the render processes to the main process.
  // These may be similar to how we route the messages from the
  // main process but different enough that duplication is ok.
  //
  auto router = [&](auto out) {
    return;
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

  w0.onMessage(router);
  w1.onMessage(router);

  while(app.run() == 0);

  // TODO on exit, kill main.
}