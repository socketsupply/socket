#include "../window/window.hh"
#include "../ipc/ipc.hh"
#include "app.hh"

#if defined(_WIN32)
#include <uxtheme.h>
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
  "co.socketsupply.queue.app",
  qos
);
#endif

namespace SSC {
  App::App () {
    this->core = new Core();
  }

  App::App (int) : App() {
#if defined(__linux__)
    gtk_init_check(0, nullptr);
#endif
  }

  int App::run () {
    auto cwd = getCwd();
    uv_chdir(cwd.c_str());

#if defined(__linux__)
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

    return shouldExit ? 1 : 0;
  }

  void App::kill () {
    // Distinguish window closing with app exiting
    shouldExit = true;
#if defined(__linux__)
    gtk_main_quit();
#elif defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    // if not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (!fromSSC) {
      [NSApp terminate:nil];
    }
#elif defined(_WIN32)
    PostQuitMessage(0);
#endif
  }

  void App::restart () {
#if defined(__linux__)
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
#if defined(__linux__)
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
    auto future = std::async(std::launch::async, [&] {
      auto threadCallback = (LPARAM) new std::function<void()>(callback);

      while (!this->isReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }

      PostThreadMessage(mainThread, WM_APP, 0, threadCallback);
    });

    future.get();
#endif
  }

  String App::getCwd () {
    String cwd = "";

#if defined(__linux__)
    auto canonical = fs::canonical("/proc/self/exe");
    cwd = fs::path(canonical).parent_path().string();
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

  void App::setWindowManager (void *windowManager) {
    this->windowManager = windowManager;
  }

  void * App::getWindowManager () const {
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
    MessageBoxA(nullptr, SSC::WStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const SSC::String &s) {
    MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const char* s) {
    MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  App::App (void* h) : App() {
    this->hInstance = (HINSTANCE) h;

    if (isDebugEnabled()) {
      #if DEBUG == 1
        AllocConsole();
        freopen_s(&console, "CONOUT$", "w", stdout);
      #endif
    }

    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    SSC::setWindowCompositionAttribute = reinterpret_cast<SSC::SetWindowCompositionAttribute>(GetProcAddress(
      GetModuleHandleW(L"user32.dll"),
      "SetWindowCompositionAttribute")
    );

    if (hUxtheme) {
      refreshImmersiveColorPolicyState = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));
      shouldSystemUseDarkMode = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(138));
      allowDarkModeForApp = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
    }

    allowDarkModeForApp(shouldSystemUseDarkMode());
    refreshImmersiveColorPolicyState();

    // this fixes bad default quality DPI.
    SetProcessDPIAware();

    auto iconPath = fs::path { getCwd() / fs::path { "index.ico" } };

    HICON icon = (HICON) LoadImageA(
      NULL,
      iconPath.string().c_str(),
      IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON),
      GetSystemMetrics(SM_CXSMICON),
      LR_LOADFROMFILE
    );

    auto *szWindowClass = L"DesktopApp";
    auto *szTitle = L"Socket";

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = bgBrush;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = TEXT("DesktopApp");
    wcex.hIconSm = icon; // ico doesn't auto scale, needs 16x16 icon lol fuck you bill
    wcex.hIcon = icon;
    wcex.lpfnWndProc = Window::WndProc;

    if (!RegisterClassEx(&wcex)) {
      alert("Application could not launch, possible missing resources.");
    }
  };
}
#endif // _WIN32
