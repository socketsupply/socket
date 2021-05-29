#include "webview.h"
#include "process.h"
#include "util.h"

constexpr auto _settings = SETTINGS;
constexpr auto _menu = MENU;

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

  auto settings = parseConfig(replace(_settings, "%%", "\n"));
  Opkit::appData = settings;

  Opkit::Process process(
    settings["cmd"],
    cwd,
    [&](Opkit::Process::string_type stdout) {
      if (stdout.find("binding;") != -1) {
        win->binding(replace(stdout, "binding;", ""));
      } else if (stdout.find("stdout;") != -1) {
        std::cout << stdout.substr(7) << std::endl;
      } else if (stdout.find("ipc;") != -1) {
        win->resolve(stdout);
      } else {
        win->emit("data", stdout);
      }
    },
    [](Opkit::Process::string_type stderr) {
      std::cerr << stderr << std::endl;
    }
  );

  win->menu(_menu);

  win->setSize(
    0,
    0,
    WEBVIEW_HINT_NONE
  );

  win->ipc("quit", [&](auto seq, auto value) {
    Opkit::Process::kill(process.getPID());
  });

  win->ipc("dialog", [&](auto seq, auto value) {
    win->dialog(seq);
  });

  win->ipc("menu", [&](auto seq, auto value) {
    win->menu(value);
    win->resolve("ipc;0;" + seq + ";null");
  });

  win->ipc("setTitle", [&](auto seq, auto value) {
    win->setTitle(value);
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->ipc("setSize", [&](auto seq, auto value) {
    auto parts = split(value, 'x');

    win->setSize(
      std::stoi(parts[0].c_str()),
      std::stoi(parts[1].c_str()),
      WEBVIEW_HINT_NONE
    );

    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->ipc("contextMenu", [&](std::string seq, std::string value) {
    win->createContextMenu(seq, value);
  });

  win->ipc("send", [&](std::string seq, std::string value) {
    process.write("ipc;0;" + seq + ";" + value);
  });

  win->ipc("hide", [&](std::string seq, std::string value) {
    win->hide();
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->ipc("show", [&](std::string seq, std::string value) {
    win->show();
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->navigate("file://" + cwd + "/index.html"); 
  win->run();

  return 0;
}
