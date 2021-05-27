#include "webview.h"
#include "process.h"
#include "platform.h"

#include <iostream>

constexpr auto settings = SETTINGS;
constexpr auto menu = MENU;
// constexpr auto title = WIN_TITLE;
// constexpr auto width = WIN_WIDTH;
// constexpr auto height = WIN_HEIGHT;
// constexpr auto cmd = CMD;
// constexpr auto arg = ARG;

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

  std::string cwd = getCwd(argv[0]);

  Opkit::Process process(
    cmd,
    cwd,
    [](Opkit::Process::string_type stdout) {
      if (stdout.find("stdout;") != std::string::npos) {
        std::cout << stdout.substr(7) << std::endl;
      } else if (stdout.find("ipc;") != std::string::npos) {
        win->resolve(stdout);
      } else {
        win->emit("data", stdout);
      }
    },
    [](Opkit::Process::string_type stderr) {
      std::cerr << stderr << std::endl;
    }
  );

  win->setTitle(title);

  win->setSize(
    std::stoi(width),
    std::stoi(height),
    WEBVIEW_HINT_NONE
  );

  win->menu(menu);

  win->ipc("dialog", [&](auto seq, auto value) {
    win->dialog(seq);
  });

  win->ipc("setTitle", [&](auto seq, auto value) {
    win->setTitle(value);
    win->resolve("ipc;0;" + seq + ";" + value);
  });

  win->ipc("contextMenu", [&](std::string seq, std::string value) {
    win->createContextMenu(seq, value);
  });

  win->ipc("send", [&](std::string seq, std::string value) {
    process.write("ipc;0;" + seq + ";" + value);
  });

  win->ipc("quit", [&](auto seq, auto value) {
    Opkit::Process::kill(process.getPID());
    win->resolve("ipc;0;" + seq + ";" + value);
    win->terminate();
  });
 
  win->navigate("file://" + cwd + "/index.html");
  win->run();

  return 0;
}
