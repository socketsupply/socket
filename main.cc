#include "webview.h"
#include "process.h"

#include <iostream>
#include <sstream>
#include <filesystem>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
#else
#include <unistd.h>

static auto process = std::make_unique<Process>();

int main(int argc, char *argv[])
#endif
{
  static auto win = std::make_unique<webview::webview>(true, nullptr);
  win->set_title("Operator");
  win->set_size(750, 520, WEBVIEW_HINT_NONE);

  createMenu();

  win->ipc("log", [](auto seq, auto s) {
    std::cout << s << std::endl;
    win->resolve("ipc;0;" + seq + ";");
  });

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
    if (msg.find("ipc;") != std::string::npos) {
      win->resolve(msg);
    } else {
      win->emit("data", msg);
    }
  };
 
  std::string cwd = getCwd();

  std::stringstream ss;
  ss << "file://";
  ss << cwd;
  ss << "/render.html";

  win->navigate(ss.str());

  std::thread main([&]() {
    process->spawn("node", "main.js", cwd.c_str());
  });

  win->run();
  main.join();

  return 0;
}
