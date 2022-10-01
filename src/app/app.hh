#ifndef SSC_APP_APP_H
#define SSC_APP_APP_H

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#include <objc/objc-runtime.h>
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
    NSAutoreleasePool* pool = [NSAutoreleasePool new];
#endif

    public:
      bool fromSSC = false;
      App(int);
      int run();
      void kill();
      void restart();
      void dispatch(std::function<void()> work);
      std::string getCwd(const std::string&);
  };
}
#endif
