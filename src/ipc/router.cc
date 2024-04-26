#include "bridge.hh"
#include "router.hh"

namespace SSC::IPC {
  /*
    .
    ├── a-conflict-index
    │             └── index.html
    ├── a-conflict-index.html
    ├── an-index-file
    │             ├── a-html-file.html
    │             └── index.html
    ├── another-file.html
    └── index.html

    Subtleties:
    Direct file navigation always wins
    /foo/index.html have precedent over foo.html
    /foo redirects to /foo/ when there is a /foo/index.html

    '/' -> '/index.html'
    '/index.html' -> '/index.html'
    '/a-conflict-index' -> redirect to '/a-conflict-index/'
    '/another-file' -> '/another-file.html'
    '/another-file.html' -> '/another-file.html'
    '/an-index-file/' -> '/an-index-file/index.html'
    '/an-index-file' -> redirect to '/an-index-file/'
    '/an-index-file/a-html-file' -> '/an-index-file/a-html-file.html'
   */
  Router::WebViewURLPathResolution Router::resolveURLPathForWebView (String inputPath, const String& basePath) {
    namespace fs = std::filesystem;

    if (inputPath.starts_with("/")) {
      inputPath = inputPath.substr(1);
    }

    // Resolve the full path
    const auto fullPath = (fs::path(basePath) / fs::path(inputPath)).make_preferred();

    // 1. Try the given path if it's a file
    if (fs::is_regular_file(fullPath)) {
      return Router::WebViewURLPathResolution{"/" + replace(fs::relative(fullPath, basePath).string(), "\\\\", "/")};
    }

    // 2. Try appending a `/` to the path and checking for an index.html
    const auto indexPath = fullPath / fs::path("index.html");
    if (fs::is_regular_file(indexPath)) {
      if (fullPath.string().ends_with("\\") || fullPath.string().ends_with("/")) {
        return Router::WebViewURLPathResolution{
          .pathname = "/" + replace(fs::relative(indexPath, basePath).string(), "\\\\", "/"),
          .redirect = false
        };
      } else {
        return Router::WebViewURLPathResolution{
          .pathname = "/" + replace(fs::relative(fullPath, basePath).string(), "\\\\", "/") + "/",
          .redirect = true
        };
      }
    }

    // 3. Check if appending a .html file extension gives a valid file
    auto htmlPath = fullPath;
    htmlPath.replace_extension(".html");
    if (fs::is_regular_file(htmlPath)) {
      return Router::WebViewURLPathResolution{"/" + replace(fs::relative(htmlPath, basePath).string(), "\\\\", "/")};
    }

    // If no valid path is found, return empty string
    return Router::WebViewURLPathResolution{};
  };

  static const Map getWebViewNavigatorMounts () {
    static const auto userConfig = getUserConfig();
    static Map mounts;

    // determine HOME
  #if SSC_PLATFORM_WINDOWS
    static const auto HOME = Env::get("HOMEPATH", Env::get("USERPROFILE", Env::get("HOME")));
  #elif SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
    static const auto HOME = String(NSHomeDirectory().UTF8String);
  #else
    static const auto uid = getuid();
    static const auto pwuid = getpwuid(uid);
    static const auto HOME = pwuid != nullptr
      ? String(pwuid->pw_dir)
      : Env::get("HOME", getcwd());
  #endif

    if (mounts.size() > 0) {
      return mounts;
    }

    Map mappings = {
      {"\\$HOST_HOME", HOME},
      {"~", HOME},

      {"\\$HOST_CONTAINER",
    #if defined(__APPLE__)
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        [NSSearchPathForDirectoriesInDomains(NSApplicationDirectory, NSUserDomainMask, YES) objectAtIndex: 0].UTF8String
      #else
        // `homeDirectoryForCurrentUser` resolves to sandboxed container
        // directory when in "sandbox" mode, otherwise the user's HOME directory
        NSFileManager.defaultManager.homeDirectoryForCurrentUser.absoluteString.UTF8String
      #endif
    #elif defined(__linux__)
        // TODO(@jwerle): figure out `$HOST_CONTAINER` for Linux
        getcwd(),
    #elif defined(_WIN32)
        // TODO(@jwerle): figure out `$HOST_CONTAINER` for Windows
        getcwd(),
    #endif
      },

      {"\\$HOST_PROCESS_WORKING_DIRECTORY",
    #if defined(__APPLE__)
        NSBundle.mainBundle.resourcePath.UTF8String
    #else
        getcwd(),
    #endif
      }
    };

    for (const auto& tuple : userConfig) {
      if (tuple.first.starts_with("webview_navigator_mounts_")) {
        auto key = replace(tuple.first, "webview_navigator_mounts_", "");

        if (key.starts_with("android") && !platform.android) continue;
        if (key.starts_with("ios") && !platform.ios) continue;
        if (key.starts_with("linux") && !platform.linux) continue;
        if (key.starts_with("mac") && !platform.mac) continue;
        if (key.starts_with("win") && !platform.win) continue;

        key = replace(key, "android_", "");
        key = replace(key, "ios_", "");
        key = replace(key, "linux_", "");
        key = replace(key, "mac_", "");
        key = replace(key, "win_", "");

        String path = key;

        for (const auto& map : mappings) {
          path = replace(path, map.first, map.second);
        }

        const auto& value = tuple.second;
        mounts.insert_or_assign(path, value);
      }
    }

    return mounts;
  }

  Router::WebViewNavigatorMount Router::resolveNavigatorMountForWebView (const String& path) {
    static const auto mounts = getWebViewNavigatorMounts();

    for (const auto& tuple : mounts) {
      if (path.starts_with(tuple.second)) {
        const auto relative = replace(path, tuple.second, "");
        const auto resolution = resolveURLPathForWebView(relative, tuple.first);
        if (resolution.pathname.size() > 0) {
          const auto resolved = Path(tuple.first) / resolution.pathname.substr(1);
          return WebViewNavigatorMount {
            resolution,
            resolved.string(),
            path
          };
        } else {
          const auto resolved = relative.starts_with("/")
            ? Path(tuple.first) / relative.substr(1)
            : Path(tuple.first) / relative;

          return WebViewNavigatorMount {
            resolution,
            resolved.string(),
            path
          };
        }
      }
    }

    return WebViewNavigatorMount {};
  }

  void Router::init (Bridge* bridge) {
    this->bridge = bridge;

    this->init();
    this->preserveCurrentTable();
  }

  void Router::preserveCurrentTable () {
    this->preserved = this->table;
  }

  uint64_t Router::listen (const String& name, MessageCallback callback) {
    if (!this->listeners.contains(name)) {
      this->listeners[name] = Vector<MessageCallbackListenerContext>();
    }

    auto& listeners = this->listeners.at(name);
    auto token = rand64();
    listeners.push_back(MessageCallbackListenerContext { token , callback });
    return token;
  }

  bool Router::unlisten (const String& name, uint64_t token) {
    if (!this->listeners.contains(name)) {
      return false;
    }

    auto& listeners = this->listeners.at(name);
    for (int i = 0; i < listeners.size(); ++i) {
      const auto& listener = listeners[i];
      if (listener.token == token) {
        listeners.erase(listeners.begin() + i);
        return true;
      }
    }

    return false;
  }

  void Router::map (const String& name, MessageCallback callback) {
    return this->map(name, true, callback);
  }

  void Router::map (const String& name, bool async, MessageCallback callback) {
    String data = name;
    // URI hostnames are not case sensitive. Convert to lowercase.
    std::transform(data.begin(), data.end(), data.begin(),
      [](unsigned char c) { return std::tolower(c); });
    if (callback != nullptr) {
      table.insert_or_assign(data, MessageCallbackContext { async, callback });
    }
  }

  void Router::unmap (const String& name) {
    String data = name;
    // URI hostnames are not case sensitive. Convert to lowercase.
    std::transform(data.begin(), data.end(), data.begin(),
      [](unsigned char c) { return std::tolower(c); });
    if (table.find(data) != table.end()) {
      table.erase(data);
    }
  }

  bool Router::invoke (const String& uri, const char *bytes, size_t size) {
    return this->invoke(uri, bytes, size, [this](auto result) {
      this->send(result.seq, result.str(), result.post);
    });
  }

  bool Router::invoke (const String& uri, ResultCallback callback) {
    return this->invoke(uri, nullptr, 0, callback);
  }

  bool Router::invoke (
    const String& uri,
    const char *bytes,
    size_t size,
    ResultCallback callback
  ) {
    if (this->core->shuttingDown) {
      return false;
    }

    auto message = Message(uri, true);
    return this->invoke(message, bytes, size, callback);
  }

  bool Router::invoke (
    const Message& message,
    const char *bytes,
    size_t size,
    ResultCallback callback
  ) {
    if (this->core->shuttingDown) {
      return false;
    }

    auto name = message.name;
    MessageCallbackContext ctx;

    // URI hostnames are not case sensitive. Convert to lowercase.
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
      return std::tolower(c);
    });

    do {
      // lookup router function in the preserved table,
      // then the public table, return if unable to determine a context
      if (this->preserved.find(name) != this->preserved.end()) {
        ctx = this->preserved.at(name);
      } else if (this->table.find(name) != this->table.end()) {
        ctx = this->table.at(name);
      } else {
        return false;
      }
    } while (0);

    if (ctx.callback != nullptr) {
      Message msg(message);
      // decorate message with buffer if buffer was previously
      // mapped with `ipc://buffer.map`, which we do on Linux
      if (this->hasMappedBuffer(msg.index, msg.seq)) {
        msg.buffer = this->getMappedBuffer(msg.index, msg.seq);
        this->removeMappedBuffer(msg.index, msg.seq);
      } else if (bytes != nullptr && size > 0) {
        msg.buffer.bytes = std::make_shared<char*>(new char[size]{0});
        msg.buffer.size = size;
        memcpy(*msg.buffer.bytes, bytes, size);
      }

      // named listeners
      do {
        auto listeners = this->listeners[name];
        for (const auto& listener : listeners) {
          listener.callback(msg, this, [](const auto& _) {});
        }
      } while (0);

      // wild card (*) listeners
      do {
        auto listeners = this->listeners["*"];
        for (const auto& listener : listeners) {
          listener.callback(msg, this, [](const auto& _) {});
        }
      } while (0);

      if (ctx.async) {
        auto dispatched = this->dispatch([ctx, msg, callback, this]() mutable {
          ctx.callback(msg, this, [msg, callback, this](const auto result) mutable {
            if (result.seq == "-1") {
              this->send(result.seq, result.str(), result.post);
            } else {
              callback(result);
            }
          });
        });

        return dispatched;
      } else {
        ctx.callback(msg, this, [msg, callback, this](const auto result) mutable {
          if (result.seq == "-1") {
            this->send(result.seq, result.str(), result.post);
          } else {
            callback(result);
          }
        });

        return true;
      }
    }

    return false;
  }

  bool Router::send (
    const Message::Seq& seq,
    const String data,
    const Post post
  ) {
    if (this->core->shuttingDown) {
      return false;
    }

    if (post.body != nullptr || seq == "-1") {
      auto script = this->core->createPost(seq, data, post);
      return this->evaluateJavaScript(script);
    }

    auto value = encodeURIComponent(data);
    auto script = getResolveToRenderProcessJavaScript(
      seq.size() == 0 ? "-1" : seq,
      "0",
      value
    );

    return this->evaluateJavaScript(script);
  }

  bool Router::emit (
    const String& name,
    const String data
  ) {
    if (this->core->shuttingDown) {
      return false;
    }

    const auto value = encodeURIComponent(data);
    const auto script = getEmitToRenderProcessJavaScript(name, value);
    return this->evaluateJavaScript(script);
  }

  bool Router::evaluateJavaScript (const String js) {
    if (this->core->shuttingDown) {
      return false;
    }

    if (this->evaluateJavaScriptFunction != nullptr) {
      this->evaluateJavaScriptFunction(js);
      return true;
    }

    return false;
  }

  bool Router::dispatch (DispatchCallback callback) {
    if (!this->core || this->core->shuttingDown) {
      return false;
    }

    if (this->dispatchFunction != nullptr) {
      this->dispatchFunction(callback);
      return true;
    }

    return false;
  }

  bool Router::hasMappedBuffer (int index, const Message::Seq seq) {
    auto key = std::to_string(index) + seq;
    return this->buffers.find(key) != this->buffers.end();
  }

  MessageBuffer Router::getMappedBuffer (int index, const Message::Seq seq) {
    if (this->hasMappedBuffer(index, seq)) {
      auto key = std::to_string(index) + seq;
      return this->buffers.at(key);
    }

    return MessageBuffer {};
  }

  void Router::setMappedBuffer (
    int index,
    const Message::Seq seq,
    MessageBuffer buffer
  ) {
    auto key = std::to_string(index) + seq;
    this->buffers.insert_or_assign(key, buffer);
  }

  void Router::removeMappedBuffer (int index, const Message::Seq seq) {
    if (this->hasMappedBuffer(index, seq)) {
      auto key = std::to_string(index) + seq;
      this->buffers.erase(key);
    }
  }
}
