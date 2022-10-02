#ifndef SSC_APP_APP_H
#define SSC_APP_APP_H

#ifdef __APPLE__
#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
#include <Cocoa/Cocoa.h>
#include <objc/objc-runtime.h>
#endif
#elif defined(_WIN32)
#include "desktop/win64/WebView2.h"
#include "desktop/win64/WebView2EnvironmentOptions.h"

#include <signal.h>
#include <future>
#include <chrono>
#include <shobjidl.h>
#include <shlobj_core.h>

#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"WebView2LoaderStatic.lib")
#pragma comment(lib,"uv_a.lib")

// for dark mode...
#include <uxtheme.h>
#include <dwmapi.h>
#include <wingdi.h>

#pragma comment(lib,"UxTheme.lib")
#pragma comment(lib,"Dwmapi.lib")
#pragma comment(lib,"Gdi32.lib")
#endif

#include "../core/common.hh"

namespace SSC {
  //
  // Interfaces make sure all operating systems implement the same stuff
  //
  class IApp {
    // an opaque pointer to the configured `WindowFactory<Window, App>`
    void *windowFactory = nullptr;

    public:
      bool shouldExit = false;
      bool fromSSC = false;
      ExitCallback onExit = nullptr;
      void exit(int code);
      void setWindowFactory(void *windowFactory) {
        this->windowFactory = windowFactory;
      }

      void * getWindowFactory() {
        return this->windowFactory;
      }

      virtual int run() = 0;
      virtual void kill() = 0;
      virtual void restart() = 0;
      virtual void dispatch(std::function<void()> work) = 0;
      virtual std::string getCwd(const std::string&) = 0;
  };

  inline void IApp::exit (int code) {
    if (onExit != nullptr) onExit(code);
  }

  class App : public IApp {
#ifdef __APPLE__
#if !defined(TARGET_OS_IPHONE) && !defined(TARGET_IPHONE_SIMULATOR)
    NSAutoreleasePool* pool = [NSAutoreleasePool new];
#endif
#elif defined(_WIN32)
      MSG msg;
      WNDCLASSEX wcex;
      _In_ HINSTANCE hInstance;
      DWORD mainThread = GetCurrentThreadId();
#endif

    public:
      static std::atomic<bool> isReady;
      bool fromSSC = false;
#ifdef _WIN32
      App (void *);
#else
      App (int);
#endif
      int run();
      void kill();
      void restart();
      void dispatch(std::function<void()> work);
      std::string getCwd(const std::string&);
  };
}
#endif
