#include "webview.h"
#include "process.h"
#include "preload.h"

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = DEBUG;

#ifdef _WIN32
int CALLBACK WinMain(
  _In_ HINSTANCE hInstance,
  _In_ HINSTANCE hPrevInstance,
  _In_ LPSTR lpCmdLine,
  _In_ int nCmdShow) {

  static auto win = std::make_unique<Opkit::webview>(true, hInstance);

  int argc = __argc;
  char** argv = __argv;

  win->createWindow();

#else
int main(int argc, char *argv[]) {

  static auto win = std::make_unique<Opkit::webview>(true, nullptr);

#endif

  auto cwd = Str(getCwd(argv[0]));
  bool isDocumentReady = false;
  auto settings = parseConfig(replace(_settings, "%%", "\n"));
  Opkit::appData = settings;

  if (platform.darwin) {
    // On MacOS, this will hide the window initially
    // but also allow us to load the url, like a preload.
    win->setSize(
      0,
      0,
      WEBVIEW_HINT_NONE
    );
  }

  win->navigate("file://" + cwd + "/index.html"); 

  std::stringstream argvArray;
  std::stringstream argvForward;

  argvForward << " --version=" << settings["version"];

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

  win->init(
    "(() => {"
    "  window.system = {};\n"
    "  window.process = {};\n"
    "  window.process.title = '" + settings["title"] + "';\n"
    "  window.process.executable = '" + settings["executable"] + "';\n"
    "  window.process.version = '" + settings["version"] + "';\n"
    "  window.process.debug = " + std::to_string(_debug) + ";\n"
    "  window.process.bundle = '" + cwd + "';\n"
    "  window.process.argv = [" + argvArray.str() + "];\n"
    "  " + gPreload + "\n"
    "})()"
    "//# sourceURL=preload.js"
  );

  Opkit::Process process(
    settings["cmd"] + argvForward.str(),
    cwd,
    [&](auto out) {
      writeLog(out);
      while (!isDocumentReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }

      if (out.find("binding;") == 0) {
        win->binding(replace(out, "binding;", ""));
      } else if (out.find("out;") == 0) {
        std::cout << out.substr(7) << std::endl;
      } else if (out.find("ipc;") == 0) {
        win->resolve(out);
      } else {
        win->emit("data", out);
      }
    },
    [](auto err) {
      writeLog(err);
      std::cerr << err << std::endl;
    }
  );

  win->ipc("exit", [&](auto seq, auto value) {
    Opkit::Process::kill(process.getPID());
    win->terminate();
    exit(std::stoi(value));
  });

  win->ipc("ready", [&](auto seq, auto value) {
    isDocumentReady = true;
    process.write("ipc;0;0;" + value);
  });

  win->ipc("dialog", [&](auto seq, auto value) {
    win->dialog(seq);
  });

  win->ipc("setMenu", [&](auto seq, auto value) {
    win->menu(value);

    if (std::stoi(seq) > 0) {
      win->resolve("ipc;0;" + seq + ";null");
    }
  });

  win->ipc("setTitle", [&](auto seq, auto value) {
    win->setTitle(value);

    if (std::stoi(seq) > 0) {
      win->resolve("ipc;0;" + seq + ";null");
    }
  });

  win->ipc("setSize", [&](auto seq, auto value) {
    auto parts = split(value, ';');

    win->setSize(
      std::stoi(parts[0].c_str()),
      std::stoi(parts[1].c_str()),
      WEBVIEW_HINT_NONE
    );

    if (std::stoi(seq) > 0) {
      win->resolve("ipc;0;" + seq + ";null");
    }
  });

  win->ipc("contextMenu", [&](auto seq, auto value) {
    win->createContextMenu(seq, value);
  });

  win->ipc("openExternal", [&](auto seq, auto value) {
    auto r = std::to_string(win->openExternal(value));

    if (std::stoi(seq) > 0) {
      win->resolve("ipc;" + r + ";" + seq + ";" + value);
    }
  });

  win->ipc("send", [&](auto seq, auto value) {
    process.write("ipc;0;" + seq + ";" + value);
  });

  win->ipc("inspect", [&](auto seq, auto value) {
    win->inspect();
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->ipc("hide", [&](auto seq, auto value) {
    win->hide();
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->ipc("show", [&](auto seq, auto value) {
    win->show();
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->run();

  return 0;
}
