#include "../url.hh"
#include "../env.hh"
#include "../color.hh"
#include "../config.hh"
#include "../string.hh"
#include "../window.hh"
#include "../runtime.hh"

using ssc::runtime::config::isDebugEnabled;
using ssc::runtime::config::getUserConfig;
using ssc::runtime::config::getDevHost;

using ssc::runtime::string::parseStringList;
using ssc::runtime::string::trim;

using ssc::runtime::color::Color;

using ssc::runtime::url::decodeURIComponent;

namespace ssc::runtime::window {
  Manager::Manager (context::RuntimeContext& context)
    : context(context),
      windows(SOCKET_RUNTIME_MAX_WINDOWS + SOCKET_RUNTIME_MAX_WINDOWS_RESERVED)
  {}

  Manager::~Manager () {
    this->destroy();
  }

  void Manager::destroy () {
    if (this->destroyed) {
      return;
    }

    this->windows.clear();
    this->destroyed = true;
  }

  void Manager::configure (const ManagerOptions& options) {
    if (this->destroyed) {
      return;
    }

    this->options = options;
    if (this->options.userConfig.size() == 0) {
      this->options.userConfig = getUserConfig();
    }
  }

  SharedPointer<Manager::ManagedWindow> Manager::getWindow (
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

  SharedPointer<Manager::ManagedWindow> Manager::getWindow (int index) {
    return this->getWindow(index, WindowStatus::WINDOW_EXITING);
  }

  SharedPointer<Manager::ManagedWindow> Manager::getWindowForBridge (
    const IBridge* bridge
  ) {
    for (const auto& window : this->windows) {
      if (window != nullptr && window->bridge.get() == bridge) {
        return this->getWindow(window->index);
      }
    }
    return nullptr;
  }

  SharedPointer<Manager::ManagedWindow> Manager::getWindowForWebView (webview::WebView* webview) {
    for (const auto& window : this->windows) {
      if (window != nullptr && window->webview == webview) {
        return window;
      }
    }
    return nullptr;
  }

  SharedPointer<Manager::ManagedWindow> Manager::getWindowForClient (const Client& client) {
    for (const auto& window : this->windows) {
      if (window != nullptr && window->bridge != nullptr && window->bridge->client.id == client.id) {
        return this->getWindow(window->index);
      }
    }

    return nullptr;
  }

  SharedPointer<Manager::ManagedWindow> Manager::getOrCreateWindow (int index) {
    return this->getOrCreateWindow(index, Window::Options {});
  }

  SharedPointer<Manager::ManagedWindow> Manager::getOrCreateWindow (
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

  Manager::WindowStatus Manager::getWindowStatus (int index) {
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

  void Manager::destroyWindow (int index) {
    Lock lock(this->mutex);

    if (this->destroyed || index < 0 || index >= this->windows.size()) {
      return;
    }

    auto window = this->windows[index];
    if (window != nullptr) {
      window->close();

      this->windows[index] = nullptr;
      if (window->options.shouldExitApplicationOnClose) {
        static_cast<runtime::Runtime&>(this->context).dispatch([window]() {
          window->exit(0);
        });
      } else {
        window->kill();
      }
    }
  }

  SharedPointer<Manager::ManagedWindow> Manager::createWindow (
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
      .shouldPreferServiceWorker = options.shouldPreferServiceWorker,
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
      .resourcesDirectory = options.resourcesDirectory,
    };

    windowOptions.RUNTIME_PRIMORDIAL_OVERRIDES = options.RUNTIME_PRIMORDIAL_OVERRIDES;
    windowOptions.userScript = options.userScript;
    windowOptions.headless = options.headless;
    windowOptions.features = options.features;
    windowOptions.debug = isDebugEnabled() || options.debug;
    windowOptions.index = options.index;
    windowOptions.argv = options.argv;

    if (options.userConfig.size() > 0) {
      windowOptions.userConfig = options.userConfig;
    } else {
      windowOptions.userConfig = this->options.userConfig;
    }

    for (auto const &entry : parseStringList(windowOptions.userConfig["build_env"])) {
      const auto key = trim(entry);

      if (!env::has(key)) {
        continue;
      }

      const auto value = decodeURIComponent(env::get(key));

      windowOptions.env[key] = value;
    }

    for (auto const &tuple : windowOptions.userConfig) {
      if (tuple.first.starts_with("env_")) {
        const auto key = tuple.first.substr(4);
        const auto& value = tuple.second;
        windowOptions.env[key] = value;
      }
    }

    windowOptions.origin = webview::Origin("socket://" + windowOptions.userConfig["meta_bundle_identifier"] + "/");
    auto serviceWorker = static_cast<runtime::Runtime&>(this->context).serviceWorkerManager.get(
      windowOptions.origin.name()
    );

    if (!serviceWorker) {
      serviceWorker = static_cast<runtime::Runtime&>(this->context).serviceWorkerManager.init(
        windowOptions.origin.name(),
        serviceworker::Server::Options {
          windowOptions.origin.name(),
          windowOptions.userConfig
        }
      );
    }

    auto bridge = static_cast<runtime::Runtime&>(this->context).bridgeManager.get(options.index, {
      .userConfig = windowOptions.userConfig
    });

    bridge->index = options.index;
    bridge->navigator.serviceWorkerServer = serviceWorker;
  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    bridge->dispatchHandler = [](const auto callback) {
      callback();
    };
  #endif
    auto window = std::make_shared<ManagedWindow>(*this, bridge, windowOptions);

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

  SharedPointer<Manager::ManagedWindow> Manager::createDefaultWindow (const Window::Options& options) {
    static const auto devHost = getDevHost();
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

  JSON::Array Manager::json (const Vector<int>& indices) {
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

  bool Manager::emit (const String& event, const JSON::Any& json) {
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

  Manager::ManagedWindow::ManagedWindow (
    Manager &manager,
    SharedPointer<IBridge> bridge,
    const Window::Options& options
  ) : Window(bridge, options),
      manager(manager)
  {}

  Manager::ManagedWindow::~ManagedWindow () {}

  void Manager::ManagedWindow::show () {
    auto index = std::to_string(this->index);
    this->backgroundColor = Color(Window::getBackgroundColor());
    status = WindowStatus::WINDOW_SHOWING;
    Window::show();
    status = WindowStatus::WINDOW_SHOWN;
  }

  void Manager::ManagedWindow::hide () {
    auto index = std::to_string(this->index);
    status = WindowStatus::WINDOW_HIDING;
    Window::hide();
    status = WindowStatus::WINDOW_HIDDEN;
  }

  void Manager::ManagedWindow::close (int code) {
    if (status < WindowStatus::WINDOW_CLOSING) {
      auto index = std::to_string(this->index);
      status = WindowStatus::WINDOW_CLOSING;
      Window::close(code);
      status = WindowStatus::WINDOW_CLOSED;
    }
  }

  void Manager::ManagedWindow::exit (int code) {
    if (status < WindowStatus::WINDOW_EXITING) {
      auto index = std::to_string(this->index);
      status = WindowStatus::WINDOW_EXITING;
      Window::exit(code);
      status = WindowStatus::WINDOW_EXITED;
    }
  }

  void Manager::ManagedWindow::kill () {
    if (status < WindowStatus::WINDOW_KILLING) {
      auto index = std::to_string(this->index);
      status = WindowStatus::WINDOW_KILLING;
      Window::kill();
      status = WindowStatus::WINDOW_KILLED;
    }
  }

  JSON::Object Manager::ManagedWindow::json () const {
    const auto id = this->bridge->client.id;
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
      {"url", this->bridge->navigator.location.str()},
      {"location", this->bridge->navigator.location.json()},
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

  void Manager::ManagedWindow::onReadyStateChange (const ReadyState& readyState) {
    if (readyState == ReadyState::Complete) {
      const auto pendingApplicationURLs = this->pendingApplicationURLs;
      this->pendingApplicationURLs.clear();
      for (const auto& url : pendingApplicationURLs) {
        Window::handleApplicationURL(url);
      }
    }
  }

  void Manager::ManagedWindow::handleApplicationURL (const String& url) {
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

  bool Manager::ManagedWindow::emit (const String& event, const JSON::Any& json) {
    return this->bridge && this->bridge->emit(event, json);
  }

  void Manager::ManagedWindow::setBackgroundColor (int r, int g, int b, float a) {
    this->backgroundColor = Color(r, g, b, a);
    Window::setBackgroundColor(r, g, b, a);
  }

  void Manager::ManagedWindow::setBackgroundColor (const String& rgba) {
    this->backgroundColor = Color(rgba);
    Window::setBackgroundColor(rgba);
  }

  void Manager::ManagedWindow::setBackgroundColor (const ::Color& color) {
    this->backgroundColor = color;
    Window::setBackgroundColor(color.str());
  }

  String Manager::ManagedWindow::getBackgroundColor () {
    return this->backgroundColor.str();
  }

  int Manager::getNextWindowIndex (bool reserved) const {
    if (reserved) {
      for (
        int i = SOCKET_RUNTIME_MAX_WINDOWS;
        i < SOCKET_RUNTIME_MAX_WINDOWS + SOCKET_RUNTIME_MAX_WINDOWS_RESERVED;
        ++i
      ) {
        const auto window = this->windows[i];
        if (window == nullptr) {
          return i;
        }
      }
      return -1;
    }

    return -1;
  }

  int Manager::getRandomWindowIndex (bool reserved) const {
    if (reserved) {
      size_t remaining = SOCKET_RUNTIME_MAX_WINDOWS_RESERVED;
      while (remaining--) {
        const auto index = crypto::randint(
          SOCKET_RUNTIME_MAX_WINDOWS,
          SOCKET_RUNTIME_MAX_WINDOWS + SOCKET_RUNTIME_MAX_WINDOWS_RESERVED - 1
        );

        const auto window = this->windows[index];
        if (window == nullptr) {
          return index;
        }
      }
      return -1;
    }

    size_t remaining = SOCKET_RUNTIME_MAX_WINDOWS;
    while (remaining--) {
      const auto index = crypto::randint(
        0,
        SOCKET_RUNTIME_MAX_WINDOWS - 1
      );

      const auto window = this->windows[index];
      if (window == nullptr) {
        return index;
      }
    }

    return -1;
  }
}
