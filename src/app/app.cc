#include "app.hh"

using namespace SSC;

#if SOCKET_RUNTIME_PLATFORM_APPLE
static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create(
  "socket.runtime.app.queue",
  qos
);

@implementation SSCApplicationDelegate
#if SOCKET_RUNTIME_PLATFORM_MACOS
- (void) applicationDidFinishLaunching: (NSNotification*) notification {
  self.statusItem = [NSStatusBar.systemStatusBar statusItemWithLength: NSVariableStatusItemLength];
}

- (void) applicationWillBecomeActive: (NSNotification*) notification {
  dispatch_async(queue, ^{
    self.app->resume();
  });
}

- (void) applicationWillResignActive: (NSNotification*) notification {
  dispatch_async(queue, ^{
    // self.app->pause();
  });
}

- (void) menuWillOpen: (NSMenu*) menu {
  auto app = self.app;
  auto window = app->windowManager.getWindow(0);
  if (!window) return;
  auto w = window->window;
  if (!w) return;

  [w makeKeyAndOrderFront: nil];
  [NSApp activateIgnoringOtherApps:YES];

  if (app != nullptr) {
    for (auto window : self.app->windowManager.windows) {
      if (window != nullptr) window->bridge.emit("tray", JSON::Object {});
    }
  }
}

- (void) application: (NSApplication*) application openURLs: (NSArray<NSURL*>*) urls {
  auto app = self.app;
  if (app != nullptr) {
    for (NSURL* url in urls) {
      for (auto& window : self.app->windowManager.windows) {
        if (window != nullptr) {
          window->handleApplicationURL(url.absoluteString.UTF8String);
        }
      }
    }
  }
}

- (BOOL) application: (NSApplication*) application
continueUserActivity: (NSUserActivity*) userActivity
  restorationHandler: (void (^)(NSArray*)) restorationHandler
{
  return [self
                         application: application
    willContinueUserActivityWithType: userActivity.activityType
  ];
}

              - (BOOL) application: (NSApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType
{
  static auto userConfig = getUserConfig();
  const auto webpageURL = application.userActivity.webpageURL;

  if (userActivityType == nullptr) {
    return NO;
  }

  if (webpageURL == nullptr) {
    return NO;
  }

  if (webpageURL.host == nullptr) {
    return NO;
  }

  const auto activityType = String(userActivityType.UTF8String);

  if (activityType != String(NSUserActivityTypeBrowsingWeb.UTF8String)) {
    return NO;
  }

  const auto host = String(webpageURL.host.UTF8String);
  const auto links = parseStringList(userConfig["meta_application_links"], ' ');

  if (links.size() == 0) {
    return NO;
  }

  bool exists = false;

  for (const auto& link : links) {
    const auto parts = split(link, '?');
    if (host == parts[0]) {
      exists = true;
      break;
    }
  }

  if (!exists) {
    return NO;
  }

  const auto url = String(webpageURL.absoluteString.UTF8String);
  bool emitted = false;

  for (auto& window : self.app->windowManager.windows) {
    if (window != nullptr) {
      window->handleApplicationURL(url);
      emitted = true;
    }
  }

  if (!emitted) {
    return NO;
  }

  return YES;
}

                 - (void) application: (NSApplication*) application
didFailToContinueUserActivityWithType: (NSString*) userActivityType
                                error: (NSError*) error {
  debug("application:didFailToContinueUserActivityWithType:error: %@", error);
}

#elif SOCKET_RUNTIME_PLATFORM_IOS
-           (BOOL) application: (UIApplication*) application
 didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{
  auto const notificationCenter = NSNotificationCenter.defaultCenter;
  const auto frame = UIScreen.mainScreen.bounds;

  Vector<String> argv;
  bool isTest = false;

  self.app = App::sharedApplication();
  self.app->applicationDelegate = self;

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardDidShow:)
           name: UIKeyboardDidShowNotification
         object: nil
  ];

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardDidHide:)
           name: UIKeyboardDidHideNotification
         object: nil
  ];

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardWillShow:)
           name: UIKeyboardWillShowNotification
         object: nil
  ];

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardWillHide:)
           name: UIKeyboardWillHideNotification
         object: nil
  ];

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardWillChange:)
           name: UIKeyboardWillChangeFrameNotification
         object: nil
  ];

  for (const auto& arg : split(self.app->userConfig["ssc_argv"], ',')) {
    if (arg.find("--test") == 0) {
      isTest = true;
    }

    argv.push_back("'" + trim(arg) + "'");
  }

  auto windowManagerOptions = WindowManagerOptions {};

  for (const auto& arg : split(self.app->userConfig["ssc_argv"], ',')) {
    if (arg.find("--test") == 0) {
      windowManagerOptions.features.useTestScript = true;
    }

    windowManagerOptions.argv.push_back("'" + trim(arg) + "'");
  }


  windowManagerOptions.userConfig = self.app->userConfig;

  self.app->windowManager.configure(windowManagerOptions);

  static const auto port = getDevPort();
  static const auto host = getDevHost();

  if (
    self.app->userConfig["webview_service_worker_mode"] != "hybrid" &&
    self.app->userConfig["permissions_allow_service_worker"] != "false"
  ) {
    auto serviceWorkerWindowOptions = Window::Options {};
    auto serviceWorkerUserConfig = self.app->userConfig;

    serviceWorkerUserConfig["webview_watch_reload"] = "false";
    serviceWorkerWindowOptions.shouldExitApplicationOnClose = false;
    serviceWorkerWindowOptions.index = SOCKET_RUNTIME_SERVICE_WORKER_CONTAINER_WINDOW_INDEX;
    serviceWorkerWindowOptions.headless = Env::get("SOCKET_RUNTIME_SERVICE_WORKER_DEBUG").size() == 0;
    serviceWorkerWindowOptions.userConfig = serviceWorkerUserConfig;
    serviceWorkerWindowOptions.features.useGlobalCommonJS = false;
    serviceWorkerWindowOptions.features.useGlobalNodeJS = false;

    auto serviceWorkerWindow = self.app->windowManager.createWindow(serviceWorkerWindowOptions);
    self.app->serviceWorkerContainer.init(&serviceWorkerWindow->bridge);

    serviceWorkerWindow->navigate(
      "socket://" + self.app->userConfig["meta_bundle_identifier"] + "/socket/service-worker/index.html"
    );
  }

  auto defaultWindow = self.app->windowManager.createDefaultWindow(Window::Options {
     .shouldExitApplicationOnClose = true
  });

  if (self.app->userConfig["webview_service_worker_mode"] == "hybrid") {
    self.app->serviceWorkerContainer.init(&defaultWindow->bridge);
  }

  defaultWindow->setTitle(self.app->userConfig["meta_title"]);

  if (isDebugEnabled() && port > 0 && host.size() > 0) {
    defaultWindow->navigate(host + ":" + std::to_string(port));
  } else if (self.app->userConfig["webview_root"].size() != 0) {
    defaultWindow->navigate(
      "socket://" + self.app->userConfig["meta_bundle_identifier"] + self.app->userConfig["webview_root"]
    );
  } else {
    defaultWindow->navigate(
      "socket://" + self.app->userConfig["meta_bundle_identifier"] + "/index.html"
    );
  }

  defaultWindow->show();

  return YES;
}

- (void) applicationDidEnterBackground: (UIApplication*) application {
  for (const auto& window : self.app->windowManager.windows) {
    if (window != nullptr) {
      window->eval("window.blur()");
    }
  }
}

- (void) applicationWillEnterForeground: (UIApplication*) application {
  for (const auto& window : self.app->windowManager.windows) {
    if (window != nullptr) {
      if (!window->webview.isHidden) {
        window->eval("window.focus()");
      }
    }
  }

  for (const auto& window : self.app->windowManager.windows) {
    if (window != nullptr) {
      // XXX(@jwerle): maybe bluetooth should be a singleton instance with observers
      window->bridge.bluetooth.startScanning();
    }
  }
}

- (void) applicationWillTerminate: (UIApplication*) application {
  dispatch_async(queue, ^{
    // TODO(@jwerle): what should we do here?
    self.app->stop();
  });
}

- (void) applicationDidBecomeActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    self.app->resume();
  });
}

- (void) applicationWillResignActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    self.app->pause();
  });
}

-  (BOOL) application: (UIApplication*) application
 continueUserActivity: (NSUserActivity*) userActivity
   restorationHandler: (void (^)(NSArray<id<UIUserActivityRestoring>>*)) restorationHandler
{
  return [self
                         application: application
    willContinueUserActivityWithType: userActivity.activityType
  ];
}

              - (BOOL) application: (UIApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType
{
  static auto userConfig = getUserConfig();
  const auto webpageURL = application.userActivity.webpageURL;

  if (userActivityType == nullptr) {
    return NO;
  }

  if (webpageURL == nullptr) {
    return NO;
  }

  if (webpageURL.host == nullptr) {
    return NO;
  }

  const auto activityType = String(userActivityType.UTF8String);

  if (activityType != String(NSUserActivityTypeBrowsingWeb.UTF8String)) {
    return NO;
  }

  const auto host = String(webpageURL.host.UTF8String);
  const auto links = parseStringList(userConfig["meta_application_links"], ' ');

  if (links.size() == 0) {
    return NO;
  }

  bool exists = false;

  for (const auto& link : links) {
    const auto parts = split(link, '?');
    if (host == parts[0]) {
      exists = true;
      break;
    }
  }

  if (!exists) {
    return NO;
  }

  bool emitted = false;
  for (const auto& window : self.app->windowManager.windows) {
    if (window != nullptr) {
      window->handleApplicationURL(webpageURL.absoluteString.UTF8String);
      emitted = true;
    }
  }

  return emitted;
}

- (void) keyboardWillHide: (NSNotification*) notification {
  const auto info = notification.userInfo;
  const auto keyboardFrameBegin = (NSValue*) [info valueForKey: UIKeyboardFrameEndUserInfoKey];
  const auto rect = [keyboardFrameBegin CGRectValue];
  const auto height = rect.size.height;

  const auto window = self.app->windowManager.getWindow(0);
  window->webview.scrollView.scrollEnabled = YES;

  for (const auto window : self.app->windowManager.windows) {
    if (window) {
      window->bridge.emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
          {"event", "will-hide"},
          {"height", height}
        }}
      });
    }
  }
}

- (void) keyboardDidHide: (NSNotification*) notification {
  for (const auto window : self.app->windowManager.windows) {
    if (window) {
      window->bridge.emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
          {"event", "did-hide"}
        }}
      });
    }
  }
}

- (void)keyboardWillShow: (NSNotification*) notification {
  const auto info = notification.userInfo;
  const auto keyboardFrameBegin = (NSValue*) [info valueForKey: UIKeyboardFrameEndUserInfoKey];
  const auto rect = [keyboardFrameBegin CGRectValue];
  const auto height = rect.size.height;

  const auto window = self.app->windowManager.getWindow(0);
  window->webview.scrollView.scrollEnabled = NO;

  for (const auto window : self.app->windowManager.windows) {
    if (window && !window->window.isHidden) {
      window->bridge.emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
        {"event", "will-show"},
        {"height", height}
        }}
      });
    }
  }
}

- (void) keyboardDidShow: (NSNotification*) notification {
  for (const auto window : self.app->windowManager.windows) {
    if (window && !window->window.isHidden) {
      window->bridge.emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
          {"event", "did-show"}
        }}
      });
    }
  }
}

- (void) keyboardWillChange: (NSNotification*) notification {
  const auto keyboardInfo = notification.userInfo;
  const auto keyboardFrameBegin = (NSValue* )[keyboardInfo valueForKey: UIKeyboardFrameEndUserInfoKey];
  const auto rect = [keyboardFrameBegin CGRectValue];
  const auto width = rect.size.width;
  const auto height = rect.size.height;

  for (const auto window : self.app->windowManager.windows) {
    if (window && !window->window.isHidden) {
      window->bridge.emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
          {"event", "will-change"},
          {"width", width},
          {"height", height},
        }}
      });
    }
  }
}

- (BOOL) application: (UIApplication*) app
             openURL: (NSURL*) url
             options: (NSDictionary<UIApplicationOpenURLOptionsKey, id>*) options
{
  for (const auto window : self.app->windowManager.windows) {
    if (window) {
      window->handleApplicationURL(url.absoluteString.UTF8String);
      return YES;
    }
  }

  return NO;
}
#endif
@end
#endif

namespace SSC {
  static App* applicationInstance = nullptr;

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
  static Atomic<bool> isConsoleVisible = false;
  static FILE* console = nullptr;

  static inline void alert (const WString &ws) {
    MessageBoxA(nullptr, convertWStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const String &s) {
    MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const char* s) {
    MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static void showWindowsConsole () {
    if (!isConsoleVisible) {
      isConsoleVisible = true;
      AllocConsole();
      freopen_s(&console, "CONOUT$", "w", stdout);
    }
  }

  static void hideWindowsConsole () {
    if (isConsoleVisible) {
      isConsoleVisible = false;
      fclose(console);
      FreeConsole();
    }
  }

  // message is defined in WinUser.h
  // https://raw.githubusercontent.com/tpn/winsdk-10/master/Include/10.0.10240.0/um/WinUser.h
  static LRESULT CALLBACK onWindowProcMessage (
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
  ) {
    auto app = SSC::App::sharedApplication();

    if (app == nullptr) {
      return 0;
    }

    auto window = reinterpret_cast<WindowManager::ManagedWindow*>(
      GetWindowLongPtr(hWnd, GWLP_USERDATA)
    );

    // invalidate `window` pointer that potentially is leaked
    if (window != nullptr && app->windowManager.getWindow(window->index).get() != window) {
      window = nullptr;
    }

    auto userConfig = window != nullptr
      ? reinterpret_cast<Window*>(window)->bridge.userConfig
      : getUserConfig();

    if (message == WM_COPYDATA) {
      auto copyData = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
      message = (UINT) copyData->dwData;
      wParam = (WPARAM) copyData->cbData;
      lParam = (LPARAM) copyData->lpData;
    }

    switch (message) {
      case WM_SIZE: {
        if (window == nullptr || window->controller == nullptr) {
          break;
        }

        RECT bounds;
        GetClientRect(hWnd, &bounds);
        window->size.height = bounds.bottom - bounds.top;
        window->size.width = bounds.right - bounds.left;
        window->controller->put_Bounds(bounds);
        break;
      }

      case WM_SOCKET_TRAY: {
        // XXX(@jwerle, @heapwolf): is this a correct for an `isAgent` predicate?
        auto isAgent = userConfig.count("tray_icon") != 0;

        if (window != nullptr && lParam == WM_LBUTTONDOWN) {
          SetForegroundWindow(hWnd);
          if (isAgent) {
            POINT point;
            GetCursorPos(&point);
            TrackPopupMenu(
              window->menutray,
              TPM_BOTTOMALIGN | TPM_LEFTALIGN,
              point.x,
              point.y,
              0,
              hWnd,
              NULL
            );
          }

          PostMessage(hWnd, WM_NULL, 0, 0);

          // broadcast an event to all the windows that the tray icon was clicked
          for (auto window : app->windowManager.windows) {
            if (window != nullptr) {
              window->bridge.emit("tray", JSON::Object {});
            }
          }
        }

        // XXX: falls through to `WM_COMMAND` below
      }

      case WM_COMMAND: {
        if (window == nullptr) {
          break;
        }

        if (window->menuMap.contains(wParam)) {
          String meta(window->menuMap[wParam]);
          auto parts = split(meta, '\t');

          if (parts.size() > 1) {
            auto title = parts[0];
            auto parent = parts[1];

            if (title.find("About") == 0) {
              reinterpret_cast<Window*>(window)->about();
              break;
            }

            if (title.find("Quit") == 0) {
              window->exit(0);
              break;
            }

            window->eval(getResolveMenuSelectionJavaScript("0", title, parent, "system"));
          }
        } else if (window->menuTrayMap.contains(wParam)) {
          String meta(window->menuTrayMap[wParam]);
          auto parts = split(meta, ':');

          if (parts.size() > 0) {
            auto title = trim(parts[0]);
            auto tag = parts.size() > 1 ? trim(parts[1]) : "";
            window->eval(getResolveMenuSelectionJavaScript("0", title, tag, "tray"));
          }
        }

        break;
      }

      case WM_SETTINGCHANGE: {
        // TODO(heapwolf): Dark mode
        break;
      }

      case WM_CREATE: {
        // TODO(heapwolf): Dark mode
        SetWindowTheme(hWnd, L"Explorer", NULL);
        SetMenu(hWnd, CreateMenu());
        break;
      }

      case WM_CLOSE: {
        if (!window || !window->options.closable) {
          break;
        }

        auto index = window->index;
        const JSON::Object json = JSON::Object::Entries {
          {"data", index}
        };

        for (auto window : app->windowManager.windows) {
          if (window != nullptr && window->index != index) {
            window->eval(getEmitToRenderProcessJavaScript("window-closed", json.str()));
          }
        }

        app->windowManager.destroyWindow(index);
        break;
      }

      case WM_HOTKEY: {
        if (window != nullptr) {
          window->hotkey.onHotKeyBindingCallback((HotKeyBinding::ID) wParam);
        }
        break;
      }

      case WM_HANDLE_DEEP_LINK: {
        const auto url = String(reinterpret_cast<const char*>(lParam), wParam);

        for (auto window : app->windowManager.windows) {
          if (window != nullptr) {
            window->handleApplicationURL(url);
          }
        }
        break;
      }

      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
  }
#endif

  App* App::sharedApplication () {
    return applicationInstance;
  }

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  App::App (
    JNIEnv* env,
    jobject self,
    SharedPointer<Core> core
  )
    : userConfig(getUserConfig()),
      core(core),
      windowManager(core),
      serviceWorkerContainer(core),
      jvm(env),
      androidLooper(env)
  {
    if (applicationInstance == nullptr) {
      applicationInstance = this;
    }

    this->jni = env;
    this->self = env->NewGlobalRef(self);
    this->init();
  }
#else
  App::App (
  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    HINSTANCE instanceId,
  #else
    int instanceId,
  #endif
    SharedPointer<Core> core
  ) : userConfig(getUserConfig()),
      core(core),
      windowManager(core),
      serviceWorkerContainer(core)
  {
    if (applicationInstance == nullptr) {
      applicationInstance = this;
    }

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    this->hInstance = instanceId;

    // this fixes bad default quality DPI.
    SetProcessDPIAware();

    if (userConfig["win_logo"].size() == 0 && userConfig["win_icon"].size() > 0) {
      userConfig["win_logo"] = fs::path(userConfig["win_icon"]).filename().string();
    }

    auto iconPath = fs::path { getcwd() / fs::path { userConfig["win_logo"] } };

    HICON icon = (HICON) LoadImageA(
      NULL,
      iconPath.string().c_str(),
      IMAGE_ICON,
      GetSystemMetrics(SM_CXICON),
      GetSystemMetrics(SM_CXICON),
      LR_LOADFROMFILE
    );

    auto windowClassName = userConfig["meta_bundle_identifier"];

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = windowClassName.c_str();
    wcex.hIconSm = icon; // ico doesn't auto scale, needs 16x16 icon lol fuck you bill
    wcex.hIcon = icon;
    wcex.lpfnWndProc = onWindowProcMessage;

    if (!RegisterClassEx(&wcex)) {
      alert("Application could not launch, possible missing resources.");
    }
  #endif

  #if !SOCKET_RUNTIME_DESKTOP_EXTENSION
    const auto cwd = getcwd();
    uv_chdir(cwd.c_str());
  #endif
    this->init();
  }
#endif

  App::~App () {
    if (applicationInstance == this) {
      applicationInstance = nullptr;
    }
  }

  void App::init () {
  #if SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_PLATFORM_LINUX
    gtk_init_check(0, nullptr);
  #elif SOCKET_RUNTIME_PLATFORM_MACOS
    this->applicationDelegate = [SSCApplicationDelegate new];
    this->applicationDelegate.app = this;
    NSApplication.sharedApplication.delegate = this->applicationDelegate;
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    OleInitialize(nullptr);
  #endif
  }

  int App::run (int argc, char** argv) {
  #if SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
    gtk_main();
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    // MUST be acquired on "main" thread
    // `run()` should called when the main activity is created
    if (!this->androidLooper.isAcquired()) {
      this->androidLooper.acquire();
    }
  #elif SOCKET_RUNTIME_PLATFORM_MACOS
    [NSApp run];
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    @autoreleasepool {
      return UIApplicationMain(
        argc,
        argv,
        nullptr,
        NSStringFromClass(SSCApplicationDelegate.class)
      );
    }
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    MSG msg;

    if (!GetMessage(&msg, nullptr, 0, 0)) {
      shouldExit = true;
      return 1;
    }

    if (msg.hwnd) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (msg.message == WM_APP) {
      // from PostThreadMessage
      auto callback = (Function<void()> *)(msg.lParam);
      (*callback)();
      delete callback;
    }

    if (msg.message == WM_QUIT && shouldExit) {
      return 1;
    }
  #endif

    if (shouldExit) {
      this->core->isShuttingDown = true;
    }

    return shouldExit ? 1 : 0;
  }

  void App::resume () {
    if (this->core != nullptr && this->paused) {
      this->paused = false;
      this->windowManager.emit("applicationresume");

      this->dispatch([this]() {
        this->core->resume();
      });
    }
  }

  void App::pause () {
    if (this->core != nullptr && !this->paused) {
      this->paused = true;
      this->windowManager.emit("applicationpause");

      this->dispatch([this]() {
        this->core->pause();
      });
    }
  }

  void App::stop () {
    if (this->stopped) {
      return;
    }

    this->stopped = true;
    this->windowManager.emit("applicationstop");
    this->pause();

    SSC::applicationInstance = nullptr;

    this->shouldExit = true;

    this->core->shutdown();
  #if SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
    gtk_main_quit();
  #elif SOCKET_RUNTIME_PLATFORM_MACOS
    // if not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (!wasLaunchedFromCli) {
      [NSApp terminate: nil];
    }
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    PostQuitMessage(0);
  #endif
  }

  void App::dispatch (Function<void()> callback) {
  #if SOCKET_RUNTIME_PLATFORM_LINUX
    g_main_context_invoke(
      nullptr,
      +[](gpointer userData) -> gboolean {
        auto callback = reinterpret_cast<Function<void()>*>(userData);
        if (*callback != nullptr) {
          (*callback)();
          delete callback;
        }
        return G_SOURCE_REMOVE;
      },
      new Function<void()>(callback)
    );
  #elif SOCKET_RUNTIME_PLATFORM_APPLE
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        callback();
      });
    });
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    static auto mainThread = GetCurrentThreadId();
    auto threadCallback = (LPARAM) new Function<void()>(callback);

    if (this->isReady) {
      PostThreadMessage(mainThread, WM_APP, 0, threadCallback);
      return;
    }

    Thread t([&, threadCallback] {

      // TODO(trevnorris): Need to also check a shouldExit so this doesn't run forever in case
      // the rest of the application needs to exit before isReady is set.
      while (!this->isReady) {
        msleep(16);
      }

      PostThreadMessage(mainThread, WM_APP, 0, threadCallback);
    });

    t.detach();
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    auto attachment = Android::JNIEnvironmentAttachment(this->jvm);
    this->androidLooper.dispatch([=, this] () {
      const auto attachment = Android::JNIEnvironmentAttachment(this->jvm);
      callback();
    });
  #endif
  }

  String App::getcwd () {
    return SSC::getcwd();
  }

  bool App::hasRuntimePermission (const String& permission) const {
    static const auto userConfig = getUserConfig();
    const auto key = String("permissions_allow_") + replace(permission, "-", "_");

    if (!userConfig.contains(key)) {
      return true;
    }

    return userConfig.at(key) != "false";
  }

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
  LRESULT App::forwardWindowProcMessage (
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
  ) {
    return onWindowProcMessage(hWnd, message, wParam, lParam);
  }
#endif
}

#if SOCKET_RUNTIME_PLATFORM_ANDROID
extern "C" {
  jlong ANDROID_EXTERNAL(app, App, alloc)(JNIEnv *env, jobject self) {
    if (App::sharedApplication() == nullptr) {
      auto app = new App(env, self);
    }
    return reinterpret_cast<jlong>(App::sharedApplication());
  }

  jboolean ANDROID_EXTERNAL(app, App, setRootDirectory)(
    JNIEnv *env,
    jobject self,
    jstring rootDirectoryString
  ) {
    const auto rootDirectory = Android::StringWrap(env, rootDirectoryString).str();
    setcwd(rootDirectory);
    uv_chdir(rootDirectory.c_str());
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setExternalStorageDirectory)(
    JNIEnv *env,
    jobject self,
    jstring externalStorageDirectoryString
  ) {
    const auto directory = Android::StringWrap(env, externalStorageDirectoryString).str();
    FileResource::setExternalAndroidStorageDirectory(Path(directory));
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setExternalFilesDirectory)(
    JNIEnv *env,
    jobject self,
    jstring externalFilesDirectoryString
  ) {
    const auto directory = Android::StringWrap(env, externalFilesDirectoryString).str();
    FileResource::setExternalAndroidFilesDirectory(Path(directory));
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setExternalCacheDirectory)(
    JNIEnv *env,
    jobject self,
    jstring externalCacheDirectoryString
  ) {
    const auto directory = Android::StringWrap(env, externalCacheDirectoryString).str();
    FileResource::setExternalAndroidCacheDirectory(Path(directory));
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setAssetManager)(
    JNIEnv *env,
    jobject self,
    jobject assetManager
  ) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    if (!assetManager) {
      ANDROID_THROW(env, "'assetManager' object is null");
      return false;
    }

    // `Core::FS` and `FileResource` will use the asset manager
    // when looking for file resources, the asset manager will
    // take precedence
    FileResource::setSharedAndroidAssetManager(AAssetManager_fromJava(env, assetManager));
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setMimeTypeMap)(
    JNIEnv *env,
    jobject self,
    jobject mimeTypeMap
  ) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    if (!mimeTypeMap) {
      ANDROID_THROW(env, "'mimeTypeMap' object is null");
      return false;
    }

    Android::initializeMimeTypeMap(env->NewGlobalRef(mimeTypeMap), app->jvm);
    return true;
  }

  void ANDROID_EXTERNAL(app, App, setBuildInformation)(
    JNIEnv *env,
    jobject self,
    jstring brandString,
    jstring deviceString,
    jstring fingerprintString,
    jstring hardwareString,
    jstring modelString,
    jstring manufacturerString,
    jstring productString
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->androidBuildInformation.brand = Android::StringWrap(env, brandString).str();
    app->androidBuildInformation.device = Android::StringWrap(env, deviceString).str();
    app->androidBuildInformation.fingerprint = Android::StringWrap(env, fingerprintString).str();
    app->androidBuildInformation.hardware = Android::StringWrap(env, hardwareString).str();
    app->androidBuildInformation.model = Android::StringWrap(env, modelString).str();
    app->androidBuildInformation.manufacturer = Android::StringWrap(env, manufacturerString).str();
    app->androidBuildInformation.product = Android::StringWrap(env, productString).str();
    app->isAndroidEmulator = (
      (
        app->androidBuildInformation.brand.starts_with("generic") &&
        app->androidBuildInformation.device.starts_with("generic")
      ) ||
      app->androidBuildInformation.fingerprint.starts_with("generic") ||
      app->androidBuildInformation.fingerprint.starts_with("unknown") ||
      app->androidBuildInformation.hardware.find("goldfish") != String::npos ||
      app->androidBuildInformation.hardware.find("ranchu") != String::npos ||
      app->androidBuildInformation.model.find("google_sdk") != String::npos ||
      app->androidBuildInformation.model.find("Emulator") != String::npos ||
      app->androidBuildInformation.model.find("Android SDK built for x86") != String::npos ||
      app->androidBuildInformation.manufacturer.find("Genymotion") != String::npos ||
      app->androidBuildInformation.product.find("sdk_google") != String::npos ||
      app->androidBuildInformation.product.find("google_sdk") != String::npos ||
      app->androidBuildInformation.product.find("sdk") != String::npos ||
      app->androidBuildInformation.product.find("sdk_x86") != String::npos ||
      app->androidBuildInformation.product.find("sdk_gphone64_arm64") != String::npos ||
      app->androidBuildInformation.product.find("vbox86p") != String::npos ||
      app->androidBuildInformation.product.find("emulator") != String::npos ||
      app->androidBuildInformation.product.find("simulator") != String::npos
    );
  }

  void ANDROID_EXTERNAL(app, App, setWellKnownDirectories)(
    JNIEnv *env,
    jobject self,
    jstring downloadsString,
    jstring documentsString,
    jstring picturesString,
    jstring desktopString,
    jstring videosString,
    jstring configString,
    jstring mediaString,
    jstring musicString,
    jstring homeString,
    jstring dataString,
    jstring logString,
    jstring tmpString
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    FileResource::WellKnownPaths defaults;
    defaults.downloads = Path(Android::StringWrap(env, downloadsString).str());
    defaults.documents = Path(Android::StringWrap(env, documentsString).str());
    defaults.pictures = Path(Android::StringWrap(env, picturesString).str());
    defaults.desktop = Path(Android::StringWrap(env, desktopString).str());
    defaults.videos = Path(Android::StringWrap(env, videosString).str());
    defaults.config = Path(Android::StringWrap(env, configString).str());
    defaults.media = Path(Android::StringWrap(env, mediaString).str());
    defaults.music = Path(Android::StringWrap(env, musicString).str());
    defaults.home = Path(Android::StringWrap(env, homeString).str());
    defaults.data = Path(Android::StringWrap(env, dataString).str());
    defaults.log = Path(Android::StringWrap(env, logString).str());
    defaults.tmp = Path(Android::StringWrap(env, tmpString).str());
    FileResource::WellKnownPaths::setDefaults(defaults);
  }

  jstring ANDROID_EXTERNAL(app, App, getUserConfigValue)(
    JNIEnv *env,
    jobject self,
    jstring keyString
  ) {
    const auto app = App::sharedApplication();
    const auto key = Android::StringWrap(env, keyString).str();
    const auto value = env->NewStringUTF(
      app->userConfig.contains(key)
        ? app->userConfig.at(key).c_str()
        : ""
    );

    return value;
  }

  jboolean ANDROID_EXTERNAL(app, App, hasRuntimePermission)(
    JNIEnv *env,
    jobject self,
    jstring permissionString
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    const auto permission = Android::StringWrap(env, permissionString).str();
    return app->hasRuntimePermission(permission);
  }

  jboolean ANDROID_EXTERNAL(app, App, isDebugEnabled)(
    JNIEnv *env,
    jobject self
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    return isDebugEnabled();
  }

  void ANDROID_EXTERNAL(app, App, onCreateAppActivity)(
    JNIEnv *env,
    jobject self,
    jobject activity
  ) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    if (app->activity) {
      app->jni->DeleteGlobalRef(app->activity);
    }

    app->activity = env->NewGlobalRef(activity);
    app->core->platform.configureAndroidContext(
      Android::JVMEnvironment(app->jni),
      app->activity
    );

    app->run();

    if (app->windowManager.getWindowStatus(0) == WindowManager::WINDOW_NONE) {
      auto windowManagerOptions = WindowManagerOptions {};

      for (const auto& arg : split(app->userConfig["ssc_argv"], ',')) {
        if (arg.find("--test") == 0) {
          windowManagerOptions.features.useTestScript = true;
        }

        windowManagerOptions.argv.push_back("'" + trim(arg) + "'");
      }

      windowManagerOptions.userConfig = app->userConfig;

      app->windowManager.configure(windowManagerOptions);

      app->dispatch([=]() {
        auto defaultWindow = app->windowManager.createDefaultWindow(Window::Options {
          .shouldExitApplicationOnClose = true
        });

        if (
          app->userConfig["webview_service_worker_mode"] != "hybrid" &&
          app->userConfig["permissions_allow_service_worker"] != "false"
        ) {
          if (app->windowManager.getWindowStatus(SOCKET_RUNTIME_SERVICE_WORKER_CONTAINER_WINDOW_INDEX) == WindowManager::WINDOW_NONE) {
            auto serviceWorkerWindowOptions = Window::Options {};
            auto serviceWorkerUserConfig = app->userConfig;

            serviceWorkerUserConfig["webview_watch_reload"] = "false";
            serviceWorkerWindowOptions.shouldExitApplicationOnClose = false;
            serviceWorkerWindowOptions.index = SOCKET_RUNTIME_SERVICE_WORKER_CONTAINER_WINDOW_INDEX;
            serviceWorkerWindowOptions.headless = Env::get("SOCKET_RUNTIME_SERVICE_WORKER_DEBUG").size() == 0;
            serviceWorkerWindowOptions.userConfig = serviceWorkerUserConfig;
            serviceWorkerWindowOptions.features.useGlobalCommonJS = false;
            serviceWorkerWindowOptions.features.useGlobalNodeJS = false;

            auto serviceWorkerWindow = app->windowManager.createWindow(serviceWorkerWindowOptions);
            app->serviceWorkerContainer.init(&serviceWorkerWindow->bridge);
            serviceWorkerWindow->navigate(
              "socket://" + app->userConfig["meta_bundle_identifier"] + "/socket/service-worker/index.html"
            );

            app->core->setTimeout(256, [=](){
              serviceWorkerWindow->hide();
            });
          }
        }

        if (
          app->userConfig["permissions_allow_service_worker"] != "false" &&
          app->userConfig["webview_service_worker_mode"] == "hybrid"
        ) {
          app->serviceWorkerContainer.init(&defaultWindow->bridge);
        }

        defaultWindow->setTitle(app->userConfig["meta_title"]);

        static const auto port = getDevPort();
        static const auto host = getDevHost();

        if (isDebugEnabled() && port > 0 && host.size() > 0) {
          defaultWindow->navigate(host + ":" + std::to_string(port));
        } else if (app->userConfig["webview_root"].size() != 0) {
          defaultWindow->navigate(
            "socket://" + app->userConfig["meta_bundle_identifier"] + app->userConfig["webview_root"]
          );
        } else {
          defaultWindow->navigate(
            "socket://" + app->userConfig["meta_bundle_identifier"] + "/index.html"
          );
        }
      });
    }
  }

  void ANDROID_EXTERNAL(app, App, onDestroyAppActivity)(
    JNIEnv *env,
    jobject self,
    jobject activity
  ) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    if (app->activity) {
      env->DeleteGlobalRef(app->activity);
      app->activity = nullptr;
      app->core->platform.activity = nullptr;
    }
  }

  void ANDROID_EXTERNAL(app, App, onDestroy)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    if (app->self) {
      env->DeleteGlobalRef(app->self);
    }

    app->jni = nullptr;
    app->self = nullptr;

    app->pause();
  }

  void ANDROID_EXTERNAL(app, App, onStart)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->resume();
  }

  void ANDROID_EXTERNAL(app, App, onStop)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->pause();
  }

  void ANDROID_EXTERNAL(app, App, onResume)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->resume();
  }

  void ANDROID_EXTERNAL(app, App, onPause)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->pause();
  }

  void ANDROID_EXTERNAL(app, App, onPermissionChange)(
    JNIEnv *env,
    jobject self,
    jstring nameString,
    jstring stateString
  ) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    const auto name = Android::StringWrap(env, nameString).str();
    const auto state = Android::StringWrap(env, stateString).str();

    if (name == "geolocation") {
      app->core->geolocation.permissionChangeObservers.dispatch(JSON::Object::Entries {
        {"name", name},
        {"state", state}
      });
    }

    if (name == "notification") {
      app->core->notifications.permissionChangeObservers.dispatch(JSON::Object::Entries {
        {"name", name},
        {"state", state}
      });
    }

    if (name == "camera" || name == "microphone") {
      app->core->mediaDevices.permissionChangeObservers.dispatch(JSON::Object::Entries {
        {"name", name},
        {"state", state}
      });
    }
  }
}
#endif
