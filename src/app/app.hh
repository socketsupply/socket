#ifndef SOCKET_RUNTIME_APP_APP_H
#define SOCKET_RUNTIME_APP_APP_H

#include "../window/window.hh"
#include "../serviceworker/container.hh"

#include "../core/core.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "../platform/android.hh"
#endif

#if SOCKET_RUNTIME_PLATFORM_IOS
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#endif

namespace SSC {
  class App;
}

#if SOCKET_RUNTIME_PLATFORM_APPLE
@interface SSCApplicationDelegate :
#if SOCKET_RUNTIME_PLATFORM_MACOS
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

@property (nonatomic, assign) SSC::App* app;
@end
#endif

namespace SSC {
  class App {
    public:
      static inline Atomic<bool> isReady = false;
      static App* sharedApplication ();

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      // created and set in `App::App()` on macOS or
      // created by `UIApplicationMain` and set in `application:didFinishLaunchingWithOptions:` on iOS
      SSCApplicationDelegate* applicationDelegate = nullptr;
    #endif

    #if SOCKET_RUNTIME_PLATFORM_MACOS
      NSAutoreleasePool* pool = [NSAutoreleasePool new];
    #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
      Atomic<bool> isConsoleVisible = false;
      _In_ HINSTANCE hInstance;
      WNDCLASSEX wcex;
      MSG msg;
    #elif SOCKET_RUNTIME_PLATFORM_ANDROID
      Android::BuildInformation androidBuildInformation;
      Android::Looper androidLooper;
      Android::JVMEnvironment jvm;
      JNIEnv* jni;
      jobject self;
      jobject appActivity;
      bool isAndroidEmulator = false;
    #endif

      ExitCallback onExit = nullptr;
      AtomicBool shouldExit = false;
      AtomicBool killed = false;
      bool wasLaunchedFromCli = false;
      bool w32ShowConsole = false;

      WindowManager windowManager;
      ServiceWorkerContainer serviceWorkerContainer;
      SharedPointer<Core> core = nullptr;
      Map userConfig;

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
      App (void *);
      void ShowConsole ();
      void HideConsole ();
    #endif

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      App (JNIEnv* env, jobject self, SharedPointer<Core> core = SharedPointer<Core>(new Core()));
    #else
      App (int instanceId, SharedPointer<Core> core = SharedPointer<Core>(new Core()));
      App (SharedPointer<Core> core = SharedPointer<Core>(new Core()));
    #endif
      App () = delete;
      App (const App&) = delete;
      App (App&&) = delete;
      ~App ();

      int run (int argc = 0, char** argv = nullptr);
      void init ();
      void kill ();
      void exit (int code);
      void restart ();
      void dispatch (Function<void()>);
      String getcwd ();
      bool hasRuntimePermission (const String& permission) const;

      /*
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      bool isAndroidPermissionAllowed (const String& permission);
    #endif
    */
  };
}
#endif
