#include "app.hh"

using namespace SSC;

#if SSC_PLATFORM_APPLE
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
#if SSC_PLATFORM_MACOS
- (void) applicationDidFinishLaunching: (NSNotification*) notification {
  self.statusItem = [NSStatusBar.systemStatusBar statusItemWithLength: NSVariableStatusItemLength];
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
      JSON::Object json = JSON::Object::Entries {{
        "url", [url.absoluteString UTF8String]
      }};

      for (auto& window : self.app->windowManager.windows) {
        if (window != nullptr) {
          window->bridge.emit("applicationurl", json.str());
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
  static auto userConfig = SSC::getUserConfig();
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
  const auto json = JSON::Object::Entries {{ "url", url }};

  bool emitted = false;

  for (auto& window : self.app->windowManager.windows) {
    if (window != nullptr) {
      window->bridge.emit("applicationurl", json);
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

#elif SSC_PLATFORM_IOS
-           (BOOL) application: (UIApplication*) application
 didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{
  auto const notificationCenter = NSNotificationCenter.defaultCenter;
  const auto frame = UIScreen.mainScreen.bounds;

  Vector<String> argv;
  StringStream env;
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

  env << String("width=" + std::to_string(frame.size.width) + "&");
  env << String("height=" + std::to_string(frame.size.height) + "&");

  self.app->windowManager.configure(WindowManagerOptions {
    .isTest = isTest,
    .argv = join(argv, ','),
    .cwd = self.app->getcwd(),
    .userConfig = self.app->userConfig,
  });

  const auto defaultWindow = self.app->windowManager.createDefaultWindow(WindowOptions {
    .canExit = true,
    .title = self.app->userConfig["meta_title"],
    .env = env.str()
  });

  static const auto port = getDevPort();
  static const auto host = getDevHost();

  if (
    self.app->userConfig["webview_service_worker_mode"] != "hybrid" &&
    self.app->userConfig["permissions_allow_service_worker"] != "false"
  ) {
    const auto screen = defaultWindow->getScreenSize();
    auto serviceWorkerUserConfig = self.app->userConfig;
    serviceWorkerUserConfig["webview_watch_reload"] = "false";
    auto serviceWorkerWindow = self.app->windowManager.createWindow({
      .canExit = false,
      .index = SSC_SERVICE_WORKER_CONTAINER_WINDOW_INDEX,
      .headless = Env::get("SOCKET_RUNTIME_SERVICE_WORKER_DEBUG").size() == 0,
      .env = env.str(),
      .userConfig = serviceWorkerUserConfig,
      .preloadCommonJS = false
    });

    self.app->core->serviceWorker.init(&serviceWorkerWindow->bridge);

    serviceWorkerWindow->navigate(
      "socket://" + self.app->userConfig["meta_bundle_identifier"] + "/socket/service-worker/index.html"
    );
  } else if (self.app->userConfig["webview_service_worker_mode"] == "hybrid") {
    self.app->core->serviceWorker.init(&defaultWindow->bridge);
  }

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
  debug("applicationDidEnterBackground");
  for (const auto& window : self.app->windowManager.windows) {
    if (window != nullptr) {
      window->eval("window.blur()");
    }
  }
}

- (void) applicationWillEnterForeground: (UIApplication*) application {
  debug("applicationWillEnterForeground");
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
  debug("applicationWillTerminate");
  // TODO(@jwerle): what should we do here?
}

- (void) applicationDidBecomeActive: (UIApplication*) application {
  debug("applicationDidBecomeActive");
  dispatch_async(queue, ^{
    self.app->core->resumeAllPeers();
    self.app->core->runEventLoop();
  });
}

- (void) applicationWillResignActive: (UIApplication*) application {
  debug("applicationWillResignActive");
  dispatch_async(queue, ^{
    self.app->core->stopEventLoop();
    self.app->core->pauseAllPeers();
  });
}

-  (BOOL) application: (UIApplication*) application
 continueUserActivity: (NSUserActivity*) userActivity
   restorationHandler: (void (^)(NSArray<id<UIUserActivityRestoring>>*)) restorationHandler
{
  debug("application:continueUserActivity:restorationHandler");
  return [self
                         application: application
    willContinueUserActivityWithType: userActivity.activityType
  ];
}

              - (BOOL) application: (UIApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType
{
  debug("application:willContinueUserActivityWithType");
  static auto userConfig = SSC::getUserConfig();
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
      window->bridge.emit("applicationurl", JSON::Object::Entries {
        { "url", webpageURL.absoluteString.UTF8String}
      });
      emitted = true;
    }
  }

  return emitted;
}

- (void) touchesBegan: (NSSet<UITouch*>*) touches
            withEvent: (UIEvent*) event
{
  debug("touchesBegan:withEvent:");
}

- (void) touchesMoved: (NSSet<UITouch*>*) touches
            withEvent: (UIEvent*) event
{
  debug("touchesMoved:withEvent:");
}

- (void) touchesEnded: (NSSet<UITouch*>*) touches
           withEvent: (UIEvent*) event
{
  debug("touchesEnded:withEvent:");
}

- (void) touchesCancelled: (NSSet<UITouch*>*) touches
               withEvent: (UIEvent*) event
{
  debug("touchesCancelled:withEvent:");
}

- (void) keyboardWillHide: (NSNotification*) notification {
  debug("keyboardWillHide");
  for (const auto window : self.app->windowManager.windows) {
    if (window) {
      const auto info = notification.userInfo;
      const auto keyboardFrameBegin = (NSValue*) [info valueForKey: UIKeyboardFrameEndUserInfoKey];
      const auto rect = [keyboardFrameBegin CGRectValue];
      const auto height = rect.size.height;

      window->webview.scrollView.scrollEnabled = YES;
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
  debug("keyboardDidHide");
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

- (void) keyboardWillShow: (NSNotification*) notification {
  debug("keyboardWillShow");
  for (const auto window : self.app->windowManager.windows) {
    if (window && !window->window.isHidden) {
      const auto info = notification.userInfo;
      const auto keyboardFrameBegin = (NSValue*) [info valueForKey: UIKeyboardFrameEndUserInfoKey];
      const auto rect = [keyboardFrameBegin CGRectValue];
      const auto height = rect.size.height;

      window->webview.scrollView.scrollEnabled = NO;
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
  debug("keyboardDidShow");
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
  debug("keyboardWillChange");
  for (const auto window : self.app->windowManager.windows) {
    if (window && !window->window.isHidden) {
      const auto keyboardInfo = notification.userInfo;
      const auto keyboardFrameBegin = (NSValue* )[keyboardInfo valueForKey: UIKeyboardFrameEndUserInfoKey];
      const auto rect = [keyboardFrameBegin CGRectValue];
      const auto width = rect.size.width;
      const auto height = rect.size.height;

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
      // TODO can this be escaped or is the url encoded property already?
      return window->bridge.emit("applicationurl", JSON::Object::Entries {
        {"url", url.absoluteString.UTF8String}
      });
    }
  }

  return NO;
}
#endif
@end
#endif

namespace SSC {
  static App* applicationInstance = nullptr;

#if SSC_PLATFORM_WINDOWS
  static FILE* console = nullptr;

  static inline void alert (const SSC::WString &ws) {
    MessageBoxA(nullptr, SSC::convertWStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const SSC::String &s) {
    MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const char* s) {
    MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }
#endif

  App* App::sharedApplication () {
    return SSC::applicationInstance;
  }

  App::App (SharedPointer<Core> core)
    : core(core),
      windowManager(core)
  {
    if (applicationInstance == nullptr) {
      SSC::applicationInstance = this;
    }

    const auto cwd = getcwd();
    uv_chdir(cwd.c_str());

  #if SSC_PLATFORM_LINUX
    gtk_init_check(0, nullptr);
  #elif SSC_PLATFORM_MACOS
    this->applicationDelegate = [SSCApplicationDelegate new];
    this->applicationDelegate.app = this;
    NSApplication.sharedApplication.delegate = this->applicationDelegate;
  #endif
  }

  App::App (int _, SharedPointer<Core> core)
    : App(core)
  {}

  App::~App () {
    if (applicationInstance == this) {
      applicationInstance = nullptr;
    }
  }

  int App::run (int argc, char** argv) {
  #if SSC_PLATFORM_LINUX
    gtk_main();
  #elif SSC_PLATFORM_MACOS
    [NSApp run];
  #elif SSC_PLATFORM_IOS
    @autoreleasepool {
      return UIApplicationMain(
        argc,
        argv,
        nullptr,
        NSStringFromClass(SSCApplicationDelegate.class)
      );
    }
  #elif SSC_PLATFORM_WINDOWS
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
      this->core->shuttingDown = true;
    }

    return shouldExit ? 1 : 0;
  }

  void App::kill () {
    this->killed = true;
    this->core->shuttingDown = true;
    this->core->shutdown();
    // Distinguish window closing with app exiting
    shouldExit = true;
  #if SSC_PLATFORM_LINUX
    gtk_main();
  #elif SSC_PLATFORM_MACOS
    // if not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (!wasLaunchedFromCli) {
      [NSApp terminate: nil];
    }
  #elif SSC_PLATFORM_WINDOWS
    if (isDebugEnabled()) {
      if (w32ShowConsole) {
        HideConsole();
      }
    }
    PostQuitMessage(0);
  #endif
  }

  void App::restart () {
  #if SSC_PLATFORM_LINUX
    // @TODO
  #elif SSC_PLATFORM_MACOS
    // @TODO
  #elif SSC_PLATFORM_WINDOWS
    char filename[MAX_PATH] = "";
    PROCESS_INFORMATION pi;
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    GetModuleFileName(NULL, filename, MAX_PATH);
    CreateProcess(NULL, filename, NULL, NULL, NULL, NULL, NULL, NULL, &si, &pi);
    std::exit(0);
  #endif
  }

  void App::dispatch (Function<void()> callback) {
  #if SSC_PLATFORM_LINUX
    // @TODO
    auto threadCallback = new Function<void()>(callback);

    g_idle_add_full(
      G_PRIORITY_HIGH_IDLE,
      (GSourceFunc)([](void* callback) -> int {
        (*static_cast<Function<void()>*>(callback))();
        return G_SOURCE_REMOVE;
      }),
      threadCallback,
      [](void* callback) {
        delete static_cast<Function<void()>*>(callback);
      }
    );
  #elif SSC_PLATFORM_APPLE
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        callback();
      });
    });
  #elif SSC_PLATFORM_WINDOWS
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
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }

      PostThreadMessage(mainThread, WM_APP, 0, threadCallback);
    });

    t.detach();
  #endif
  }

  String App::getcwd () {
    return SSC::getcwd();
  }

  void App::exit (int code) {
    if (this->onExit != nullptr) {
      this->onExit(code);
    }
  }

#if SSC_PLATFORM_WINDOWS
  void App::ShowConsole () {
    if (!isConsoleVisible) {
      isConsoleVisible = true;
      AllocConsole();
      freopen_s(&console, "CONOUT$", "w", stdout);
    }
  }

  void App::HideConsole () {
    if (isConsoleVisible) {
      isConsoleVisible = false;
      fclose(console);
      FreeConsole();
    }
  }

  App::App (void* h) : App() {
    static auto userConfig = SSC::getUserConfig();
    this->hInstance = (HINSTANCE) h;

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
    wcex.lpfnWndProc = Window::WndProc;

    if (!RegisterClassEx(&wcex)) {
      alert("Application could not launch, possible missing resources.");
    }
  };
#endif
}
