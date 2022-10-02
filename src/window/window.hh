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
#elif defined(_WIN32)
#include "win64/WebView2.h"
#include "win64/WebView2EnvironmentOptions.h"
#include <shobjidl.h>
#include <shlobj_core.h>

#include <dwmapi.h>
#include <wingdi.h>
#endif

#include "../core/core.hh"
#include "../app/app.hh"
#include "options.hh"

namespace SSC {
  class Window;

  enum {
    WINDOW_HINT_NONE = 0,  // Width and height are default size
    WINDOW_HINT_MIN = 1,   // Width and height are minimum bounds
    WINDOW_HINT_MAX = 2,   // Width and height are maximum bounds
    WINDOW_HINT_FIXED = 3  // Window size can not be changed by a user
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
    public:
      App app;
      WindowOptions opts;

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
#elif defined(_WIN32)
      static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
      ICoreWebView2Controller *controller = nullptr;
      ICoreWebView2 *webview = nullptr;
      HMENU systemMenu;
      DWORD mainThread = GetCurrentThreadId();
      POINT m_minsz = POINT {0, 0};
      POINT m_maxsz = POINT {0, 0};
      DragDrop* drop;
      HWND window;
      void resize (HWND window);
#endif

      Window(App&, WindowOptions);

      void about();
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
      void closeContextMenu();
      void setBackgroundColor(int r, int g, int b, float a);
      void openDialog( // @TODO(jwerle): use `OpenDialogOptions` here instead
        const std::string&,
        bool,
        bool,
        bool,
        bool,
        const std::string&, const std::string&, const std::string&
      );
      void showInspector();

      void setSystemMenu(const std::string& seq, const std::string& menu);
      void setSystemMenuItemEnabled(bool enabled, int barPos, int menuPos);
      int openExternal(const std::string& s);
      ScreenSize getScreenSize();
  };
}
#endif
