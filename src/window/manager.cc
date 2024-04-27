#include "window.hh"

namespace SSC {
  WindowManager::WindowManager (App &app)
    : app(app),
      windows(SSC_MAX_WINDOWS + SSC_MAX_WINDOWS_RESERVED)
  {
    if (isDebugEnabled()) {
      this->lastDebugLogLine = std::chrono::system_clock::now();
    }
  }

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

  void WindowManager::WindowManager::configure (const WindowManagerOptions& configuration) {
    if (this->destroyed) {
      return;
    }

    this->options.defaultHeight = configuration.defaultHeight;
    this->options.defaultWidth = configuration.defaultWidth;
    this->options.defaultMinWidth = configuration.defaultMinWidth;
    this->options.defaultMinHeight = configuration.defaultMinHeight;
    this->options.defaultMaxWidth = configuration.defaultMaxWidth;
    this->options.defaultMaxHeight = configuration.defaultMaxHeight;
    this->options.onMessage = configuration.onMessage;
    this->options.userConfig = configuration.userConfig;
    this->options.onExit = configuration.onExit;
    this->options.aspectRatio = configuration.aspectRatio;
    this->options.isTest = configuration.isTest;
    this->options.argv = configuration.argv;
    this->options.cwd = configuration.cwd;
    this->options.userConfig = getUserConfig();
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

  SharedPointer<WindowManager::ManagedWindow> WindowManager::WindowManager::getOrCreateWindow (int index) {
    return this->getOrCreateWindow(index, WindowOptions {});
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::WindowManager::getOrCreateWindow (
    int index,
    const WindowOptions& options
  ) {
    if (this->destroyed || index < 0 || index >= this->windows.size()) {
      return nullptr;
    }

    if (this->getWindowStatus(index) == WindowStatus::WINDOW_NONE) {
      WindowOptions opts = options;
      opts.index = index;
      return this->createWindow(opts);
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
      if (window->status < WINDOW_CLOSING) {
        window->close(0);
      }

      if (window->status < WINDOW_KILLING) {
        window->kill();
      }

      this->windows[index] = nullptr;
    }
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::createWindow (const WindowOptions& options) {
    Lock lock(this->mutex);

    if (this->destroyed || options.index < 0 || options.index >= this->windows.size()) {
      return nullptr;
    }

    if (this->windows[options.index] != nullptr) {
      return this->windows[options.index];
    }

    StringStream env;

    if (options.userConfig.size() > 0) {
      if (options.userConfig.contains("build_env")) {
        for (auto const &envKey : parseStringList(options.userConfig.at("build_env"))) {
          auto cleanKey = trim(envKey);

          if (!Env::has(cleanKey)) {
            continue;
          }

          auto envValue = Env::get(cleanKey.c_str());

          env << String(
              cleanKey + "=" + encodeURIComponent(envValue) + "&"
              );
        }
      }
    } else {
      for (auto const &envKey : parseStringList(this->options.userConfig["build_env"])) {
        auto cleanKey = trim(envKey);

        if (!Env::has(cleanKey)) {
          continue;
        }

        auto envValue = Env::get(cleanKey);

        env << String(
          cleanKey + "=" + encodeURIComponent(envValue) + "&"
        );
      }
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

    WindowOptions windowOptions = {
      .resizable = options.resizable,
      .minimizable = options.minimizable,
      .maximizable = options.maximizable,
      .closable = options.closable,
      .frameless = options.frameless,
      .utility = options.utility,
      .canExit = options.canExit,
      .width = width,
      .height = height,
      .minWidth = minWidth,
      .minHeight = minHeight,
      .maxWidth = maxWidth,
      .maxHeight = maxHeight,
      .radius = options.radius,
      .margin = options.margin,
      .index = options.index,
      .debug = isDebugEnabled() || options.debug,
      .isTest = this->options.isTest,
      .headless = options.headless,
      .aspectRatio = options.aspectRatio,
      .titlebarStyle = options.titlebarStyle,
      .windowControlOffsets = options.windowControlOffsets,
      .backgroundColorLight = options.backgroundColorLight,
      .backgroundColorDark = options.backgroundColorDark,
      .cwd = this->options.cwd,
      .title = options.title.size() > 0 ? options.title : "",
      .url = options.url.size() > 0 ? options.url : "data:text/html,<html>",
      .argv = this->options.argv,
      .preload = options.preload.size() > 0 ? options.preload : "",
      .env = env.str(),
      .userConfig = this->options.userConfig,
      .userScript = options.userScript,
      .runtimePrimordialOverrides = options.runtimePrimordialOverrides,
      .preloadCommonJS = options.preloadCommonJS != false
    };

    for (const auto& tuple : options.userConfig) {
      windowOptions.userConfig[tuple.first] = tuple.second;
    }

    if (isDebugEnabled()) {
      this->log("Creating Window#" + std::to_string(options.index));
    }

    auto window = SharedPointer<ManagedWindow>(new ManagedWindow(*this, app, windowOptions));

    window->status = WindowStatus::WINDOW_CREATED;
    window->onExit = this->options.onExit;
    window->onMessage = this->options.onMessage;

    this->windows[options.index] = std::move(window);

    return this->windows.at(options.index);
  }

  SharedPointer<WindowManager::ManagedWindow> WindowManager::createDefaultWindow (const WindowOptions& options) {
    return this->createWindow(WindowOptions {
      .resizable = options.resizable,
      .minimizable = options.minimizable,
      .maximizable = options.maximizable,
      .closable = options.closable,
      .frameless = options.frameless,
      .utility = options.utility,
      .canExit = true,
      .width = options.width,
      .height = options.height,
      .index = 0,
    #ifdef PORT
      .port = PORT,
    #endif
      .headless = (
        options.userConfig.contains("build_headless") &&
        options.userConfig.at("build_headless") == "true"
      ),
      .titlebarStyle = options.titlebarStyle,
      .windowControlOffsets = options.windowControlOffsets,
      .backgroundColorLight = options.backgroundColorLight,
      .backgroundColorDark = options.backgroundColorDark,
      .userConfig = options.userConfig
    });
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

  WindowManager::ManagedWindow::ManagedWindow (
    WindowManager &manager,
    App &app,
    WindowOptions options
  ) : Window(app, options),
      manager(manager)
  {}

  WindowManager::ManagedWindow::~ManagedWindow () {}

  void WindowManager::ManagedWindow::show (const String &seq) {
    auto index = std::to_string(this->opts.index);
    manager.log("Showing Window#" + index + " (seq=" + seq + ")");
    status = WindowStatus::WINDOW_SHOWING;
    Window::show();
    status = WindowStatus::WINDOW_SHOWN;
  }

  void WindowManager::ManagedWindow::hide (const String &seq) {
    if (
      status > WindowStatus::WINDOW_HIDDEN &&
      status < WindowStatus::WINDOW_EXITING
    ) {
      auto index = std::to_string(this->opts.index);
      manager.log("Hiding Window#" + index + " (seq=" + seq + ")");
      status = WindowStatus::WINDOW_HIDING;
      Window::hide();
      status = WindowStatus::WINDOW_HIDDEN;
    }
  }

  void WindowManager::ManagedWindow::close (int code) {
    if (status < WindowStatus::WINDOW_CLOSING) {
      auto index = std::to_string(this->opts.index);
      manager.log("Closing Window#" + index + " (code=" + std::to_string(code) + ")");
      status = WindowStatus::WINDOW_CLOSING;
      Window::close(code);
      if (this->opts.canExit) {
        status = WindowStatus::WINDOW_EXITED;
      } else {
        status = WindowStatus::WINDOW_CLOSED;
      }
    }
  }

  void WindowManager::ManagedWindow::exit (int code) {
    if (status < WindowStatus::WINDOW_EXITING) {
      auto index = std::to_string(this->opts.index);
      manager.log("Exiting Window#" + index + " (code=" + std::to_string(code) + ")");
      status = WindowStatus::WINDOW_EXITING;
      Window::exit(code);
      status = WindowStatus::WINDOW_EXITED;
    }
  }

  void WindowManager::ManagedWindow::kill () {
    if (status < WindowStatus::WINDOW_KILLING) {
      auto index = std::to_string(this->opts.index);
      manager.log("Killing Window#" + index);
      status = WindowStatus::WINDOW_KILLING;
      Window::kill();
      status = WindowStatus::WINDOW_KILLED;
      manager.destroyWindow(this->opts.index);
    }
  }

  JSON::Object WindowManager::ManagedWindow::json () const {
    auto index = this->opts.index;
    auto size = this->getSize();
    uint64_t id = 0;

    if (this->bridge != nullptr) {
      id = this->bridge->id;
    }

    return JSON::Object::Entries {
      {"id", std::to_string(id)},
      {"index", index},
      {"title", this->getTitle()},
      {"width", size.width},
      {"height", size.height},
      {"status", this->status}
    };
  }
}
