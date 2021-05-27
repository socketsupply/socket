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
  auto settings = parseConfig(std::regex_replace(_settings, std::regex("%%"), "\n"));

  Opkit::Process process(
    settings["cmd"],
    cwd,
    [](Opkit::Process::string_type stdout) {
      if (stdout.find("stdout;") != -1) {
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

  Opkit::appData = settings;

  win->setTitle(settings["title"]);

  std::cout << settings["width"] << std::endl;

  win->setSize(
    std::stoi(settings["width"].c_str()),
    std::stoi(settings["height"].c_str()),
    WEBVIEW_HINT_NONE
  );

  win->menu(_menu);

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
