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
      static constexpr int DEFAULT_INSTANCE_ID = 0;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      // created and set in `App::App()` on macOS or
      // created by `UIApplicationMain` and set in `application:didFinishLaunchingWithOptions:` on iOS
      SSCApplicationDelegate* applicationDelegate = nullptr;
      #if SOCKET_RUNTIME_PLATFORM_MACOS
        NSAutoreleasePool* pool = [NSAutoreleasePool new];
      #endif
    #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
      _In_ HINSTANCE hInstance;
      WNDCLASSEX wcex;
      MSG msg;
    #elif SOCKET_RUNTIME_PLATFORM_ANDROID
      Android::BuildInformation androidBuildInformation;
      Android::Looper androidLooper;
      Android::JVMEnvironment jvm;
      JNIEnv* jni = nullptr;
      jobject self = nullptr;
      jobject appActivity = nullptr;
      bool isAndroidEmulator = false;
    #endif

      ExitCallback onExit = nullptr;
      AtomicBool shouldExit = false;
      AtomicBool killed = false;
      bool wasLaunchedFromCli = false;

      Vector<String> pendingApplicationURLs;
      WindowManager windowManager;
      ServiceWorkerContainer serviceWorkerContainer;
      SharedPointer<Core> core = nullptr;
      Map userConfig;

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      /**
       * `App` class constructor for Android.
       * The `App` instance is constructed from the context of
       * the shared `Application` singleton on Android. This is a
       * special case constructor.
       */
      App (
        JNIEnv* env,
        jobject self,
        SharedPointer<Core> core = SharedPointer<Core>(new Core())
      );
    #else
      /**
       * `App` class constructor for desktop (Linux, macOS, Windows) and
       * iOS (iPhoneOS, iPhoneSimulator, iPad) where `instanceId` can be
       * a `HINSTANCE` on Windows or an empty value (`0`) on other platforms.
       */
      App (
      #if SOCKET_RUNTIME_PLATFORM_WINDOWS
        HINSTANCE instanceId = 0,
      #else
        int instanceId = 0,
      #endif
        SharedPointer<Core> core = SharedPointer<Core>(new Core())
      );
    #endif

      App () = delete;
      App (const App&) = delete;
      App (App&&) = delete;
      ~App ();

      App& operator = (App&) = delete;
      App& operator = (App&&) = delete;

      int run (int argc = 0, char** argv = nullptr);
      void init ();
      void kill ();
      void exit (int code);
      void restart ();
      void dispatch (Function<void()>);
      String getcwd ();
      bool hasRuntimePermission (const String& permission) const;

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
      LRESULT forwardWindowProcMessage (HWND, UINT, WPARAM, LPARAM);
    #endif
  };
}
#endif
