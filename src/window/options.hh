#ifndef SSC_WINDOW_OPTIONS_H
#define SSC_WINDOW_OPTIONS_H

#include "../core/types.hh"
#include "../core/config.hh"

namespace SSC {
  struct WindowOptions {
    bool resizable = true;
    bool minimizable = true;
    bool maximizable = true;
    bool closable = true;
    bool frameless = false;
    bool utility = false;
    bool canExit = false;
    float width = 0;
    float height = 0;
    float minWidth = 0;
    float minHeight = 0;
    float maxWidth = 0;
    float maxHeight = 0;
    float radius = 0.0;
    float margin = 0;
    int index = 0;
    int debug = 0;
    int port = 0;
    bool isTest = false;
    bool headless = false;
    String aspectRatio = "";
    String titleBarStyle = "";
    String trafficLightPosition = "";
    String cwd = "";
    String title = "";
    String url = "data:text/html,<html>";
    String argv = "";
    String preload = "";
    String env;
    Map userConfig = getUserConfig();
    MessageCallback onMessage = [](const String) {};
    ExitCallback onExit = nullptr;
    uint64_t clientId = 0;
    String userScript = "";
    String runtimePrimordialOverrides = "";

  };
}
#endif
