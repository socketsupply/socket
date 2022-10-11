#ifndef SSC_APP_APP_H
#define SSC_APP_APP_H

#include "../core/core.hh"
#include "../ipc/ipc.hh"

namespace SSC {
#ifdef __linux__
  class Bridge;
#endif

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
#ifdef __APPLE__
#if !defined(TARGET_OS_IPHONE) && !defined(TARGET_IPHONE_SIMULATOR)
      NSAutoreleasePool* pool = [NSAutoreleasePool new];
#endif
#elif defined(_WIN32)
      MSG msg;
      WNDCLASSEX wcex;
      _In_ HINSTANCE hInstance;
      DWORD mainThread = GetCurrentThreadId();
#elif defined(__linux__)
      Bridge *bridge;
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

#ifdef __linux__
  class Bridge {
    public:
      App *app;
      Core *core;

      Bridge (App *app) {
        this->core = new Core();
        this->app = app;
      }

      bool route (String msg, char *buf, size_t bufsize);
      void send (IPC::Message message, String seq, String msg, Post post);
      bool invoke (IPC::Message message, char *buf, size_t bufsize, Callback cb);
      bool invoke (IPC::Message message, Callback cb);
  };
#endif

#if defined(_WIN32)
  extern FILE* console;
#endif
}
#endif
