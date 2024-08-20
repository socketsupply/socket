#include "../app/app.hh"
#include "window.hh"

namespace SSC {
  WindowManager::WindowManager (SharedPointer<Core> core)
    : core(core),
      windows(SOCKET_RUNTIME_MAX_WINDOWS + SOCKET_RUNTIME_MAX_WINDOWS_RESERVED)
  {}

  WindowManager::~WindowManager () {
    this->destroy();
  }

  void WindowManager::WindowManager::destroy () {
    if (this->destroyed) {
      return;
    }

    this->windows.clear();
    this->destroyed = true;
  }

  void WindowManager::WindowManager::configure (const WindowManagerOptions& options) {
    if (this->destroyed) {
      return;
    }

    this->options = options;
    if (this->options.userConfig.size() == 0) {
      this->options.userConfig = getUserConfig();
    }
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::WindowManager::getWindow (
    int index,
    WindowStatus status
  ) {
    Lock lock(this->mutex);

    if (this->destroyed || index < 0 || index >= this->windows.size()) {
      return nullptr;
    }

    if (
      this->getWindowStatus(index) > WindowStatus::WINDOW_NONE &&
      this->getWindowStatus(index) < status &&
      this->windows[index] != nullptr
    ) {
      return this->windows[index];
    }

    return nullptr;
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::WindowManager::getWindow (int index) {
    return this->getWindow(index, WindowStatus::WINDOW_EXITING);
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::getWindowForBridge (
    const IPC::Bridge* bridge
  ) {
    for (const auto& window : this->windows) {
      if (window != nullptr && &window->bridge == bridge) {
        return this->getWindow(window->index);
      }
    }
    return nullptr;
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::getWindowForWebView (WebView* webview) {
    for (const auto& window : this->windows) {
      if (window != nullptr && window->webview == webview) {
        return window;
      }
    }
    return nullptr;
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::getWindowForClient (const IPC::Client& client) {
    for (const auto& window : this->windows) {
      if (window != nullptr && window->bridge.client.id == client.id) {
        return this->getWindow(window->index);
      }
    }

    return nullptr;
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::WindowManager::getOrCreateWindow (int index) {
    return this->getOrCreateWindow(index, Window::Options {});
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::WindowManager::getOrCreateWindow (
    int index,
    const Window::Options& options
  ) {
    if (this->destroyed || index < 0 || index >= this->windows.size()) {
      return nullptr;
    }

    if (this->getWindowStatus(index) == WindowStatus::WINDOW_NONE) {
      Window::Options optionsCopy = options;
      optionsCopy.index = index;
      return this->createWindow(optionsCopy);
    }

    return this->getWindow(index);
  }

  WindowManager::WindowStatus WindowManager::getWindowStatus (int index) {
    Lock lock(this->mutex);

    if (this->destroyed || index < 0 || index >= this->windows.size()) {
      return WindowStatus::WINDOW_NONE;
    }

    const auto window = this->windows[index];
    if (window != nullptr) {
      return window->status;
    }

    return WindowStatus::WINDOW_NONE;
  }

  void WindowManager::destroyWindow (int index) {
    Lock lock(this->mutex);

    if (this->destroyed || index < 0 || index >= this->windows.size()) {
      return;
    }

    auto window = this->windows[index];
    if (window != nullptr) {
      window->close();

      this->windows[index] = nullptr;
      if (window->options.shouldExitApplicationOnClose) {
        App::sharedApplication()->dispatch([window]() {
          window->exit(0);
        });
      } else {
        window->kill();
      }
    }
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::createWindow (
    const Window::Options& options
  ) {
    Lock lock(this->mutex);

    if (
      this->destroyed ||
      options.index < 0 ||
      options.index >= this->windows.size()
    ) {
      return nullptr;
    }

    if (this->windows[options.index] != nullptr) {
      return this->windows[options.index];
    }

    auto screen = Window::getScreenSize();

    float width = options.width <= 0
      ? Window::getSizeInPixels(this->options.defaultWidth, screen.width)
      : options.width;
    float height = options.height <= 0
      ? Window::getSizeInPixels(this->options.defaultHeight, screen.height)
      : options.height;
    float minWidth = options.minWidth <= 0
      ? Window::getSizeInPixels(this->options.defaultMinWidth, screen.width)
      : options.minWidth;
    float minHeight = options.minHeight <= 0
      ? Window::getSizeInPixels(this->options.defaultMinHeight, screen.height)
      : options.minHeight;
    float maxWidth = options.maxWidth <= 0
      ? Window::getSizeInPixels(this->options.defaultMaxWidth, screen.width)
      : options.maxWidth;
    float maxHeight = options.maxHeight <= 0
      ? Window::getSizeInPixels(this->options.defaultMaxHeight, screen.height)
      : options.maxHeight;

    Window::Options windowOptions = {
      .minimizable = options.minimizable,
      .maximizable = options.maximizable,
      .resizable = options.resizable,
      .closable = options.closable,
      .frameless = options.frameless,
      .utility = options.utility,
      .shouldExitApplicationOnClose = options.shouldExitApplicationOnClose,
      .maxHeight = maxHeight,
      .minHeight = minHeight,
      .height = height,
      .maxWidth = maxWidth,
      .minWidth = minWidth,
      .width = width,
      .radius = options.radius,
      .margin = options.margin,
      .aspectRatio = options.aspectRatio,
      .titlebarStyle = options.titlebarStyle,
      .windowControlOffsets = options.windowControlOffsets,
      .backgroundColorLight = options.backgroundColorLight,
      .backgroundColorDark = options.backgroundColorDark,
    };

    windowOptions.RUNTIME_PRIMORDIAL_OVERRIDES = options.RUNTIME_PRIMORDIAL_OVERRIDES;
    windowOptions.userScript = options.userScript;
    windowOptions.userConfig = this->options.userConfig;
    windowOptions.headless = options.headless;
    windowOptions.features = options.features;
    windowOptions.debug = isDebugEnabled() || options.debug;
    windowOptions.index = options.index;
    windowOptions.argv = options.argv;

    for (auto const &entry : parseStringList(this->options.userConfig["build_env"])) {
      const auto key = trim(entry);

      if (!Env::has(key)) {
        continue;
      }

      const auto value = decodeURIComponent(Env::get(key));

      windowOptions.env[key] = value;
    }

    if (options.userConfig.contains("build_env")) {
      for (auto const &entry : parseStringList(options.userConfig.at("build_env"))) {
        const auto key = trim(entry);

        if (!Env::has(key)) {
          continue;
        }

        const auto value = decodeURIComponent(Env::get(key));

        windowOptions.env[key] = value;
      }
    }

    for (const auto& tuple : options.userConfig) {
      windowOptions.userConfig[tuple.first] = tuple.second;
    }

    auto window = std::make_shared<ManagedWindow>(*this, this->core, windowOptions);

    window->status = WindowStatus::WINDOW_CREATED;
    window->onExit = this->options.onExit;
    window->onMessage = this->options.onMessage;

    this->windows[options.index] = window;

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
    if (window->options.headless) {
      window->status = WindowStatus::WINDOW_HIDDEN;
    } else {
      window->status = WindowStatus::WINDOW_SHOWN;
    }
    #endif

    return this->windows.at(options.index);
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::createDefaultWindow (const Window::Options& options) {
    static const auto devHost = SSC::getDevHost();
    auto windowOptions = Window::Options {
      .minimizable = options.minimizable,
      .maximizable = options.maximizable,
      .resizable = options.resizable,
      .closable = options.closable,
      .frameless = options.frameless,
      .utility = options.utility,
      .shouldExitApplicationOnClose = true,
      .height = options.height,
      .width = options.width,
      .titlebarStyle = options.titlebarStyle,
      .windowControlOffsets = options.windowControlOffsets,
      .backgroundColorLight = options.backgroundColorLight,
      .backgroundColorDark = options.backgroundColorDark,
    };

    windowOptions.index = 0;
    windowOptions.headless = (
      options.userConfig.contains("build_headless") &&
      options.userConfig.at("build_headless") == "true"
    );

    if (options.userConfig.size() > 0) {
      windowOptions.userConfig = options.userConfig;
    }

    return this->createWindow(windowOptions);
  }

  JSON::Array WindowManager::json (const Vector<int>& indices) {
    auto i = 0;
    JSON::Array result;
    for (auto index : indices) {
      auto window = this->getWindow(index);
      if (window != nullptr) {
        result[i++] = window->json();
      }
    }
    return result;
  }

  bool WindowManager::emit (const String& event, const JSON::Any& json) {
    bool status = false;
    for (const auto& window : this->windows) {
      if (
        window != nullptr &&
        // only "shown" or "hidden" managed windows will
        // have events dispatched to them
        window->status >= WINDOW_HIDDEN &&
        window->status < WINDOW_CLOSING
      ) {
        if (window->emit(event, json)) {
          status = true;
        }
      }
    }

    return status;
  }

  WindowManager::ManagedWindow::ManagedWindow (
    WindowManager &manager,
    SharedPointer<Core> core,
    const Window::Options& options
  ) : index(options.index),
      Window(core, options),
      manager(manager)
  {}

  WindowManager::ManagedWindow::~ManagedWindow () {}

  void WindowManager::ManagedWindow::show () {
    auto index = std::to_string(this->index);
    this->backgroundColor = Color(Window::getBackgroundColor());
    status = WindowStatus::WINDOW_SHOWING;
    Window::show();
    status = WindowStatus::WINDOW_SHOWN;
  }

  void WindowManager::ManagedWindow::hide () {
    auto index = std::to_string(this->index);
    status = WindowStatus::WINDOW_HIDING;
    Window::hide();
    status = WindowStatus::WINDOW_HIDDEN;
  }

  void WindowManager::ManagedWindow::close (int code) {
    if (status < WindowStatus::WINDOW_CLOSING) {
      auto index = std::to_string(this->index);
      status = WindowStatus::WINDOW_CLOSING;
      Window::close(code);
      status = WindowStatus::WINDOW_CLOSED;
    }
  }

  void WindowManager::ManagedWindow::exit (int code) {
    if (status < WindowStatus::WINDOW_EXITING) {
      auto index = std::to_string(this->index);
      status = WindowStatus::WINDOW_EXITING;
      Window::exit(code);
      status = WindowStatus::WINDOW_EXITED;
    }
  }

  void WindowManager::ManagedWindow::kill () {
    if (status < WindowStatus::WINDOW_KILLING) {
      auto index = std::to_string(this->index);
      status = WindowStatus::WINDOW_KILLING;
      Window::kill();
      status = WindowStatus::WINDOW_KILLED;
    }
  }

  JSON::Object WindowManager::ManagedWindow::json () const {
    const auto id = this->bridge.id;
    const auto size = this->getSize();
    const auto index = this->index;
    const auto readyState = String(
      this->readyState == Window::ReadyState::Loading
        ? "loading"
        : this->readyState == Window::ReadyState::Interactive
          ? "interactive"
          : this->readyState == Window::ReadyState::Complete
            ? "complete"
            : "none"
    );

    return JSON::Object::Entries {
      {"id", std::to_string(id)},
      {"index", index},
      {"title", this->getTitle()},
      {"width", size.width},
      {"height", size.height},
      {"status", this->status},
      {"readyState", readyState},
      {"backgroundColor", this->backgroundColor.json()},
      {"position", JSON::Object::Entries {
        {"x", this->position.x},
        {"y", this->position.y}
      }}
    };
  }

  void WindowManager::ManagedWindow::onReadyStateChange (const ReadyState& readyState) {
    if (readyState == ReadyState::Complete) {
      const auto pendingApplicationURLs = this->pendingApplicationURLs;
      this->pendingApplicationURLs.clear();
      for (const auto& url : pendingApplicationURLs) {
        Window::handleApplicationURL(url);
      }
    }
  }

  void WindowManager::ManagedWindow::handleApplicationURL (const String& url) {
    Lock lock(this->mutex);
    this->pendingApplicationURLs.push_back(url);
    if (this->readyState == ReadyState::Complete) {
      const auto pendingApplicationURLs = this->pendingApplicationURLs;
      this->pendingApplicationURLs.clear();
      for (const auto& url : pendingApplicationURLs) {
        Window::handleApplicationURL(url);
      }
    }
  }

  bool WindowManager::ManagedWindow::emit (const String& event, const JSON::Any& json) {
    return this->bridge.emit(event, json);
  }

  void WindowManager::ManagedWindow::setBackgroundColor (int r, int g, int b, float a) {
    this->backgroundColor = Color(r, g, b, a);
    Window::setBackgroundColor(r, g, b, a);
  }

  void WindowManager::ManagedWindow::setBackgroundColor (const String& rgba) {
    this->backgroundColor = Color(rgba);
    Window::setBackgroundColor(rgba);
  }

  void WindowManager::ManagedWindow::setBackgroundColor (const Color& color) {
    this->backgroundColor = color;
    Window::setBackgroundColor(color.str());
  }

  String WindowManager::ManagedWindow::getBackgroundColor () {
    return this->backgroundColor.str();
  }
}
