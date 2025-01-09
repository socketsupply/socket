#include "../cwd.hh"
#include "../app.hh"

namespace ssc::runtime::app {
  static SharedPointer<App> sharedApplicationInstance = nullptr;

  SharedPointer<App> App::sharedApplication () {
    return sharedApplicationInstance;
  }

  void releaseApplication () {
    sharedApplicationInstance = nullptr;
  }

  App::App (const Options& options)
    : runtime({
        .userConfig = options.userConfig,
        .loop = options.loop,
        .features = options.features
      })
  {
    if (sharedApplicationInstance == nullptr) {
      sharedApplicationInstance.reset(this);
    }

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    this->instance = options.instanceId;
    registerWindowClass(this);
  #endif

  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    this->runtime.android.jvm = options.env;
    this->runtime.android.self = options.self;
  #endif

  #if !SOCKET_RUNTIME_DESKTOP_EXTENSION
    const auto cwd = getcwd();
    uv_chdir(cwd.c_str());
  #endif

    this->init();
  }

  App::~App () {}

  void App::init () {
  #if SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_PLATFORM_LINUX
    gtk_init_check(0, nullptr);
  #elif SOCKET_RUNTIME_PLATFORM_MACOS
    this->delegate = [SSCApplicationDelegate new];
    this->delegate.app = App::sharedApplication();
    NSApplication.sharedApplication.delegate = this->delegate;
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    OleInitialize(nullptr);
  #endif
  }

  int App::run (int argc, char** argv) {
    this->isStarted = true;
  #if SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
    gtk_main();
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    // MUST be acquired on "main" thread
    // `run()` should called when the main activity is created
    if (!this->runtime.android.looper.acquired()) {
      this->runtime.android.looper.acquire();
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
      this->shouldExit = true;
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

    if (msg.message == WM_QUIT && this->shouldExit) {
      return 1;
    }
  #endif

    return this->shouldExit ? 1 : 0;
  }

  void App::resume () {
    if (this->paused()) {
      this->isPaused = false;
      this->runtime.windowManager.emit("applicationresume");

      this->dispatch([this]() {
        this->runtime.resume();
      });
    }
  }

  void App::pause () {
    if (!this->paused()) {
      this->isPaused = true;
      this->runtime.windowManager.emit("applicationpause");

      this->dispatch([this]() {
        this->runtime.pause();
      });
    }
  }

  void App::stop () {
    if (this->stopped()) {
      return;
    }

    this->isStarted = false;
    this->isStopped = true;

    this->runtime.windowManager.emit("applicationstop");
    msleep(256);
    this->pause();

    this->shouldExit = true;
    this->isPaused = false;

    this->runtime.destroy();

  #if SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
    gtk_main_quit();
  #elif SOCKET_RUNTIME_PLATFORM_MACOS
    // if not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (this->launchSource == App::LaunchSource::Platform) {
      [NSApp terminate: nil];
    }
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    PostQuitMessage(0);
  #endif
  }

  bool App::paused () const {
    return this->isPaused.load(std::memory_order_relaxed);
  }

  bool App::started () const {
    return this->isStarted.load(std::memory_order_relaxed);
  }

  bool App::stopped () const {
    return this->isStopped.load(std::memory_order_relaxed);
  }

  bool App::hasRuntimePermission (const String& permission) const {
    return this->runtime.hasPermission(permission);
  }

  void App::dispatch (Function<void()> callback) {
    this->runtime.dispatch(callback);
  }
}
