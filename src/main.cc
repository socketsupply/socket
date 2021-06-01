#include "webview.h"
#include "process.h"
#include "preload.h"

constexpr auto _settings = SETTINGS;
constexpr auto _debug = DEBUG;

#define STR(x) #x

#ifdef _WIN32
#include <direct.h>

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
#else
#include <unistd.h>

int main(int argc, char *argv[])
#endif
{
  static auto win = std::make_unique<Opkit::webview>(true, nullptr);

  auto cwd = getCwd(argv[0]);
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

  std::stringstream argvs;
  int c = 0;

  for (auto const arg : std::span(argv, argc)) {
    argvs
      << "'"
      << replace(std::string(arg), "'", "\'")
      << (c++ < argc ? "', " : "'");
  }

  win->init(
    "(() => {"
    "  window.main = window.main || {};\n"
    "  window.main.name = '" + settings["name"] + "';\n"
    "  window.main.version = '" + settings["version"] + "';\n"
    "  window.main.debug = " + std::to_string(_debug) + ";\n"
    "  window.main.argv = [" + argvs.str() + "];\n"
    "  " + gPreload + "\n"
    "})()"
    "//# sourceURL=preload.js"
  );

  Opkit::Process process(
    settings["cmd"] + " " + argvs.str(),
    cwd,
    [&](Opkit::Process::string_type stdout) {
      while (!isDocumentReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }

      if (stdout.find("binding;") == 0) {
        win->binding(replace(stdout, "binding;", ""));
      } else if (stdout.find("stdout;") == 0) {
        std::cout << stdout.substr(7) << std::endl;
      } else if (stdout.find("ipc;") == 0) {
        win->resolve(stdout);
      } else {
        win->emit("data", stdout);
      }
    },
    [](Opkit::Process::string_type stderr) {
      std::cerr << stderr << std::endl;
    }
  );

  win->ipc("quit", [&](auto seq, auto value) {
    Opkit::Process::kill(process.getPID());
    win->terminate();
  });

  win->ipc("ready", [&](auto seq, auto value) {
    isDocumentReady = true;
  });

  win->ipc("dialog", [&](auto seq, auto value) {
    win->dialog(seq);
  });

  win->ipc("setMenu", [&](auto seq, auto value) {
    win->menu(value);

    if (std::stoi(seq) > 0) {
      win->resolve("ipc;0;" + seq + ";" + value);
    }
  });

  win->ipc("setTitle", [&](auto seq, auto value) {
    win->setTitle(value);

    if (std::stoi(seq) > 0) {
      win->resolve("ipc;0;" + seq + ";" + trim(value));
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
      win->resolve("ipc;0;" + seq + ";" + value);
    }
  });

  win->ipc("contextMenu", [&](std::string seq, std::string value) {
    win->createContextMenu(seq, value);
  });

  win->ipc("openExternal", [&](std::string seq, std::string value) {
    auto r = std::to_string(win->openExternal(value));
    win->resolve("ipc;" + r + ";" + seq + ";" + value);
  });

  win->ipc("send", [&](std::string seq, std::string value) {
    process.write("ipc;0;" + seq + ";" + value);
  });

  win->ipc("inspect", [&](std::string seq, std::string value) {
    win->inspect();
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->ipc("hide", [&](std::string seq, std::string value) {
    win->hide();
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->ipc("show", [&](std::string seq, std::string value) {
    win->show();
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->run();

  return 0;
}
