#ifndef SSC_APP_APP_H
#define SSC_APP_APP_H

#include "../core/core.hh"
#include "../ipc/ipc.hh"
#include "../window/window.hh"

namespace SSC {
  class App;
}

#ifdef SSC_PLATFORM_APPLE
@interface SSCApplicationDelegate :
#if SSC_PLATFORM_MACOS
  NSObject<NSApplicationDelegate>
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

#elif SSC_PLATFORM_IOS
  UIResponder <UIApplicationDelegate>
  - (BOOL) application: (UIApplication*) application
  continueUserActivity: (NSUserActivity*) userActivity
    restorationHandler: (void (^)(NSArray<id<UIUserActivityRestoring>>*)) restorationHandler;

              - (BOOL) application: (UIApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType;
#endif

@property (nonatomic, assign) SSC::App* app;
@end
#endif

namespace SSC {
  class App {
    public:
      static inline Atomic<bool> isReady = false;
      static App* sharedApplication ();

    #if SSC_PLATFORM_APPLE
      // created and set in `App::App()` on macOS or
      // created by `UIApplicationMain` and set in `application:didFinishLaunchingWithOptions:` on iOS
      SSCApplicationDelegate* applicationDelegate = nullptr;
    #endif

    #if SSC_PLATFORM_MACOS
      NSAutoreleasePool* pool = [NSAutoreleasePool new];
    #elif SSC_PLATFORM_WINDOWS
      Atomic<bool> isConsoleVisible = false;
      _In_ HINSTANCE hInstance;
      WNDCLASSEX wcex;
      MSG msg;
    #endif

      ExitCallback onExit = nullptr;
      AtomicBool shouldExit = false;
      AtomicBool killed = false;
      bool wasLaunchedFromCli = false;
      bool w32ShowConsole = false;

      WindowManager windowManager;
      SharedPointer<Core> core = nullptr;
      Map userConfig = SSC::getUserConfig();

    #if SSC_PLATFORM_WINDOWS
      App (void *);
      void ShowConsole ();
      void HideConsole ();
    #endif

      App (int, SharedPointer<Core> = SharedPointer<Core>(new Core()));
      App (SharedPointer<Core> = SharedPointer<Core>(new Core()));
      App () = delete;
      App (const App&) = delete;
      App (App&&) = delete;
      ~App ();

      int run (int argc = 0, char** argv = nullptr);
      void kill ();
      void exit (int code);
      void restart ();
      void dispatch (Function<void()>);
      String getcwd ();
  };

}
#endif
