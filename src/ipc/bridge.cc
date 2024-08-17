#include "../serviceworker/protocols.hh"
#include "../extension/extension.hh"
#include "../window/window.hh"
#include "../core/version.hh"
#include "../app/app.hh"
#include "ipc.hh"

extern const SSC::Map SSC::getUserConfig ();
extern bool SSC::isDebugEnabled ();

namespace SSC::IPC {
  static Vector<Bridge*> instances;
  static Mutex mutex;

  // The `ESM_IMPORT_PROXY_TEMPLATE` is used to provide an ESM module as
  // a proxy to a canonical URL for a module import.
  static constexpr auto ESM_IMPORT_PROXY_TEMPLATE_WITH_DEFAULT_EXPORT = R"S(
/**
 * This module exists to provide a proxy to a canonical URL for a module
 * so `{{protocol}}:{{specifier}}` and `{{protocol}}://{bundle_identifier}/socket/{{pathname}}`
 * resolve to the exact same module instance.
 * @see {@link https://github.com/socketsupply/socket/blob/{{commit}}/api{{pathname}}}
 */
import module from '{{url}}'
export * from '{{url}}'
export default module
)S";

  static constexpr auto ESM_IMPORT_PROXY_TEMPLATE_WITHOUT_DEFAULT_EXPORT = R"S(
/**
 * This module exists to provide a proxy to a canonical URL for a module
 * so `{{protocol}}:{{specifier}}` and `{{protocol}}://{bundle_identifier}/socket/{{pathname}}`
 * resolve to the exact same module instance.
 * @see {@link https://github.com/socketsupply/socket/blob/{{commit}}/api{{pathname}}}
 */
export * from '{{url}}'
)S";

  static const Vector<String> allowedNodeCoreModules = {
    "async_hooks",
    "assert",
    "buffer",
    "console",
    "constants",
    "child_process",
    "crypto",
    "dgram",
    "dns",
    "dns/promises",
    "events",
    "fs",
    "fs/constants",
    "fs/promises",
    "http",
    "https",
    "ip",
    "module",
    "net",
    "os",
    "os/constants",
    "path",
    "path/posix",
    "path/win32",
    "perf_hooks",
    "process",
    "querystring",
    "stream",
    "stream/web",
    "string_decoder",
    "sys",
    "test",
    "timers",
    "timers/promises",
    "tty",
    "url",
    "util",
    "vm",
    "worker_threads"
  };

#if SOCKET_RUNTIME_PLATFORM_DESKTOP
  static FileSystemWatcher* developerResourcesFileSystemWatcher = nullptr;
  static void initializeDeveloperResourcesFileSystemWatcher (SharedPointer<Core> core) {
    auto defaultUserConfig = SSC::getUserConfig();
    if (
      developerResourcesFileSystemWatcher == nullptr &&
      isDebugEnabled() &&
      defaultUserConfig["webview_watch"] == "true"
    ) {
      developerResourcesFileSystemWatcher = new FileSystemWatcher(getcwd());
      developerResourcesFileSystemWatcher->core = core.get();
      developerResourcesFileSystemWatcher->start([=](
        const auto& path,
        const auto& events,
        const auto& context
      ) mutable {
        Lock lock(SSC::IPC::mutex);

        static const auto cwd = getcwd();
        const auto relativePath = fs::relative(path, cwd).string();
        const auto json = JSON::Object::Entries {{"path", relativePath}};
        const auto result = SSC::IPC::Result(json);

        for (auto& bridge : instances) {
          auto userConfig = bridge->userConfig;
          if (
            !platform.ios &&
            !platform.android &&
            userConfig["webview_watch"] == "true" &&
            bridge->userConfig["webview_service_worker_mode"] != "hybrid" &&
            (!userConfig.contains("webview_watch_reload") || userConfig.at("webview_watch_reload") != "false")
          ) {
            // check if changed path was a service worker, if so unregister it so it can be reloaded
            for (const auto& entry : App::sharedApplication()->serviceWorkerContainer.registrations) {
              const auto& registration = entry.second;
            #if SOCKET_RUNTIME_PLATFORM_ANDROID
              auto scriptURL = String("https://");
            #else
              auto scriptURL = String("socket://");
            #endif

              scriptURL += userConfig["meta_bundle_identifier"];

              if (!relativePath.starts_with("/")) {
                scriptURL += "/";
              }

              scriptURL += relativePath;
              if (registration.scriptURL == scriptURL) {
                // 1. unregister service worker
                // 2. re-register service worker
                // 3. wait for it to be registered
                // 4. emit 'filedidchange' event
                bridge->navigator.serviceWorker.unregisterServiceWorker(entry.first);
                bridge->core->setTimeout(8, [bridge, result, &registration] () {
                  bridge->core->setInterval(8, [bridge, result, &registration] (auto cancel) {
                    if (registration.state == ServiceWorkerContainer::Registration::State::Activated) {
                      cancel();

                      uint64_t timeout = 500;
                      if (bridge->userConfig["webview_watch_service_worker_reload_timeout"].size() > 0) {
                        try {
                          timeout = std::stoull(bridge->userConfig["webview_watch_service_worker_reload_timeout"]);
                        } catch (...) {}
                      }

                      bridge->core->setTimeout(timeout, [bridge, result] () {
                        bridge->emit("filedidchange", result.json().str());
                      });
                    }
                  });

                  bridge->navigator.serviceWorker.registerServiceWorker(registration.options);
                });
                return;
              }
            }
          }

          bridge->emit("filedidchange", result.json().str());
        }
      });
    }
  }
#endif
  Bridge::Options::Options (
    const Map& userConfig,
    const Preload::Options& preload
  ) : userConfig(userConfig),
      preload(preload)
  {}

  Bridge::Bridge (
    SharedPointer<Core> core,
    const Options& options
  ) : core(core),
      userConfig(options.userConfig),
      router(this),
      navigator(this),
      schemeHandlers(this)
  {
    this->id = rand64();
    this->client.id = this->id;
  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    this->isAndroidEmulator = App::sharedApplication()->isAndroidEmulator;
  #endif

    this->bluetooth.sendFunction = [this](
      const String& seq,
      const JSON::Any value,
      const SSC::Post post
    ) {
      this->send(seq, value.str(), post);
    };

    this->bluetooth.emitFunction = [this](
      const String& seq,
      const JSON::Any value
    ) {
      this->emit(seq, value.str());
    };

    this->dispatchFunction = [] (auto callback) {
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      callback();
    #else
      App::sharedApplication()->dispatch(callback);
    #endif
    };

    core->networkStatus.addObserver(this->networkStatusObserver, [this](auto json) {
      if (json.has("name")) {
        this->emit(json["name"].str(), json.str());
      }
    });

    core->networkStatus.start();

    core->geolocation.addPermissionChangeObserver(this->geolocationPermissionChangeObserver, [this] (auto json) {
      JSON::Object event = JSON::Object::Entries {
        {"name", "geolocation"},
        {"state", json["state"]}
      };
      this->emit("permissionchange", event.str());
    });

    // on Linux, much of the Notification API is supported so these observers
    // below are not needed as those events already occur in the webview
    // we are patching for the other platforms
  #if !SOCKET_RUNTIME_PLATFORM_LINUX
    core->notifications.addPermissionChangeObserver(this->notificationsPermissionChangeObserver, [this](auto json) {
      JSON::Object event = JSON::Object::Entries {
        {"name", "notifications"},
        {"state", json["state"]}
      };
      this->emit("permissionchange", event.str());
    });

    if (userConfig["permissions_allow_notifications"] != "false") {
      core->notifications.addNotificationResponseObserver(this->notificationResponseObserver, [this](auto json) {
        this->emit("notificationresponse", json.str());
      });

      core->notifications.addNotificationPresentedObserver(this->notificationPresentedObserver, [this](auto json) {
        this->emit("notificationpresented", json.str());
      });
    }
  #endif

    Lock lock(SSC::IPC::mutex);
    instances.push_back(this);
  #if SOCKET_RUNTIME_PLATFORM_DESKTOP
    initializeDeveloperResourcesFileSystemWatcher(core);
  #endif
  }

  Bridge::~Bridge () {
    // remove observers
    core->geolocation.removePermissionChangeObserver(this->geolocationPermissionChangeObserver);
    core->networkStatus.removeObserver(this->networkStatusObserver);
    core->notifications.removePermissionChangeObserver(this->notificationsPermissionChangeObserver);
    core->notifications.removeNotificationResponseObserver(this->notificationResponseObserver);
    core->notifications.removeNotificationPresentedObserver(this->notificationPresentedObserver);

    do {
      Lock lock(SSC::IPC::mutex);
      const auto cursor = std::find(instances.begin(), instances.end(), this);
      if (cursor != instances.end()) {
        instances.erase(cursor);
      }

      #if SOCKET_RUNTIME_PLATFORM_DESKTOP
        if (instances.size() == 0) {
          if (developerResourcesFileSystemWatcher) {
            developerResourcesFileSystemWatcher->stop();
            delete developerResourcesFileSystemWatcher;
          }
        }
      #endif
    } while (0);
  }

  void Bridge::init () {
    this->router.init();
    this->navigator.init();
    this->schemeHandlers.init();
  }

  void Bridge::configureWebView (WebView* webview) {
    this->navigator.configureWebView(webview);
  }

  bool Bridge::evaluateJavaScript (const String& source) {
    if (this->core->isShuttingDown) {
      return false;
    }

    if (this->evaluateJavaScriptFunction != nullptr) {
      this->evaluateJavaScriptFunction(source);
      return true;
    }

    return false;
  }

  bool Bridge::dispatch (const DispatchCallback& callback) {
    if (!this->core || this->core->isShuttingDown) {
      return false;
    }

    if (this->dispatchFunction != nullptr) {
      this->dispatchFunction(callback);
      return true;
    }

    return false;
  }

  bool Bridge::navigate (const String& url) {
    if (!this->core || this->core->isShuttingDown) {
      return false;
    }

    if (this->navigateFunction != nullptr) {
      this->navigateFunction(url);
      return true;
    }

    return false;
  }

  bool Bridge::route (const String& uri, SharedPointer<char[]> bytes, size_t size) {
    return this->route(uri, bytes, size, nullptr);
  }

  bool Bridge::route (
    const String& uri,
    SharedPointer<char[]> bytes,
    size_t size,
    Router::ResultCallback callback
  ) {
    if (callback != nullptr) {
      return this->router.invoke(uri, bytes, size, callback);
    } else {
      return this->router.invoke(uri, bytes, size);
    }
  }

  bool Bridge::send (
    const Message::Seq& seq,
    const String& data,
    const Post& post
  ) {
    if (this->core->isShuttingDown) {
      return false;
    }

    if (post.body != nullptr || seq == "-1") {
      const auto script = this->core->createPost(seq, data, post);
      return this->evaluateJavaScript(script);
    }

    const auto value = encodeURIComponent(data);
    const auto script = getResolveToRenderProcessJavaScript(
      seq.size() == 0 ? "-1" : seq,
      "0",
      value
    );

    return this->evaluateJavaScript(script);
  }

  bool Bridge::send (const Message::Seq& seq, const JSON::Any& json, const Post& post) {
    return this->send(seq, json.str(), post);
  }

  bool Bridge::emit (const String& name, const String& data) {
    if (this->core->isShuttingDown) {
      return false;
    }

    const auto value = encodeURIComponent(data);
    const auto script = getEmitToRenderProcessJavaScript(name, value);
    return this->evaluateJavaScript(script);
  }

  bool Bridge::emit (const String& name, const JSON::Any& json) {
    return this->emit(name, json.str());
  }

  void Bridge::configureSchemeHandlers (
    const SchemeHandlers::Configuration& configuration
  ) {
    this->schemeHandlers.configure(configuration);
    this->schemeHandlers.registerSchemeHandler("ipc", [this](
      const auto request,
      const auto bridge,
      auto callbacks,
      auto callback
    ) {
      auto message = Message(request->url(), true);

      if (request->method == "OPTIONS") {
        auto response = SchemeHandlers::Response(request, 204);
        return callback(response);
      }

      message.isHTTP = true;
      message.cancel = std::make_shared<MessageCancellation>();

      callbacks->cancel = [message] () {
        if (message.cancel->handler != nullptr) {
          message.cancel->handler(message.cancel->data);
        }
      };

      const auto size = request->body.size;
      const auto bytes = request->body.bytes;
      const auto invoked = this->router.invoke(message.str(), request->body.bytes, size, [=](Result result) {
        if (!request->isActive()) {
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
            if (request->isCancelled()) {
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
            if (request->isCancelled()) {
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
            {"url", request->url()}
          }}
        });

        return callback(response);
      }
    });

    this->schemeHandlers.registerSchemeHandler("socket", [this](
      const auto request,
      const auto bridge,
      auto callbacks,
      auto callback
    ) {
      auto userConfig = this->userConfig;
      auto bundleIdentifier = userConfig["meta_bundle_identifier"];
      auto app = App::sharedApplication();
      auto window = app->windowManager.getWindowForBridge(bridge);

      // if there was no window, then this is a bad request as scheme
      // handlers should only be handled directly in a window with
      // a navigator and a connected IPC bridge
      if (window == nullptr) {
        auto response = SchemeHandlers::Response(request);
        response.writeHead(400);
        callback(response);
        return;
      }

      if (request->method == "OPTIONS") {
        auto response = SchemeHandlers::Response(request);
        response.writeHead(204);
        callback(response);
        return;
      }

      // the location of static application resources
      const auto applicationResources = FileResource::getResourcesPath().string();
      // default response is 404
      auto response = SchemeHandlers::Response(request, 404);

      // the resouce path that may be request
      String resourcePath;

      // the content location relative to the request origin
      String contentLocation;

      // application resource or service worker request at `socket://<bundle_identifier>/*`
      if (request->hostname == bundleIdentifier) {
        const auto resolved = this->navigator.location.resolve(request->pathname, applicationResources);

        if (resolved.redirect) {
          if (request->method == "GET") {
            auto location = resolved.pathname;
            if (request->query.size() > 0) {
              location += "?" + request->query;
            }

            if (request->fragment.size() > 0) {
              location += "#" + request->fragment;
            }

            response.redirect(location);
            return callback(response);
          }
        } else if (resolved.isResource()) {
          resourcePath = applicationResources + resolved.pathname;
        } else if (resolved.isMount()) {
          resourcePath = resolved.mount.filename;
        } else if (request->pathname == "" || request->pathname == "/") {
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
          if (resourcePath.starts_with(applicationResources)) {
            contentLocation = resourcePath.substr(applicationResources.size(), resourcePath.size());
          }

          auto resource = FileResource(resourcePath);

          if (!resource.exists()) {
            response.writeHead(404);
          } else {
            if (contentLocation.size() > 0) {
              response.setHeader("content-location", contentLocation);
            }

            if (request->method == "OPTIONS") {
              response.writeHead(204);
            }

            if (request->method == "HEAD") {
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

            if (request->method == "GET") {
              if (resource.mimeType() != "text/html") {
                response.setHeader("cache-control", "public");
                response.send(resource);
              } else {
                const auto html = this->client.preload.insertIntoHTML(resource.str(), {
                  .protocolHandlerSchemes = this->navigator.serviceWorker.protocols.getSchemes()
                });

                response.setHeader("content-type", "text/html");
                response.setHeader("content-length", html.size());
                response.setHeader("cache-control", "public");
                response.writeHead(200);
                response.write(html);
              }
            }
          }

          return callback(response);
        }

        if (this->navigator.serviceWorker.registrations.size() > 0) {
          const auto fetch = ServiceWorkerContainer::FetchRequest {
            request->method,
            request->scheme,
            request->hostname,
            request->pathname,
            request->query,
            request->headers,
            ServiceWorkerContainer::FetchBody { request->body.size, request->body.bytes },
            request->client
          };

          const auto fetched = this->navigator.serviceWorker.fetch(fetch, [request, callback, response] (auto res) mutable {
            if (!request->isActive()) {
              return;
            }

            if (res.statusCode == 0) {
              response.fail("ServiceWorker request failed");
            } else {
              response.writeHead(res.statusCode, res.headers);
              response.write(res.body.size, res.body.bytes);
            }

            callback(response);
          });

          if (fetched) {
            this->core->setTimeout(32000, [request] () mutable {
              if (request->isActive()) {
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
      if (request->hostname.size() == 0) {
        auto pathname = request->pathname;

        if (pathname.ends_with("/")) {
          pathname = pathname.substr(0, pathname.size() - 1);
        }

        const auto specifier = pathname.substr(1);

        if (!pathname.ends_with(".js")) {
          pathname += ".js";
        }

        if (!pathname.starts_with("/")) {
          pathname = "/" + pathname;
        }

        resourcePath = applicationResources + "/socket" + pathname;
        contentLocation = "/socket" + pathname;

        auto resource = FileResource(resourcePath, { .cache = true });

        if (resource.exists()) {
          const auto url = (
            #if SOCKET_RUNTIME_PLATFORM_ANDROID
            "https://" +
            #else
            "socket://" +
            #endif
            bundleIdentifier +
            contentLocation +
            (request->query.size() > 0 ? "?" + request->query : "")
          );

          const auto moduleImportProxy = tmpl(
            String(resource.read()).find("export default") != String::npos
              ? ESM_IMPORT_PROXY_TEMPLATE_WITH_DEFAULT_EXPORT
              : ESM_IMPORT_PROXY_TEMPLATE_WITHOUT_DEFAULT_EXPORT,
            Map {
              {"url", url},
              {"commit", VERSION_HASH_STRING},
              {"protocol", "socket"},
              {"pathname", pathname},
              {"specifier", specifier},
              {"bundle_identifier", bundleIdentifier}
            }
          );

          const auto contentType = resource.mimeType();

          if (contentType.size() > 0) {
            response.setHeader("content-type", contentType);
          }

          response.setHeader("content-length", moduleImportProxy.size());

          if (contentLocation.size() > 0) {
            response.setHeader("content-location", contentLocation);
          }

          response.writeHead(200);
          response.write(moduleImportProxy);
          return callback(response);
        }
        response.setHeader("content-type", "text/javascript");
      }

      response.writeHead(404);
      callback(response);
    });

    this->schemeHandlers.registerSchemeHandler("node", [this](
      const auto request,
      const auto router,
      auto callbacks,
      auto callback
    ) {
      if (request->method == "OPTIONS") {
        auto response = SchemeHandlers::Response(request);
        response.writeHead(204);
        callback(response);
        return;
      }

      auto userConfig = this->userConfig;
      auto bundleIdentifier = userConfig["meta_bundle_identifier"];
      // the location of static application resources
      const auto applicationResources = FileResource::getResourcesPath().string();
      // default response is 404
      auto response = SchemeHandlers::Response(request, 404);

      // the resouce path that may be request
      String resourcePath;

      // the content location relative to the request origin
      String contentLocation;

      // module or stdlib import/fetch `socket:<module>/<path>` which will just
      // proxy an import into a normal resource request above
      if (request->hostname.size() == 0) {
        const auto isAllowedNodeCoreModule = allowedNodeCoreModules.end() != std::find(
          allowedNodeCoreModules.begin(),
          allowedNodeCoreModules.end(),
          request->pathname.substr(1)
        );

        if (!isAllowedNodeCoreModule) {
          response.writeHead(404);
          return callback(response);
        }

        auto pathname = request->pathname;

        if (!pathname.ends_with(".js")) {
          pathname += ".js";
        }

        if (!pathname.starts_with("/")) {
          pathname = "/" + pathname;
        }

        contentLocation = "/socket" + pathname;
        resourcePath = applicationResources + contentLocation;

        auto resource = FileResource(resourcePath, { .cache = true });

        if (!resource.exists()) {
          if (!pathname.ends_with(".js")) {
            pathname = request->pathname;

            if (!pathname.starts_with("/")) {
              pathname = "/" + pathname;
            }

            if (pathname.ends_with("/")) {
              pathname = pathname.substr(0, pathname.size() - 1);
            }

            contentLocation = "/socket" + pathname + "/index.js";
            resourcePath = applicationResources + contentLocation;
          }

          resource = FileResource(resourcePath, { .cache = true });
        }

        if (resource.exists()) {
          const auto url = (
            #if SOCKET_RUNTIME_PLATFORM_ANDROID
            "https://" +
            #else
            "socket://" +
            #endif
            bundleIdentifier +
            "/socket" +
            pathname
          );
          const auto moduleImportProxy = tmpl(
            String(resource.read()).find("export default") != String::npos
              ? ESM_IMPORT_PROXY_TEMPLATE_WITH_DEFAULT_EXPORT
              : ESM_IMPORT_PROXY_TEMPLATE_WITHOUT_DEFAULT_EXPORT,
            Map {
              {"url", url},
              {"commit", VERSION_HASH_STRING},
              {"protocol", "node"},
              {"pathname", pathname},
              {"specifier", pathname.substr(1)},
              {"bundle_identifier", bundleIdentifier}
            }
          );

          const auto contentType = resource.mimeType();

          if (contentType.size() > 0) {
            response.setHeader("content-type", contentType);
          }

          response.setHeader("content-length", moduleImportProxy.size());

          if (contentLocation.size() > 0) {
            response.setHeader("content-location", contentLocation);
          }

          response.writeHead(200);
          response.write(trim(moduleImportProxy));
        }

        return callback(response);
      }

      response.writeHead(404);
      callback(response);
    });

    Map protocolHandlers = {
      {"npm", "/socket/npm/service-worker.js"}
    };

    for (const auto& entry : split(this->userConfig["webview_protocol-handlers"], " ")) {
      const auto scheme = replace(trim(entry), ":", "");
      if (this->navigator.serviceWorker.protocols.registerHandler(scheme)) {
        protocolHandlers.insert_or_assign(scheme, "");
      }
    }

    for (const auto& entry : this->userConfig) {
      const auto& key = entry.first;
      if (key.starts_with("webview_protocol-handlers_")) {
        const auto scheme = replace(replace(trim(key), "webview_protocol-handlers_", ""), ":", "");;
        const auto data = entry.second;
        if (this->navigator.serviceWorker.protocols.registerHandler(scheme, { data })) {
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
      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        "https://" +
      #else
        "socket://" +
      #endif
        this->userConfig["meta_bundle_identifier"] +
        scriptURL
      );

      this->navigator.serviceWorker.registerServiceWorker({
        .type = ServiceWorkerContainer::RegistrationOptions::Type::Module,
        .scope = scope,
        .scriptURL = scriptURL,
        .scheme = scheme,
        .id = id
      });

      this->schemeHandlers.registerSchemeHandler(scheme, [this](
        auto request,
        auto bridge,
        auto callbacks,
        auto callback
      ) {
        auto app = App::sharedApplication();
        auto window = app->windowManager.getWindowForBridge(bridge);

        if (window == nullptr) {
          auto response = SchemeHandlers::Response(request);
          response.writeHead(400);
          callback(response);
          return;
        }

        if (request->method == "OPTIONS") {
          auto response = SchemeHandlers::Response(request);
          response.writeHead(204);
          callback(response);
          return;
        }

        if (this->navigator.serviceWorker.registrations.size() > 0) {
          auto hostname = request->hostname;
          auto pathname = request->pathname;

          if (request->scheme == "npm") {
            if (hostname.size() > 0) {
              pathname = "/" + hostname;
            }
            hostname = this->userConfig["meta_bundle_identifier"];
          }

          const auto scope = this->navigator.serviceWorker.protocols.getServiceWorkerScope(request->scheme);

          if (scope.size() > 0) {
            pathname = scope + pathname;
          }

          const auto fetch = ServiceWorkerContainer::FetchRequest {
            request->method,
            request->scheme,
            hostname,
            pathname,
            request->query,
            request->headers,
            ServiceWorkerContainer::FetchBody { request->body.size, request->body.bytes },
            request->client
          };

          const auto fetched = this->navigator.serviceWorker.fetch(fetch, [request, callback] (auto res) mutable {
            if (!request->isActive()) {
              return;
            }

            auto response = SchemeHandlers::Response(request);

            if (res.statusCode == 0) {
              response.fail("ServiceWorker request failed");
            } else {
              response.writeHead(res.statusCode, res.headers);
              response.write(res.body.size, res.body.bytes);
            }

            callback(response);
          });

          if (fetched) {
            this->core->setTimeout(32000, [request] () mutable {
              if (request->isActive()) {
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
  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    static const int MAX_ALLOWED_SCHEME_ORIGINS = 64;
    static const int MAX_CUSTOM_SCHEME_REGISTRATIONS = 64;
    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions4> options;
    Vector<SharedPointer<WString>> schemes;
    Vector<SharedPointer<WString>> origins;

    if (this->schemeHandlers.configuration.webview.As(&options) == S_OK) {
      ICoreWebView2CustomSchemeRegistration* registrations[MAX_CUSTOM_SCHEME_REGISTRATIONS] = {};
      const WCHAR* allowedOrigins[MAX_ALLOWED_SCHEME_ORIGINS] = {};

      int registrationsCount = 0;
      int allowedOriginsCount = 0;

      allowedOrigins[allowedOriginsCount++] = L"about://*";
      allowedOrigins[allowedOriginsCount++] = L"https://*";

      for (const auto& entry : this->schemeHandlers.handlers) {
        const auto origin = entry.first + "://*";
        origins.push_back(std::make_shared<WString>(convertStringToWString(origin)));
        allowedOrigins[allowedOriginsCount++] = origins.back()->c_str();
      }

    	// store registratino refs here
      Set<Microsoft::WRL::ComPtr<CoreWebView2CustomSchemeRegistration>> registrationsSet;

      for (const auto& entry : this->schemeHandlers.handlers) {
        schemes.push_back(std::make_shared<WString>(convertStringToWString(entry.first)));
        auto registration = Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(
          schemes.back()->c_str()
        );

        registration->SetAllowedOrigins(allowedOriginsCount, allowedOrigins);
	if (entry.first != "npm") {
          registration->put_HasAuthorityComponent(true);
	}
        registration->put_TreatAsSecure(true);
        registrations[registrationsCount++] = registration.Get();
        registrationsSet.insert(registration);
      }

      options->SetCustomSchemeRegistrations(
        registrationsCount,
        static_cast<ICoreWebView2CustomSchemeRegistration**>(registrations)
      );
    }
  #endif
  }

  void Bridge::configureNavigatorMounts () {
    this->navigator.configureMounts();
  }
}

#if SOCKET_RUNTIME_PLATFORM_ANDROID
extern "C" {
  jboolean ANDROID_EXTERNAL(ipc, Bridge, emit) (
    JNIEnv* env,
    jobject self,
    jint index,
    jstring eventString,
    jstring dataString
  ) {
    using namespace SSC;
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    const auto window = app->windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window requested");
      return false;
    }

    const auto event = Android::StringWrap(env, eventString).str();
    const auto data = Android::StringWrap(env, dataString).str();
    return window->bridge.emit(event, data);
  }
}
#endif
