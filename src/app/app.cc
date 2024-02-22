#include "../core/platform.hh"
#include "../window/window.hh"
#include "../ipc/ipc.hh"
#include "app.hh"

#if defined(_WIN32)
#include <uxtheme.h>
#include <thread>
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Gdi32.lib")
#endif

#if defined(__APPLE__)
static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create(
  "socket.runtime.app.queue",
  qos
);

#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
@implementation SSCApplicationDelegate
- (void) applicationDidFinishLaunching: (NSNotification*) notification {
  self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
}

- (void) menuWillOpen: (NSMenu*) menu {
  auto app = self.app;
  auto w = app->windowManager->getWindow(0)->window;
  if (!w) return;

  [w makeKeyAndOrderFront: nil];
  [NSApp activateIgnoringOtherApps:YES];

  if (app != nullptr && app->windowManager != nullptr) {
    for (auto window : self.app->windowManager->windows) {
      if (window != nullptr) window->bridge->router.emit("tray", "true");
    }
  }
}

- (void) application: (NSApplication*) application openURLs: (NSArray<NSURL*>*) urls {
  auto app = self.app;
  if (app != nullptr && app->windowManager != nullptr) {
    for (NSURL* url in urls) {
      SSC::JSON::Object json = SSC::JSON::Object::Entries {{
        "url", [url.absoluteString UTF8String]
      }};

      for (auto window : self.app->windowManager->windows) {
        if (window != nullptr) {
          window->bridge->router.emit("applicationurl", json.str());
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
  auto webpageURL = application.userActivity.webpageURL;

  if (userActivityType == nullptr) {
    return NO;
  }

  if (webpageURL == nullptr) {
    return NO;
  }

  if (webpageURL.host == nullptr) {
    return NO;
  }

  auto activityType = SSC::String(userActivityType.UTF8String);

  if (activityType != SSC::String(NSUserActivityTypeBrowsingWeb.UTF8String)) {
    return NO;
  }

  auto host = SSC::String(webpageURL.host.UTF8String);
  auto links = SSC::parseStringList(userConfig["meta_application_links"], ' ');

  if (links.size() == 0) {
    return NO;
  }

  bool exists = false;

  for (const auto& link : links) {
    const auto parts = SSC::split(link, '?');
    if (host == parts[0]) {
      exists = true;
      break;
    }
  }

  if (!exists) {
    return NO;
  }

  auto url = SSC::String(webpageURL.absoluteString.UTF8String);

  SSC::JSON::Object json = SSC::JSON::Object::Entries {{ "url", url }};

  bool emitted = false;

  for (auto window : self.app->windowManager->windows) {
    if (window != nullptr) {
      window->bridge->router.emit("applicationurl", json.str());
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
@end
#endif
#endif

namespace SSC {
#if defined(_WIN32)
  FILE* console;
#endif
  static App* applicationInstance = nullptr;

  App* App::instance () {
    return SSC::applicationInstance;
  }

  App::App () {
    if (applicationInstance == nullptr) {
      SSC::applicationInstance = this;
    }

    this->core = new Core();
    auto cwd = getcwd();
    uv_chdir(cwd.c_str());
  }

  App::App (int) : App() {
  #if defined(__linux__) && !defined(__ANDROID__)
    gtk_init_check(0, nullptr);
  #elif defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    this->delegate.app = this;
    NSApplication.sharedApplication.delegate = this->delegate;
  #endif
  }

  App::~App () {
    if (applicationInstance == this) {
      applicationInstance = nullptr;
    }
  }

  int App::run () {
  #if defined(__linux__) && !defined(__ANDROID__)
    gtk_main();
  #elif defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    [NSApp run];
  #elif defined(_WIN32)
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
      auto callback = (std::function<void()> *)(msg.lParam);
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
  #if defined(__linux__) && !defined(__ANDROID__)
    gtk_main_quit();
  #elif defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    // if not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (!fromSSC) {
      [NSApp terminate:nil];
    }
  #elif defined(_WIN32)
    if (isDebugEnabled()) {
      if (w32ShowConsole) {
        HideConsole();
      }
    }
    PostQuitMessage(0);
  #endif
  }

  void App::restart () {
  #if defined(__linux__) && !defined(__ANDROID__)
    // @TODO
  #elif defined(__APPLE__)
    // @TODO
  #elif defined(_WIN32)
    char filename[MAX_PATH] = "";
    PROCESS_INFORMATION pi;
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    GetModuleFileName(NULL, filename, MAX_PATH);
    CreateProcess(NULL, filename, NULL, NULL, NULL, NULL, NULL, NULL, &si, &pi);
    std::exit(0);
  #endif
  }

  void App::dispatch (std::function<void()> callback) {
  #if defined(__linux__) && !defined(__ANDROID__)
    auto threadCallback = new std::function<void()>(callback);

    g_idle_add_full(
      G_PRIORITY_HIGH_IDLE,
      (GSourceFunc)([](void* callback) -> int {
        (*static_cast<std::function<void()>*>(callback))();
        return G_SOURCE_REMOVE;
      }),
      threadCallback,
      [](void* callback) {
        delete static_cast<std::function<void()>*>(callback);
      }
    );
  #elif defined(__APPLE__)
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        callback();
      });
    });
  #elif defined(_WIN32)
    static auto mainThread = GetCurrentThreadId();
    auto threadCallback = (LPARAM) new std::function<void()>(callback);
    if (this->isReady) {
      PostThreadMessage(mainThread, WM_APP, 0, threadCallback);
      return;
    }
    std::thread t([&, threadCallback] {

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
    static String cwd = "";

    #if defined(__linux__) && !defined(__ANDROID__)
      try {
        auto canonical = fs::canonical("/proc/self/exe");
        cwd = fs::path(canonical).parent_path().string();
      } catch (...) {}
    #elif defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
      cwd = [bundlePath UTF8String];
    #elif defined(_WIN32)
      wchar_t filename[MAX_PATH];
      GetModuleFileNameW(NULL, filename, MAX_PATH);
      auto path = fs::path { filename }.remove_filename();
      cwd = path.string();
    #endif

    return cwd;
  }

  void App::setWindowManager (WindowManager* windowManager) {
    this->windowManager = windowManager;
  }

  WindowManager* App::getWindowManager () const {
    return this->windowManager;
  }

  void App::exit (int code) {
    if (this->onExit != nullptr) {
      this->onExit(code);
    }
  }
}

#if defined(_WIN32)

namespace SSC {
  static inline void alert (const SSC::WString &ws) {
    MessageBoxA(nullptr, SSC::convertWStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const SSC::String &s) {
    MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const char* s) {
    MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  void App::ShowConsole() {
    if (consoleVisible)
      return;
    consoleVisible = true;
    AllocConsole();
    freopen_s(&console, "CONOUT$", "w", stdout);
  }

  void App::HideConsole() {
    if (!consoleVisible)
      return;
    consoleVisible = false;
    fclose(console);
    FreeConsole();
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
}
#endif // _WIN32
