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

  auto defaultWindow = self.app->windowManager.createDefaultWindow(Window::Options {
     .shouldExitApplicationOnClose = true
    });

  defaultWindow->setTitle(self.app->userConfig["meta_title"]);

  static const auto port = getDevPort();
  static const auto host = getDevHost();

  if (
    self.app->userConfig["webview_service_worker_mode"] != "hybrid" &&
    self.app->userConfig["permissions_allow_service_worker"] != "false"
  ) {
    auto serviceWorkerWindowOptions = Window::Options {};
    auto serviceWorkerUserConfig = self.app->userConfig;
    const auto screen = defaultWindow->getScreenSize();

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
  } else if (self.app->userConfig["webview_service_worker_mode"] == "hybrid") {
    self.app->serviceWorkerContainer.init(&defaultWindow->bridge);
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
  // TODO(@jwerle): what should we do here?
}

- (void) applicationDidBecomeActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    self.app->core->udp.resumeAllSockets();
    self.app->core->runEventLoop();
  });
}

- (void) applicationWillResignActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    self.app->core->stopEventLoop();
    self.app->core->udp.pauseAllSockets();
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
      window->bridge.emit("applicationurl", JSON::Object::Entries {
        { "url", webpageURL.absoluteString.UTF8String}
      });
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

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
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
#endif

  App* App::sharedApplication () {
    return applicationInstance;
  }

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  App::App (JNIEnv* env, jobject self, SharedPointer<Core> core)
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
  App::App (int _, SharedPointer<Core> core)
    : App(core)
  {}

  App::App (SharedPointer<Core> core)
    : userConfig(getUserConfig()),
      core(core),
      windowManager(core),
      serviceWorkerContainer(core)
  {
    if (applicationInstance == nullptr) {
      applicationInstance = this;
    }

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
  #if SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
    gtk_main_quit();
  #elif SOCKET_RUNTIME_PLATFORM_MACOS
    // if not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (!wasLaunchedFromCli) {
      [NSApp terminate: nil];
    }
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    if (isDebugEnabled()) {
      if (w32ShowConsole) {
        HideConsole();
      }
    }
    PostQuitMessage(0);
  #endif
  }

  void App::restart () {
  #if SOCKET_RUNTIME_PLATFORM_LINUX
    // @TODO
  #elif SOCKET_RUNTIME_PLATFORM_MACOS
    // @TODO
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    char filename[MAX_PATH] = "";
    PROCESS_INFORMATION pi;
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    GetModuleFileName(NULL, filename, MAX_PATH);
    CreateProcess(NULL, filename, NULL, NULL, NULL, NULL, NULL, NULL, &si, &pi);
    std::exit(0);
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
      return false;
    }

    return userConfig.at(key) != "false";
  }

  /*
#if SOCKET_RUNTIME_PLATFORM_ANDROID
  bool App::isAndroidPermissionAllowed (const String& permission) {
    const auto attachment = Android::JNIEnvironmentAttachment(
      this->jvm.get(),
      this->jvm.jniVersion
    );

    return CallClassMethodFromAndroidEnvironment(
      this->jni,
      Boolean,
      this->self,
      "checkPermission",
      "("
      "Ljava/lang/String;" // permission name
      ")Z" // Boolean return type
    );
  }
#endif
*/

  void App::exit (int code) {
    if (this->onExit != nullptr) {
      this->onExit(code);
    }
  }

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
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
    static auto userConfig = getUserConfig();
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
    jobject appActivity
  ) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    if (app->appActivity) {
      app->jni->DeleteGlobalRef(app->appActivity);
    }

    app->appActivity = env->NewGlobalRef(appActivity);
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

        defaultWindow->setTitle(app->userConfig["meta_title"]);

        if (
          app->userConfig["webview_service_worker_mode"] != "hybrid" &&
          app->userConfig["permissions_allow_service_worker"] != "false"
        ) {
          if (app->windowManager.getWindowStatus(SOCKET_RUNTIME_SERVICE_WORKER_CONTAINER_WINDOW_INDEX) == WindowManager::WINDOW_NONE) {
            auto serviceWorkerWindowOptions = Window::Options {};
            auto serviceWorkerUserConfig = app->userConfig;
            auto screen = defaultWindow->getScreenSize();

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
          }
        } else if (app->userConfig["webview_service_worker_mode"] == "hybrid") {
          app->serviceWorkerContainer.init(&defaultWindow->bridge);
        }

        defaultWindow->show();

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
    jobject appActivity
  ) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    if (app->appActivity) {
      env->DeleteGlobalRef(app->appActivity);
      app->appActivity = nullptr;
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

    app->core->udp.pauseAllSockets();
    app->core->stopEventLoop();
  }

  void ANDROID_EXTERNAL(app, App, onStart)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->core->udp.resumeAllSockets();
    app->core->runEventLoop();
  }

  void ANDROID_EXTERNAL(app, App, onStop)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->core->udp.pauseAllSockets();
    app->core->stopEventLoop();
  }

  void ANDROID_EXTERNAL(app, App, onResume)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->core->udp.resumeAllSockets();
    app->core->runEventLoop();
  }

  void ANDROID_EXTERNAL(app, App, onPause)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->core->udp.pauseAllSockets();
    app->core->stopEventLoop();
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
        {"state", state}
      });
    }

    if (name == "notification") {
      app->core->notifications.permissionChangeObservers.dispatch(JSON::Object::Entries {
        {"state", state}
      });
    }
  }
}
#endif
