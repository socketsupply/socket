#include "window.hh"

namespace SSC {
  WindowManager::WindowManager (App &app)
    : app(app),
      inits(SSC_MAX_WINDOWS + SSC_MAX_WINDOWS_RESERVED),
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

    for (auto window : windows) {
      destroyWindow(window);
    }

    this->destroyed = true;
    this->windows.clear();
    this->inits.clear();
  }

  void WindowManager::WindowManager::configure (WindowManagerOptions configuration) {
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

  WindowManager::ManagedWindow* WindowManager::WindowManager::getWindow (int index, WindowStatus status) {
    Lock lock(this->mutex);
    if (this->destroyed) {
      return nullptr;
    }

    if (
      this->getWindowStatus(index) > WindowStatus::WINDOW_NONE &&
      this->getWindowStatus(index) < status
    ) {
      return this->windows[index];
    }

    return nullptr;
  }

  WindowManager::ManagedWindow* WindowManager::WindowManager::getWindow (int index) {
    return this->getWindow(index, WindowStatus::WINDOW_EXITING);
  }

  WindowManager::ManagedWindow* WindowManager::WindowManager::getOrCreateWindow (int index) {
    return this->getOrCreateWindow(index, WindowOptions {});
  }

  WindowManager::ManagedWindow* WindowManager::WindowManager::getOrCreateWindow (
    int index,
    WindowOptions opts
  ) {
    if (this->destroyed || index < 0) {
      return nullptr;
    }

    if (this->getWindowStatus(index) == WindowStatus::WINDOW_NONE) {
      opts.index = index;
      return this->createWindow(opts);
    }

    return this->getWindow(index);
  }

  WindowManager::WindowStatus WindowManager::getWindowStatus (int index) {
    Lock lock(this->mutex);
    if (this->destroyed) {
      return WindowStatus::WINDOW_NONE;
    }

    if (index >= 0 && this->inits[index] && this->windows[index] != nullptr) {
      return this->windows[index]->status;
    }

    return WindowStatus::WINDOW_NONE;
  }

  void WindowManager::destroyWindow (int index) {
    Lock lock(this->mutex);

    if (!this->destroyed && index >= 0 && this->inits[index] && this->windows[index] != nullptr) {
      return this->destroyWindow(windows[index]);
    }
  }

  void WindowManager::destroyWindow (ManagedWindow* window) {
    if (!this->destroyed && window != nullptr) {
      return this->destroyWindow(reinterpret_cast<Window*>(window));
    }
  }

  void WindowManager::destroyWindow (Window* window) {
    Lock lock(this->mutex);

    if (!this->destroyed && window != nullptr && this->windows[window->index] != nullptr) {
      auto metadata = reinterpret_cast<ManagedWindow*>(window);
      this->inits[window->index] = false;
      this->windows[window->index] = nullptr;

      if (metadata->status < WINDOW_CLOSING) {
        window->close(0);
      }

      if (metadata->status < WINDOW_KILLING) {
        window->kill();
      }

      if (!window->opts.canExit) {
        delete window;
      }
    }
  }

  WindowManager::ManagedWindow* WindowManager::createWindow (WindowOptions opts) {
    Lock lock(this->mutex);

    if (this->destroyed) {
      return nullptr;
    }

    StringStream env;

    if (this->inits[opts.index] && this->windows[opts.index] != nullptr) {
      return this->windows[opts.index];
    }

    if (opts.userConfig.size() > 0) {
      for (auto const &envKey : parseStringList(opts.userConfig["build_env"])) {
        auto cleanKey = trim(envKey);

        if (!Env::has(cleanKey)) {
          continue;
        }

        auto envValue = Env::get(cleanKey.c_str());

        env << String(
          cleanKey + "=" + encodeURIComponent(envValue) + "&"
        );
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

    float width = opts.width <= 0
      ? Window::getSizeInPixels(this->options.defaultWidth, screen.width)
      : opts.width;
    float height = opts.height <= 0
      ? Window::getSizeInPixels(this->options.defaultHeight, screen.height)
      : opts.height;
    float minWidth = opts.minWidth <= 0
      ? Window::getSizeInPixels(this->options.defaultMinWidth, screen.width)
      : opts.minWidth;
    float minHeight = opts.minHeight <= 0
      ? Window::getSizeInPixels(this->options.defaultMinHeight, screen.height)
      : opts.minHeight;
    float maxWidth = opts.maxWidth <= 0
      ? Window::getSizeInPixels(this->options.defaultMaxWidth, screen.width)
      : opts.maxWidth;
    float maxHeight = opts.maxHeight <= 0
      ? Window::getSizeInPixels(this->options.defaultMaxHeight, screen.height)
      : opts.maxHeight;

    WindowOptions windowOptions = {
      .resizable = opts.resizable,
      .minimizable = opts.minimizable,
      .maximizable = opts.maximizable,
      .closable = opts.closable,
      .frameless = opts.frameless,
      .utility = opts.utility,
      .canExit = opts.canExit,
      .width = width,
      .height = height,
      .minWidth = minWidth,
      .minHeight = minHeight,
      .maxWidth = maxWidth,
      .maxHeight = maxHeight,
      .radius = opts.radius,
      .margin = opts.margin,
      .index = opts.index,
      .debug = isDebugEnabled() || opts.debug,
      .isTest = this->options.isTest,
      .headless = opts.headless,
      .aspectRatio = opts.aspectRatio,
      .titlebarStyle = opts.titlebarStyle,
      .windowControlOffsets = opts.windowControlOffsets,
      .backgroundColorLight = opts.backgroundColorLight,
      .backgroundColorDark = opts.backgroundColorDark,
      .cwd = this->options.cwd,
      .title = opts.title.size() > 0 ? opts.title : "",
      .url = opts.url.size() > 0 ? opts.url : "data:text/html,<html>",
      .argv = this->options.argv,
      .preload = opts.preload.size() > 0 ? opts.preload : "",
      .env = env.str(),
      .userConfig = this->options.userConfig,
      .userScript = opts.userScript,
      .runtimePrimordialOverrides = opts.runtimePrimordialOverrides,
      .preloadCommonJS = opts.preloadCommonJS != false
    };

    for (const auto& tuple : opts.userConfig) {
      windowOptions.userConfig[tuple.first] = tuple.second;
    }

    if (isDebugEnabled()) {
      this->log("Creating Window#" + std::to_string(opts.index));
    }

    auto window = new ManagedWindow(*this, app, windowOptions);

    window->status = WindowStatus::WINDOW_CREATED;
    window->onExit = this->options.onExit;
    window->onMessage = this->options.onMessage;

    this->windows[opts.index] = window;
    this->inits[opts.index] = true;

    return window;
  }

  WindowManager::ManagedWindow* WindowManager::createDefaultWindow (WindowOptions opts) {
    return this->createWindow(WindowOptions {
      .resizable = opts.resizable,
      .minimizable = opts.minimizable,
      .maximizable = opts.maximizable,
      .closable = opts.closable,
      .frameless = opts.frameless,
      .utility = opts.utility,
      .canExit = true,
      .width = opts.width,
      .height = opts.height,
      .index = 0,
    #ifdef PORT
      .port = PORT,
    #endif
      .headless = opts.userConfig["build_headless"] == "true",
      .titlebarStyle = opts.titlebarStyle,
      .windowControlOffsets = opts.windowControlOffsets,
      .backgroundColorLight = opts.backgroundColorLight,
      .backgroundColorDark = opts.backgroundColorDark,
      .userConfig = opts.userConfig
    });
  }

  JSON::Array WindowManager::json (Vector<int> indices) {
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
      gc();
    }
  }

  void WindowManager::ManagedWindow::gc () {
    if (App::instance() != nullptr) {
      manager.destroyWindow(reinterpret_cast<Window*>(this));
    }
  }

  JSON::Object WindowManager::ManagedWindow::json () {
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
