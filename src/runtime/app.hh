#ifndef SOCKET_RUNTIME_APP_H
#define SOCKET_RUNTIME_APP_H

#include "platform.hh"
#include "context.hh"
#include "runtime.hh"
#include "config.hh"

#if SOCKET_RUNTIME_PLATFORM_IOS
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#endif

namespace ssc::runtime::app {
  class App;
}

#if SOCKET_RUNTIME_PLATFORM_APPLE
@interface SSCApplicationDelegate :
#if SOCKET_RUNTIME_PLATFORM_MACOS
  NSObject<NSApplicationDelegate>
  @property (strong, nonatomic) NSStatusItem *statusItem;
  - (void) applicationDidFinishLaunching: (NSNotification*) notification;
  - (void) application: (NSApplication*) application
              openURLs: (NSArray<NSURL*>*) urls;

  - (BOOL) application: (NSApplication*) application
  continueUserActivity: (NSUserActivity*) userActivity
    restorationHandler: (void (^)(NSArray*)) restorationHandler;

              - (BOOL) application: (NSApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType;

                   - (void) application: (NSApplication*) application
  didFailToContinueUserActivityWithType: (NSString*) userActivityType
                                  error: (NSError*) error;

#elif SOCKET_RUNTIME_PLATFORM_IOS
  UIResponder <UIApplicationDelegate>
  @property (nonatomic, strong) CADisplayLink *displayLink;
  @property (nonatomic, assign) CGFloat keyboardHeight;
  @property (nonatomic, assign) BOOL inMotion;
  @property (nonatomic, assign) CGFloat progress;
  @property (nonatomic, assign) NSTimeInterval animationDuration;

  - (BOOL) application: (UIApplication*) application
  continueUserActivity: (NSUserActivity*) userActivity
    restorationHandler: (void (^)(NSArray<id<UIUserActivityRestoring>>*)) restorationHandler;

              - (BOOL) application: (UIApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType;
#endif

@property (nonatomic, assign) ssc::runtime::app::App* app;
@end
#endif

namespace ssc::runtime::app {
  class App {
    public:
      static runtime::SharedPointer<App> sharedApplication ();
      static void releaseApplication ();

      enum class LaunchSource {
        Tool,
        Platform
      };

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
      using InstanceID = HINSTANCE;
    #else
      using InstanceID = int;
    #endif

      using UserConfig = Runtime::UserConfig;
      using ShutdownHandler = Function<void(int)>;

      struct Options : public runtime::Options {
        InstanceID instanceId = 0;
        UserConfig userConfig = config::getUserConfig();
        loop::Loop::Options loop;
        Runtime::Features features;
      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        JNIEnv* env;
        jobject self;
      #endif
      };

      // handlers
      ShutdownHandler shutdownHandler = nullptr;

      // state
      Atomic<bool> shouldExit = false;
      Atomic<bool> isStarted = false;
      Atomic<bool> isStopped = false;
      Atomic<bool> isPaused = false;

      LaunchSource launchSource = LaunchSource::Platform;
      Runtime runtime;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      // created and set in `App::App()` on macOS or created by
      // `UIApplicationMain` and set in the
      // `application:didFinishLaunchingWithOptions:` delegate methdo on iOS
      SSCApplicationDelegate* delegate = nullptr;
    #if SOCKET_RUNTIME_PLATFORM_MACOS
      NSAutoreleasePool* pool = [NSAutoreleasePool new];
    #endif
    #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
      _In_ HINSTANCE instance;
      WNDCLASSEX wcex;
    #endif

      App (const Options&);
      App () = delete;
      App (const App&) = delete;
      App (App&&) = delete;
      virtual ~App ();

      App& operator = (App&) = delete;
      App& operator = (App&&) = delete;

      virtual int run (int argc = 0, char** argv = nullptr);
      virtual void init ();
      virtual void stop ();
      virtual void resume ();
      virtual void pause ();
      virtual void dispatch (Function<void()>);

      virtual bool paused () const;
      virtual bool started () const;
      virtual bool stopped () const;

      bool hasRuntimePermission (const String& permission) const;

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
      LRESULT handleWindowProcMessage (HWND, UINT, WPARAM, LPARAM);
    #endif
  };

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
  LRESULT CALLBACK onWindowProcMessage (
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
  );

  void registerWindowClass (App*);
#endif
}
#endif
