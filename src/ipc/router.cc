#include "bridge.hh"
#include "router.hh"

// create a proxy module so imports of the module of concern are imported
// exactly once at the canonical URL (file:///...) in contrast to module
// URLs (socket:...)

static constexpr auto moduleTemplate =
R"S(
import module from '{{url}}'
export * from '{{url}}'
export default module
)S";

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

  Router::WebViewURLComponents Router::parseURLComponents (const SSC::String& url) {
    Router::WebViewURLComponents components;
    components.originalURL = url;
    auto input = url;

    if (input.starts_with("./")) {
      input = input.substr(1);
    }

    if (!input.starts_with("/")) {
      const auto colon = input.find(':');

      if (colon != String::npos) {
        components.scheme = input.substr(0, colon);
        input = input.substr(colon + 1, input.size());

        if (input.starts_with("//")) {
          input = input.substr(2, input.size());

          const auto slash = input.find("/");
          if (slash != String::npos) {
            components.authority = input.substr(0, slash);
            input = input.substr(slash, input.size());
          } else {
            const auto questionMark = input.find("?");
            const auto fragment = input.find("#");
            if (questionMark != String::npos & fragment != String::npos) {
              if (questionMark < fragment) {
                components.authority = input.substr(0, questionMark);
                input = input.substr(questionMark, input.size());
              } else {
                components.authority = input.substr(0, fragment);
                input = input.substr(fragment, input.size());
              }
            } else if (questionMark != String::npos) {
              components.authority = input.substr(0, questionMark);
              input = input.substr(questionMark, input.size());
            } else if (fragment != String::npos) {
              components.authority = input.substr(0, fragment);
              input = input.substr(fragment, input.size());
            }
          }
        }
      }
    }

    input = decodeURIComponent(input);

    const auto questionMark = input.find("?");
    const auto fragment = input.find("#");

    if (questionMark != String::npos && fragment != String::npos) {
      if (questionMark < fragment) {
        components.pathname = input.substr(0, questionMark);
        components.query = input.substr(questionMark + 1, fragment - questionMark - 1);
        components.fragment = input.substr(fragment + 1, input.size());
      } else {
        components.pathname = input.substr(0, fragment);
        components.fragment = input.substr(fragment + 1, input.size());
      }
    } else if (questionMark != String::npos) {
      components.pathname = input.substr(0, questionMark);
      components.query = input.substr(questionMark + 1, input.size());
    } else if (fragment != String::npos) {
      components.pathname = input.substr(0, fragment);
      components.fragment = input.substr(fragment + 1, input.size());
    } else {
      components.pathname = input;
    }

    if (!components.pathname.starts_with("/")) {
      components.pathname = "/" + components.pathname;
    }

    return components;
  }

  static const Map getWebViewNavigatorMounts () {
    static const auto userConfig = getUserConfig();
  #if defined(_WIN32)
    static const auto HOME = Env::get("HOMEPATH", Env::get("USERPROFILE", Env::get("HOME")));
  #elif defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
    static const auto HOME = String(NSHomeDirectory().UTF8String);
  #else
    static const auto uid = getuid();
    static const auto pwuid = getpwuid(uid);
    static const auto HOME = pwuid != nullptr
      ? String(pwuid->pw_dir)
      : Env::get("HOME", getcwd());
  #endif

    static Map mounts;

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

  Router::Router () : schemeHandlers(this) {}

  void Router::init (Bridge* bridge) {
    this->bridge = bridge;

    this->init();
    this->preserveCurrentTable();
  }

  Router::~Router () {
  }

  void Router::preserveCurrentTable () {
    Lock lock(mutex);
    this->preserved = this->table;
  }

  uint64_t Router::listen (const String& name, MessageCallback callback) {
    Lock lock(mutex);

    if (!this->listeners.contains(name)) {
      this->listeners[name] = Vector<MessageCallbackListenerContext>();
    }

    auto& listeners = this->listeners.at(name);
    auto token = rand64();
    listeners.push_back(MessageCallbackListenerContext { token , callback });
    return token;
  }

  bool Router::unlisten (const String& name, uint64_t token) {
    Lock lock(mutex);

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
    Lock lock(mutex);

    String data = name;
    // URI hostnames are not case sensitive. Convert to lowercase.
    std::transform(data.begin(), data.end(), data.begin(),
      [](unsigned char c) { return std::tolower(c); });
    if (callback != nullptr) {
      table.insert_or_assign(data, MessageCallbackContext { async, callback });
    }
  }

  void Router::unmap (const String& name) {
    Lock lock(mutex);

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
    Lock lock(this->mutex);
    auto key = std::to_string(index) + seq;
    return this->buffers.find(key) != this->buffers.end();
  }

  MessageBuffer Router::getMappedBuffer (int index, const Message::Seq seq) {
    if (this->hasMappedBuffer(index, seq)) {
      Lock lock(this->mutex);
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
    Lock lock(this->mutex);
    auto key = std::to_string(index) + seq;
    this->buffers.insert_or_assign(key, buffer);
  }

  void Router::removeMappedBuffer (int index, const Message::Seq seq) {
    Lock lock(this->mutex);
    if (this->hasMappedBuffer(index, seq)) {
      auto key = std::to_string(index) + seq;
      this->buffers.erase(key);
    }
  }

  void Router::configureHandlers (const SchemeHandlers::Configuration& configuration) {
    this->schemeHandlers.configure(configuration);
    this->schemeHandlers.registerSchemeHandler("ipc", [this](
      const auto& request,
      const auto router,
      auto& callbacks,
      auto callback
    ) {
      auto message = Message(request.url(), true);

      // handle special 'ipc://post' case
      if (message.name == "post") {
        uint64_t id = 0;

        try {
          id = std::stoull(message.get("id"));
        } catch (...) {
          auto response = SchemeHandlers::Response(request, 400);
          response.send(JSON::Object::Entries {
            {"err", JSON::Object::Entries {
              {"message", "Invalid 'id' given in parameters"}
            }}
          });

          callback(response);
          return;
        }

        if (!this->core->hasPost(id)) {
          auto response = SchemeHandlers::Response(request, 404);
          response.send(JSON::Object::Entries {
            {"err", JSON::Object::Entries {
              {"message", "A 'Post' was not found for the given 'id' in parameters"},
              {"type", "NotFoundError"}
            }}
          });

          callback(response);
          return;
        }

        auto response = SchemeHandlers::Response(request, 200);
        const auto post = this->core->getPost(id);

        // handle raw headers in 'Post' object
        if (post.headers.size() > 0) {
          const auto lines = split(trim(post.headers), '\n');
          for (const auto& line : lines) {
            const auto pair = split(trim(line), ':');
            const auto key = trim(pair[0]);
            const auto value = trim(pair[1]);
            response.setHeader(key, value);
          }
        }

        response.write(post.length, post.body);
        callback(response);
        this->core->removePost(id);
        return;
      }

      message.isHTTP = true;
      message.cancel = std::make_shared<MessageCancellation>();

      callbacks.cancel = [message] () {
        if (message.cancel->handler != nullptr) {
          message.cancel->handler(message.cancel->data);
        }
      };

      const auto size = request.body.size;
      const auto bytes = request.body.bytes != nullptr ? *request.body.bytes : nullptr;
      const auto invoked = this->invoke(message, bytes, size, [request, message, callback](Result result) {
        if (!request.isActive()) {
          return;
        }

        auto response = SchemeHandlers::Response(request);

        response.setHeaders(result.headers);

        // handle event source streams
        if (result.post.eventStream != nullptr) {
          response.setHeader("content-type", "text/event-stream");
          response.setHeader("cache-control", "no-store");
          *result.post.eventStream = [request, response, message, callback](
            const char* name,
            const char* data,
            bool finished
          ) mutable {
            if (request.isCancelled()) {
              if (message.cancel->handler != nullptr) {
                message.cancel->handler(message.cancel->data);
              }
              return false;
            }

            response.writeHead(200);

            const auto event = SchemeHandlers::Response::Event { name, data };

            if (event.count() > 0) {
              response.write(event.str());
            }

            if (finished) {
              callback(response);
            }

            return true;
          };
          return;
        }

        // handle chunk streams
        if (result.post.chunkStream != nullptr) {
          response.setHeader("transfer-encoding", "chunked");
          *result.post.chunkStream = [request, response, message, callback](
            const char* chunk,
            size_t size,
            bool finished
          ) mutable {
            if (request.isCancelled()) {
              if (message.cancel->handler != nullptr) {
                message.cancel->handler(message.cancel->data);
              }
              return false;
            }

            response.writeHead(200);
            response.write(size, chunk);

            if (finished) {
              callback(response);
            }

            return true;
          };
          return;
        }

        if (result.post.body != nullptr) {
          response.write(result.post.length, result.post.body);
        } else {
          response.write(result.json());
        }

        callback(response);
      });

      if (!invoked) {
        auto response = SchemeHandlers::Response(request, 404);
        response.send(JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Not found"},
            {"type", "NotFoundError"},
            {"url", request.url()}
          }}
        });

        callback(response);
      }
    });

    this->schemeHandlers.registerSchemeHandler("socket", [this](
      const auto& request,
      const auto router,
      auto& callbacks,
      auto callback
    ) {
      auto userConfig = this->bridge->userConfig;
      auto bundleIdentifier = userConfig["meta_bundle_identifier"];
      // the location of static application resources
      const auto applicationResources = FileResource::getResourcesPath().string();
      // default response is 404
      auto response = SchemeHandlers::Response(request, 404);

      // the resouce path that may be request
      String resourcePath;

      // the content location relative to the request origin
      String contentLocation;

      // application resource or service worker request at `socket://<bundle_identifier>/*`
      if (request.hostname == bundleIdentifier) {
        const auto resolved = Router::resolveURLPathForWebView(request.pathname, applicationResources);
        const auto mount = Router::resolveNavigatorMountForWebView(request.pathname);

        if (mount.resolution.redirect || resolved.redirect) {
          auto pathname = mount.resolution.redirect
            ? mount.resolution.pathname
            : resolved.pathname;

          if (request.method == "GET") {
            auto location = mount.resolution.pathname;
            if (request.query.size() > 0) {
              location += "?" + request.query;
            }

            if (request.fragment.size() > 0) {
              location += "#" + request.fragment;
            }

            response.redirect(location);
            return callback(response);
          }
        } else if (mount.path.size() > 0) {
          resourcePath = mount.path;
        } else if (request.pathname == "" || request.pathname == "/") {
          if (userConfig.contains("webview_default_index")) {
            resourcePath = userConfig["webview_default_index"];
            if (resourcePath.starts_with("./")) {
              resourcePath = applicationResources + resourcePath.substr(1);
            } else if (resourcePath.starts_with("/")) {
              resourcePath = applicationResources + resourcePath;
            } else {
              resourcePath = applicationResources + + "/" + resourcePath;
            }
          }
        }

        if (resourcePath.size() == 0 && resolved.pathname.size() > 0) {
          resourcePath = applicationResources + resolved.pathname;
        }

        // handle HEAD and GET requests for a file resource
        if (resourcePath.size() > 0) {
          contentLocation = replace(resourcePath, applicationResources, "");

          auto resource = FileResource(resourcePath);

          if (!resource.exists()) {
            response.writeHead(404);
          } else {
            if (contentLocation.size() > 0) {
              response.setHeader("content-location", contentLocation);
            }

            if (request.method == "OPTIONS") {
              response.setHeader("access-control-allow-origin", "*");
              response.setHeader("access-control-allow-methods", "GET, HEAD");
              response.setHeader("access-control-allow-headers", "*");
              response.setHeader("access-control-allow-credentials", "true");
              response.writeHead(200);
            }

            if (request.method == "HEAD") {
              const auto contentType = resource.mimeType();
              const auto contentLength = resource.size();

              if (contentType.size() > 0) {
                response.setHeader("content-type", contentType);
              }

              if (contentLength > 0) {
                response.setHeader("content-length", contentLength);
              }

              response.writeHead(200);
            }

            if (request.method == "GET") {
              if (resource.mimeType() != "text/html") {
                response.send(resource);
              } else {
                const auto html = injectHTMLPreload(
                  this->core,
                  userConfig,
                  resource.string(),
                  this->bridge->preload
                );

                response.setHeader("content-type", "text/html");
                response.setHeader("content-length", html.size());
                response.writeHead(200);
                response.write(html);
              }
            }
          }

          return callback(response);
        }

        if (router->core->serviceWorker.registrations.size() > 0) {
          const auto fetch = ServiceWorkerContainer::FetchRequest {
            request.method,
            request.scheme,
            request.hostname,
            request.pathname,
            request.query,
            request.headers,
            ServiceWorkerContainer::FetchBuffer { request.body.size, request.body.bytes },
            ServiceWorkerContainer::Client { request.client.id }
          };

          const auto fetched = router->core->serviceWorker.fetch(fetch, [request, callback, response] (auto res) mutable {
            if (!request.isActive()) {
              return;
            }

            response.writeHead(res.statusCode, res.headers);
            response.write(res.buffer.size, res.buffer.bytes);
            callback(response);
          });

          if (fetched) {
            router->bridge->core->setTimeout(32000, [request] () mutable {
              if (request.isActive()) {
                auto response = SchemeHandlers::Response(request, 408);
                response.fail("ServiceWorker request timed out.");
              }
            });
            return;
          }
        }

        response.writeHead(404);
        return callback(response);
      }

      // module or stdlib import/fetch `socket:<module>/<path>` which will just
      // proxy an import into a normal resource request above
      if (request.hostname.size() == 0) {
        auto pathname = request.pathname;

        if (!pathname.ends_with(".js")) {
          pathname += ".js";
        }

        if (!pathname.starts_with("/")) {
          pathname = "/" + pathname;
        }

        resourcePath = applicationResources + "/socket" + pathname;
        contentLocation = "/socket" + pathname;

        auto resource = FileResource(resourcePath);

        if (resource.exists()) {
          const auto url = "socket://" + bundleIdentifier + "/socket" + pathname;
          const auto module = tmpl(moduleTemplate, Map {{"url", url}});
          const auto contentType = resource.mimeType();

          if (contentType.size() > 0) {
            response.setHeader("content-type", contentType);
          }

          response.setHeader("content-length", module.size());

          if (contentLocation.size() > 0) {
            response.setHeader("content-location", contentLocation);
          }

          response.writeHead(200);
          response.write(trim(module));
        }

        return callback(response);
      }

      response.writeHead(404);
      callback(response);
    });

    Map protocolHandlers = {
      {"npm", "/socket/npm/service-worker.js"}
    };

    for (const auto& entry : split(this->bridge->userConfig["webview_protocol-handlers"], " ")) {
      const auto scheme = replace(trim(entry), ":", "");
      if (this->bridge->core->protocolHandlers.registerHandler(scheme)) {
        protocolHandlers.insert_or_assign(scheme, "");
      }
    }

    for (const auto& entry : this->bridge->userConfig) {
      const auto& key = entry.first;
      if (key.starts_with("webview_protocol-handlers_")) {
        const auto scheme = replace(replace(trim(key), "webview_protocol-handlers_", ""), ":", "");;
        const auto data = entry.second;
        if (this->bridge->core->protocolHandlers.registerHandler(scheme, { data })) {
          protocolHandlers.insert_or_assign(scheme, data);
        }
      }
    }

    for (const auto& entry : protocolHandlers) {
      const auto& scheme = entry.first;
      const auto id = rand64();

      auto scriptURL = trim(entry.second);

      if (scriptURL.size() == 0) {
        continue;
      }

      if (!scriptURL.starts_with(".") && !scriptURL.starts_with("/")) {
        continue;
      }

      if (scriptURL.starts_with(".")) {
        scriptURL = scriptURL.substr(1, scriptURL.size());
      }

      String scope = "/";

      auto scopeParts = split(scriptURL, "/");
      if (scopeParts.size() > 0) {
        scopeParts = Vector<String>(scopeParts.begin(), scopeParts.end() - 1);
        scope = join(scopeParts, "/");
      }

      scriptURL = (
    #if SSC_PLATFORM_ANDROID
        "https://" +
    #else
        "socket://" +
    #endif
        bridge->userConfig["meta_bundle_identifier"] +
        scriptURL
      );

      this->bridge->core->serviceWorker.registerServiceWorker({
        .type = ServiceWorkerContainer::RegistrationOptions::Type::Module,
        .scope = scope,
        .scriptURL = scriptURL,
        .scheme = scheme,
        .id = id
      });

      this->schemeHandlers.registerSchemeHandler(scheme, [this](
        const auto& request,
        const auto router,
        auto& callbacks,
        auto callback
      ) {
        if (this->core->serviceWorker.registrations.size() > 0) {
          auto hostname = request.hostname;
          auto pathname = request.pathname;

          if (request.scheme == "npm") {
            hostname = this->bridge->userConfig["meta_bundle_identifier"];
          }

          const auto scope = this->core->protocolHandlers.getServiceWorkerScope(request.scheme);

          if (scope.size() > 0) {
            pathname = scope + pathname;
          }

          const auto fetch = ServiceWorkerContainer::FetchRequest {
            request.method,
            request.scheme,
            hostname,
            pathname,
            request.query,
            request.headers,
            ServiceWorkerContainer::FetchBuffer { request.body.size, request.body.bytes },
            ServiceWorkerContainer::Client { request.client.id }
          };

          const auto fetched = this->core->serviceWorker.fetch(fetch, [request, callback] (auto res) mutable {
            if (!request.isActive()) {
              return;
            }

            auto response = SchemeHandlers::Response(request);
            response.writeHead(res.statusCode, res.headers);
            response.write(res.buffer.size, res.buffer.bytes);
            callback(response);
          });

          if (fetched) {
            this->bridge->core->setTimeout(32000, [request] () mutable {
              if (request.isActive()) {
                auto response = SchemeHandlers::Response(request, 408);
                response.fail("Protocol handler ServiceWorker request timed out.");
              }
            });
            return;
          }
        }

        auto response = SchemeHandlers::Response(request);
        response.writeHead(404);
        callback(response);
      });
    }
  }
}
