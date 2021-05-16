#include "webview.h"
#include "process.h"
#include "platform.h"

#include <iostream>

constexpr auto title = WIN_TITLE;
constexpr auto width = WIN_WIDTH;
constexpr auto height = WIN_HEIGHT;
constexpr auto menu = MENU;
constexpr auto cmd = CMD;
constexpr auto arg = ARG;

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
  static auto process = std::make_unique<Process>();
  static auto win = std::make_unique<webview::webview>(true, nullptr);

  win->set_title(title);

  win->set_size(
    std::stoi(width),
    std::stoi(height),
    WEBVIEW_HINT_NONE
  );

  win->menu(menu);

  win->ipc("dialog", [&](auto seq, auto value) {
    win->dialog(seq);
  });

  win->ipc("contextMenu", [&](std::string seq, std::string value) {
    createContextMenu(seq, value);
  });

  win->ipc("send", [&](std::string seq, std::string value) {
    process->write("ipc;0;" + seq + ";" + value);
  });

  process->onError = [] (const std::string msg) {
    std::cerr << msg << std::endl;
  };

  process->onData = [&] (const std::string msg) {
    if (msg.find("stdout;") != std::string::npos) {
      std::cout << msg.substr(7) << std::endl;
    } else if (msg.find("ipc;") != std::string::npos) {
      win->resolve(msg);
    } else {
      win->emit("data", msg);
    }
  };
 
  std::string cwd = getCwd(argv[0]);

  win->navigate("file://" + cwd + "/index.html");

  std::thread main([&]() {
    process->spawn(cmd, arg, cwd.c_str());
  });

  win->run();
  main.join();

  return 0;
}
