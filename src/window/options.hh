#ifndef SSC_WINDOW_OPTIONS_H
#define SSC_WINDOW_OPTIONS_H

#include "../common.hh"

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
    String cwd = "";
    String executable = "";
    String title = "";
    String url = "data:text/html,<html>";
    String version = "";
    String argv = "";
    String preload = "";
    String env;
    Map appData;
    MessageCallback onMessage = [](const String) {};
    ExitCallback onExit = nullptr;
  };
}
#endif
