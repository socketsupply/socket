#ifndef SSC_WINDOW_OPTIONS_H
#define SSC_WINDOW_OPTIONS_H

#include "../common.hh"

namespace SSC {
  struct WindowOptions {
    bool resizable = true;
    bool frameless = false;
    bool utility = false;
    bool canExit = false;
    float height = 0;
    float width = 0;
    bool isHeightInPercent = false;
    bool isWidthInPercent = false;
    int index = 0;
    int debug = 0;
    int port = 0;
    bool isTest = false;
    bool headless = false;
    bool forwardConsole = false;
    String cwd = "";
    String title = "";
    String url = "data:text/html,<html>";
    String argv = "";
    String preload = "";
    String env;
    Map appData;
    MessageCallback onMessage = [](const String) {};
    ExitCallback onExit = nullptr;
  };
}
#endif
