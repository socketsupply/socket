#include "webview.h"
#include "process.h"
#include "platform.h"

#include <iostream>

#define WIN_TITLE TO_STR(O_WIN_TITLE)
#define WIN_WIDTH TO_STR(O_WIN_WIDTH)
#define WIN_HEIGHT TO_STR(O_WIN_HEIGHT)
#define CMD TO_STR(O_CMD)
#define ARG TO_STR(O_ARG)
#define MENU TO_STR(O_MENU)

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

  win->set_title(WIN_TITLE);

  win->set_size(
    std::stoi(WIN_WIDTH),
    std::stoi(WIN_HEIGHT),
    WEBVIEW_HINT_NONE
  );

  createMenu(MENU);

  win->ipc("dialog", [&](auto seq, auto value) {
    win->dialog(seq);
  });

  win->ipc("invokeIPC", [&](std::string seq, std::string value) {
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
 
  std::string cwd = getCwd();

  win->navigate("file://" + cwd + "/index.html");

  std::thread main([&]() {
    process->spawn(CMD, ARG, cwd.c_str());
  });

  win->run();
  main.join();

  return 0;
}
