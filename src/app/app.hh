#ifndef SSC_APP_APP_H
#define SSC_APP_APP_H

#include "../core/core.hh"
#include "../ipc/ipc.hh"

namespace SSC {
  class App;
}

#if defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
@interface SSCApplicationDelegate : NSObject<NSApplicationDelegate>
@property (nonatomic) SSC::App* app;
@property (strong, nonatomic) NSStatusItem *statusItem;
- (void) applicationDidFinishLaunching: (NSNotification*) notification;
- (void) application: (NSApplication*) application openURLs: (NSArray<NSURL*>*) urls;

- (BOOL) application: (NSApplication*) application
continueUserActivity: (NSUserActivity*) userActivity
  restorationHandler: (void (^)(NSArray*)) restorationHandler;

              - (BOOL) application: (NSApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType;

                 - (void) application: (NSApplication*) application
didFailToContinueUserActivityWithType: (NSString*) userActivityType
                                error: (NSError*) error;
@end
#endif

namespace SSC {
  class WindowManager;

  class App {
    // an opaque pointer to the configured `WindowManager<Window, App>`
    public:
      static inline std::atomic<bool> isReady = false;
      static App* instance ();

    #if defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      NSAutoreleasePool* pool = [NSAutoreleasePool new];
      SSCApplicationDelegate* delegate = [SSCApplicationDelegate new];
    #elif defined(_WIN32)
      MSG msg;
      WNDCLASSEX wcex;
      _In_ HINSTANCE hInstance;
    #endif

      WindowManager *windowManager = nullptr;
      ExitCallback onExit = nullptr;
      AtomicBool shouldExit = false;
      bool fromSSC = false;
      bool w32ShowConsole = false;
      Map appData;
      Core *core;

    #ifdef _WIN32
      App (void *);
      void ShowConsole();
      void HideConsole();
      bool consoleVisible = false;
    #endif

      App (int);
      App ();
      ~App ();

      int run ();
      void kill ();
      void exit (int code);
      void restart ();
      void dispatch (std::function<void()>);
      SSC::String getCwd ();
      void setWindowManager (WindowManager*);
      WindowManager* getWindowManager () const;
  };

}
#endif
