#ifndef SSC_APP_APP_H
#define SSC_APP_APP_H

#include "../core/core.hh"
#include "../ipc/ipc.hh"

namespace SSC {
  class App {
    // an opaque pointer to the configured `WindowFactory<Window, App>`
    void *windowFactory = nullptr;
    public:
      static inline std::atomic<bool> isReady = false;

#if defined(__APPLE__) && !defined(TARGET_OS_IPHONE) && !defined(TARGET_IPHONE_SIMULATOR)
      NSAutoreleasePool* pool = [NSAutoreleasePool new];
#elif defined(_WIN32)
      MSG msg;
      WNDCLASSEX wcex;
      _In_ HINSTANCE hInstance;
#endif

      ExitCallback onExit = nullptr;
      bool shouldExit = false;
      bool fromSSC = false;
      Map appData;
      Core *core;

#ifdef _WIN32
      App (void *);
#endif

      App (int);
      App ();

      int run ();
      void kill ();
      void exit (int code);
      void restart ();
      void dispatch (std::function<void()>);
      SSC::String getCwd ();
      void setWindowFactory (void *);
      void * getWindowFactory ();
  };

#if defined(_WIN32)
  extern FILE* console;
#endif
}
#endif
