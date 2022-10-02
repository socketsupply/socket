#ifndef SSC_WINDOW_OPTIONS_H
#define SSC_WINDOW_OPTIONS_H

#include "../core/common.hh"

namespace SSC {
  struct WindowOptions {
    bool resizable = true;
    bool frameless = false;
    bool utility = false;
    bool canExit = false;
    int height = 0;
    int width = 0;
    int index = 0;
    int debug = 0;
    int port = 0;
    bool isTest = false;
    bool headless = false;
    bool forwardConsole = false;
    std::string cwd = "";
    std::string executable = "";
    std::string title = "";
    std::string url = "data:text/html,<html>";
    std::string version = "";
    std::string argv = "";
    std::string preload = "";
    std::string env;
    Map appData;
    MessageCallback onMessage = [](const std::string) {};
    ExitCallback onExit = nullptr;
  };
}
#endif
