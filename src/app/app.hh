#ifndef SSC_APP_APP_H
#define SSC_APP_APP_H

#include "../core/core.hh"
#include "../ipc/ipc.hh"

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
      void exit (int code);
      void setWindowFactory (void *windowFactory) {
        this->windowFactory = windowFactory;
      }

      void * getWindowFactory() {
        return this->windowFactory;
      }

      virtual int run () = 0;
      virtual void kill () = 0;
      virtual void restart () = 0;
      virtual void dispatch (std::function<void()> callback) = 0;
      virtual SSC::String getCwd () = 0;
  };

  inline void IApp::exit (int code) {
    if (onExit != nullptr) onExit(code);
  }

  class App : public IApp {
    public:
#if defined(__APPLE__) && !defined(TARGET_OS_IPHONE) && !defined(TARGET_IPHONE_SIMULATOR)
      NSAutoreleasePool* pool = [NSAutoreleasePool new];
#elif defined(_WIN32)
      MSG msg;
      WNDCLASSEX wcex;
      _In_ HINSTANCE hInstance;
      DWORD mainThread = GetCurrentThreadId();
#endif

      static std::atomic<bool> isReady;
      bool fromSSC = false;
      Map appData;
      Core *core;
      IPC::Bridge bridge;

#ifdef _WIN32
      App (void *);
#else
      App (int);
#endif

      App ();
      int run ();
      void kill ();
      void restart ();
      void dispatch (std::function<void()> work);
      SSC::String getCwd ();
  };

#if defined(_WIN32)
  extern FILE* console;
#endif
}
#endif
