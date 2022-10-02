#ifndef SSC_WINDOW_WINDOW_H
#define SSC_WINDOW_WINDOW_H

#ifdef __APPLE__
#import <Webkit/Webkit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "../core/apple.hh"
#elif defined(__linux__) && !defined(__ANDROID__)
#include <JavaScriptCore/JavaScript.h>
#include <webkit2/webkit2.h>
#include <gtk/gtk.h>
#endif

#include "../core/core.hh"
#include "../app/app.hh"

namespace SSC {
  class Window;

  enum {
    WINDOW_HINT_NONE = 0,  // Width and height are default size
    WINDOW_HINT_MIN = 1,   // Width and height are minimum bounds
    WINDOW_HINT_MAX = 2,   // Width and height are maximum bounds
    WINDOW_HINT_FIXED = 3  // Window size can not be changed by a user
  };

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

  class IWindow {
    public:
      int index = 0;
      void* bridge;
      WindowOptions opts;
      MessageCallback onMessage = [](const std::string) {};
      ExitCallback onExit = nullptr;
      void resolvePromise (const std::string& seq, const std::string& state, const std::string& value);

      virtual void eval(const std::string&) = 0;
      virtual void show(const std::string&) = 0;
      virtual void hide(const std::string&) = 0;
      virtual void close(int code) = 0;
      virtual void exit(int code) = 0;
      virtual void kill() = 0;
      virtual void navigate(const std::string&, const std::string&) = 0;
      virtual void setSize(const std::string&, int, int, int) = 0;
      virtual void setTitle(const std::string&, const std::string&) = 0;
      virtual void setContextMenu(const std::string&, const std::string&) = 0;
      virtual void setSystemMenu(const std::string&, const std::string&) = 0;
      virtual void setSystemMenuItemEnabled(bool enabled, int barPos, int menuPos) = 0;
      virtual void openDialog(const std::string&, bool, bool, bool, bool, const std::string&, const std::string&, const std::string&) = 0;
      virtual void setBackgroundColor(int r, int g, int b, float a) = 0;
      virtual void showInspector() = 0;
      virtual ScreenSize getScreenSize() = 0;
  };

  inline void IWindow::resolvePromise (const std::string& seq, const std::string& state, const std::string& value) {
    if (seq.find("R") == 0) {
      this->eval(resolveToRenderProcess(seq, state, value));
    }
    this->onMessage(resolveToMainProcess(seq, state, value));
  }

  class Window : public IWindow {
#if defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    NSWindow* window;
    SSCBridgedWebView* webview;
#elif defined(__linux__) && !defined(__ANDROID__)
    GtkSelectionData *selectionData = nullptr;
    GtkAccelGroup *accelGroup = nullptr;
    GtkWidget *webview = nullptr;
    GtkWidget *window = nullptr;
    GtkWidget *menubar = nullptr;
    GtkWidget *vbox = nullptr;
    GtkWidget *popup = nullptr;
    std::vector<std::string> draggablePayload;
    double dragLastX = 0;
    double dragLastY = 0;
    bool isDragInvokedInsideWindow;
    int popupId;
#endif

    public:
      App app;
      WindowOptions opts;
      Window(App&, WindowOptions);

      void eval(const std::string&);
      void show(const std::string&);
      void hide(const std::string&);
      void kill();
      void exit(int code);
      void close(int code);
      void navigate(const std::string&, const std::string&);
      void setTitle(const std::string&, const std::string&);
      void setSize(const std::string&, int, int, int);
      void setContextMenu(const std::string&, const std::string&);
      void closeContextMenu(const std::string&);
      void setBackgroundColor(int r, int g, int b, float a);
      void closeContextMenu();
      void openDialog(const std::string&, bool, bool, bool, bool, const std::string&, const std::string&, const std::string&);
      void showInspector();

      void setSystemMenu(const std::string& seq, const std::string& menu);
      void setSystemMenuItemEnabled(bool enabled, int barPos, int menuPos);
      int openExternal(const std::string& s);
      ScreenSize getScreenSize();
  };

}

#endif
