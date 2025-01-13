#include "../serviceworker.hh"
#include "../filesystem.hh"
#include "../javascript.hh"
#include "../platform.hh"
#include "../runtime.hh"
#include "../version.hh"
#include "../webview.hh"
#include "../string.hh"
#include "../window.hh"
#include "../cwd.hh"
#include "../env.hh"
#include "../ipc.hh"
#include "../url.hh"
#include "../app.hh"

#include "../bridge.hh"

using ssc::runtime::javascript::getResolveToRenderProcessJavaScript;
using ssc::runtime::javascript::getEmitToRenderProcessJavaScript;
using ssc::runtime::url::encodeURIComponent;
using ssc::runtime::webview::SchemeHandlers;

using ssc::runtime::config::isDebugEnabled;
using ssc::runtime::config::getUserConfig;

using ssc::runtime::string::parseStringList;
using ssc::runtime::string::toLowerCase;
using ssc::runtime::string::replace;
using ssc::runtime::string::split;
using ssc::runtime::string::tmpl;
using ssc::runtime::string::trim;
using ssc::runtime::string::join;

using ssc::runtime::crypto::rand64;

using ssc::runtime::app::App;

namespace ssc::runtime::bridge {
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

  Bridge::Bridge (
    const Options& options
  ) : window::IBridge(options.dispatcher, options.context, options.client, options.userConfig),
      schemeHandlers(*this),
      navigator(*this),
      router(*this)
  {
    // '-1' may mean the bridge is running as a WebKit web process extension
    if (options.client.index >= 0) {
      const auto windowClientConfigKey = String("window.") + std::to_string(options.client.index) + ".client";

      // handle user defined window client id
      if (this->userConfig[windowClientConfigKey + ".id"].size() > 0) {
        try {
          this->client.id = std::stoull(this->userConfig[windowClientConfigKey + ".id"]);
        } catch (...) {
          debug("Invalid window client ID given in '[window.%d.client] id'", options.client.index);
        }
      }
    }

    this->bluetooth.sendHandler = [this](
      const ipc::Message::Seq& seq,
      const JSON::Any value,
      const QueuedResponse queuedResponse
    ) {
      this->send(seq, value.str(), queuedResponse);
    };

    this->bluetooth.emitHandler = [this](
      const ipc::Message::Seq& seq,
      const JSON::Any value
    ) {
      this->emit(seq, value.str());
    };
  }

  Bridge::~Bridge () {
    auto app = App::sharedApplication();
    if (app && !app->shouldExit) {
      // remove observers
      app->runtime.services.geolocation.removePermissionChangeObserver(this->geolocationPermissionChangeObserver);
      app->runtime.services.networkStatus.removeObserver(this->networkStatusObserver);
      app->runtime.services.notifications.removePermissionChangeObserver(this->notificationsPermissionChangeObserver);
      app->runtime.services.notifications.removeNotificationResponseObserver(this->notificationResponseObserver);
      app->runtime.services.notifications.removeNotificationPresentedObserver(this->notificationPresentedObserver);
    }
  }

  void Bridge::init () {
    auto& runtime = static_cast<runtime::Runtime&>(this->context);

    runtime.services.networkStatus.addObserver(this->networkStatusObserver, [this](auto json) {
      if (json.has("name")) {
        this->emit(json["name"].str(), json.str());
      }
    });

    runtime.services.networkStatus.start();

    runtime.services.geolocation.addPermissionChangeObserver(this->geolocationPermissionChangeObserver, [this] (auto json) {
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
    runtime.services.notifications.addPermissionChangeObserver(this->notificationsPermissionChangeObserver, [this](auto json) {
      JSON::Object event = JSON::Object::Entries {
        {"name", "notifications"},
        {"state", json["state"]}
      };
      this->emit("permissionchange", event.str());
    });

    if (this->userConfig["permissions_allow_notifications"] != "false") {
      runtime.services.notifications.addNotificationResponseObserver(this->notificationResponseObserver, [this](auto json) {
        this->emit("notificationresponse", json.str());
      });

      runtime.services.notifications.addNotificationPresentedObserver(this->notificationPresentedObserver, [this](auto json) {
        this->emit("notificationpresented", json.str());
      });
    }
  #endif
    this->router.init();
    this->navigator.init();
    this->schemeHandlers.init();
  }

  void Bridge::configureWebView (webview::WebView* webview) {
    this->navigator.configureWebView(webview);
  }

  bool Bridge::evaluateJavaScript (const String& source) {
    if (this->evaluateJavaScriptHandler != nullptr) {
      this->evaluateJavaScriptHandler(source);
      return true;
    }

    return false;
  }

  bool Bridge::dispatch (const context::DispatchCallback callback) {
    if (this->dispatchHandler != nullptr) {
      this->dispatchHandler(callback);
      return true;
    }
    return static_cast<Runtime&>(this->context).dispatch(callback);
  }

  bool Bridge::navigate (const String& url) {
    if (this->navigateHandler != nullptr) {
      this->navigateHandler(url);
      return true;
    }

    return false;
  }

  bool Bridge::route (const String& uri, SharedPointer<unsigned char[]> bytes, size_t size) {
    return this->route(uri, bytes, size, nullptr);
  }

  bool Bridge::route (
    const String& uri,
    SharedPointer<unsigned char[]> bytes,
    size_t size,
    ipc::Router::ResultCallback callback
  ) {
    if (callback != nullptr) {
      return this->router.invoke(uri, bytes, size, callback);
    } else {
      return this->router.invoke(uri, bytes, size);
    }
  }

  bool Bridge::route (const String& uri, const bytes::Buffer& buffer) {
    return this->route(uri, buffer, nullptr);
  }

  bool Bridge::route (const String& uri, const bytes::Buffer& buffer, const ipc::Router::ResultCallback callback) {
    return this->route(
      uri,
      buffer.pointer(),
      buffer.size(),
      callback
    );
  }

  bool Bridge::send (
    const ipc::Message::Seq& seq,
    const String& data,
    const QueuedResponse& queuedResponse
  ) {
    if (queuedResponse.body != nullptr || seq == "-1") {
      const auto script = this->context.createQueuedResponse(seq, data, queuedResponse);
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

  bool Bridge::send (const ipc::Message::Seq& seq, const JSON::Any& json, const QueuedResponse& queuedResponse) {
    return this->send(seq, json.str(), queuedResponse);
  }

  bool Bridge::emit (const String& name, const String& data) {
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
      const auto& bridge,
      auto callbacks,
      auto callback
    ) {
      auto message = ipc::Message(request->url());

      if (request->method == "OPTIONS") {
        auto response = SchemeHandlers::Response(request, 204);
        return callback(response);
      }

      message.isHTTP = true;
      message.cancel = std::make_shared<ipc::MessageCancellation>();

      callbacks->cancel = [message] () {
        if (message.cancel->handler != nullptr) {
          message.cancel->handler(message.cancel->data);
        }
      };

      const auto size = request->body.size();
      const auto invoked = this->router.invoke(message, request->body.pointer(), size, [=](ipc::Result result) {
        if (!request->isActive()) {
          return;
        }

        auto response = SchemeHandlers::Response(request);

        response.setHeaders(result.headers);

        // handle event source streams
        if (result.queuedResponse.eventStreamCallback != nullptr) {
          response.setHeader("content-type", "text/event-stream");
          response.setHeader("cache-control", "no-store");
          *result.queuedResponse.eventStreamCallback = [request, response, message, callback](
            const char* name,
            const unsigned char* data,
            bool finished
          ) mutable {
            if (request->isCancelled()) {
              if (message.cancel->handler != nullptr) {
                message.cancel->handler(message.cancel->data);
              }
              return false;
            }

            response.writeHead(200);

            const auto event = SchemeHandlers::Response::Event { name, reinterpret_cast<const char*>(data) };

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
        if (result.queuedResponse.chunkStreamCallback != nullptr) {
          response.setHeader("transfer-encoding", "chunked");
          *result.queuedResponse.chunkStreamCallback = [request, response, message, callback](
            const unsigned char* chunk,
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

        if (result.queuedResponse.body != nullptr) {
          response.write(result.queuedResponse.length, result.queuedResponse.body);
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
      const auto& bridge,
      auto callbacks,
      auto callback
    ) {
      auto app = App::sharedApplication();
      auto globalConfig = getUserConfig();
      auto userConfig = this->userConfig;
      auto bundleIdentifier = userConfig["meta_bundle_identifier"];
      auto globalBundleIdentifier = globalConfig["meta_bundle_identifier"];
      auto window = app->runtime.windowManager.getWindowForBridge(&bridge);

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
      const auto applicationResources = window->options.resourcesDirectory.size() > 0
        ? fs::absolute(window->options.resourcesDirectory).string()
        : filesystem::Resource::getResourcesPath().string();
      // default response is 404
      auto response = SchemeHandlers::Response(request, 404);

      // the resouce path that may be request
      String resourcePath;

      // the content location relative to the request origin
      String contentLocation;

      // application resource or service worker request at `socket://<bundle_identifier>/*`
      if (request->hostname.size() > 0) {
        auto origin = webview::Origin(request->scheme + "://" + request->hostname);
        auto serviceWorker = app->runtime.serviceWorkerManager.get(origin.name());

        if (
          serviceWorker != nullptr &&
          request->hostname != globalBundleIdentifier &&
          window->options.shouldPreferServiceWorker &&
          serviceWorker->container.registrations.size() > 0
        ) {
          auto fetch = serviceworker::Request();
          fetch.method = request->method;
          fetch.scheme = request->scheme;
          fetch.url.scheme = request->scheme;
          fetch.url.hostname = request->hostname;
          fetch.url.pathname = request->pathname;
          fetch.url.searchParams.set(request->query);
          fetch.url.search = request->query;
          fetch.headers = request->headers;
          fetch.body = request->body;
          fetch.client = request->client;

          if (!fetch.headers.has("origin")) {
            fetch.headers.set("origin", this->navigator.location.origin);
          }

          const auto app = App::sharedApplication();
          const auto options = serviceworker::Fetch::Options { request->client };
          const auto fetched = serviceWorker->fetch(fetch, options, [=, this, &request] (auto res) mutable {
            if (!request->isActive()) {
              return;
            }

            if (res.statusCode == 0) {
              response.fail("ServiceWorker request failed");
            } else if (res.statusCode != 404) {
              response.writeHead(res.statusCode, res.headers);
              response.write(res.body.buffer);
            } else {
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

                auto resource = filesystem::Resource(resourcePath);

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
                      const auto html = request->headers["runtime-preload-injection"] == "disabled"
                        ? resource.str()
                        : this->client.preload.insertIntoHTML(resource.str(), {
                            .protocolHandlerSchemes = serviceWorker->container.protocols.getSchemes()
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
            }

            callback(response);
          });

          if (fetched) {
             this->getRuntime()->services.timers.setTimeout(32000, [&request] () mutable {
               if (request->isActive()) {
                 auto response = SchemeHandlers::Response(request, 408);
                 response.fail("ServiceWorker request timed out.");
               }
             });
            return;
          }
        }

        if (
          request->hostname == userConfig["meta_bundle_identifier"] ||
          request->hostname == globalConfig["meta_bundle_identifier"]
        ) {
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

            auto resource = filesystem::Resource(resourcePath);

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
                  const auto html = request->headers["runtime-preload-injection"] == "disabled"
                    ? resource.str()
                    : this->client.preload.insertIntoHTML(resource.str(), {
                        .protocolHandlerSchemes = serviceWorker
                          ? serviceWorker->container.protocols.getSchemes()
                          : Vector<String>()
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
        }

        if (serviceWorker && serviceWorker->container.registrations.size() > 0) {
          auto fetch = serviceworker::Request();
          fetch.method = request->method;
          fetch.scheme = request->scheme;
          fetch.url.scheme = request->scheme;
          fetch.url.hostname = request->hostname;
          fetch.url.pathname = request->pathname;
          fetch.url.searchParams.set(request->query);
          fetch.url.search = request->query;
          fetch.headers = request->headers;
          fetch.body = request->body;
          fetch.client = request->client;

          if (!fetch.headers.has("origin")) {
            fetch.headers.set("origin", this->navigator.location.origin);
          }

          const auto options = serviceworker::Fetch::Options { request->client };
          const auto fetched = serviceWorker->fetch(fetch, options, [request, callback, response] (auto res) mutable {
            if (!request->isActive()) {
              return;
            }

            if (res.statusCode == 0) {
              response.fail("ServiceWorker request failed");
            } else {
              response.writeHead(res.statusCode, res.headers);
              response.write(res.body.buffer);
            }

            callback(response);
          });

          if (fetched) {
            this->getRuntime()->services.timers.setTimeout(32000, [request] () mutable {
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

        auto resource = filesystem::Resource(resourcePath, { .cache = true });

        if (resource.exists()) {
          auto url = URL();
          #if SOCKET_RUNTIME_PLATFORM_ANDROID
          url.scheme = "https";
          #else
          url.scheme = "socket";
          #endif
          url.hostname = toLowerCase(bundleIdentifier);
          url.pathname = contentLocation;
          url.search = request->query;

          const auto moduleImportProxy = tmpl(
            String(reinterpret_cast<const char*>(resource.read())).find("export default") != String::npos
              ? ESM_IMPORT_PROXY_TEMPLATE_WITH_DEFAULT_EXPORT
              : ESM_IMPORT_PROXY_TEMPLATE_WITHOUT_DEFAULT_EXPORT,
            Map<String, String> {
              {"url", url.str()},
              {"commit", version::VERSION_HASH_STRING},
              {"protocol", "socket"},
              {"pathname", pathname},
              {"specifier", specifier},
              {"bundle_identifier", toLowerCase(bundleIdentifier)}
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
      const auto& bridge,
      auto callbacks,
      auto callback
    ) {
      if (request->method == "OPTIONS") {
        auto response = SchemeHandlers::Response(request);
        response.writeHead(204);
        callback(response);
        return;
      }

      auto globalUserConfig = getUserConfig();
      const auto bundleIdentifier = this->userConfig["meta_bundle_identifier"];
      // the location of static application resources
      const auto applicationResources = filesystem::Resource::getResourcesPath().string();
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

        auto resource = filesystem::Resource(resourcePath, { .cache = true });

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

          resource = filesystem::Resource(resourcePath, { .cache = true });
        }

        if (resource.exists()) {
          auto url = URL();
          #if SOCKET_RUNTIME_PLATFORM_ANDROID
          url.scheme = "https";
          #else
          url.scheme = "socket";
          #endif
          url.hostname = toLowerCase(bundleIdentifier);
          url.pathname = "/socket" + pathname;
          url.search = request->query;
          const auto moduleImportProxy = tmpl(
            String(reinterpret_cast<const char*>(resource.read())).find("export default") != String::npos
              ? ESM_IMPORT_PROXY_TEMPLATE_WITH_DEFAULT_EXPORT
              : ESM_IMPORT_PROXY_TEMPLATE_WITHOUT_DEFAULT_EXPORT,
            Map<String, String> {
              {"url", url.str()},
              {"commit", version::VERSION_HASH_STRING},
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

    Set<String> globalProtocolHandlers = { "npm" };
    Map<String, String> protocolHandlers = {};
    auto globalUserConfig = getUserConfig();
    protocolHandlers.insert({"npm", "/socket/npm/service-worker.js"});

    for (const auto& entry : split(globalUserConfig["webview_protocol-handlers"], " ")) {
      const auto scheme = replace(trim(entry), ":", "");
      if (this->navigator.serviceWorkerServer->container.protocols.registerHandler(scheme)) {
        protocolHandlers.insert_or_assign(scheme, "");
        globalProtocolHandlers.insert(scheme);
      }
    }

    for (const auto& entry : globalUserConfig) {
      const auto& key = entry.first;
      if (key.starts_with("webview_protocol-handlers_")) {
        const auto scheme = replace(replace(trim(key), "webview_protocol-handlers_", ""), ":", "");;
        const auto data = entry.second;
        if (this->navigator.serviceWorkerServer->container.protocols.registerHandler(scheme, { data })) {
          protocolHandlers.insert_or_assign(scheme, data);
          globalProtocolHandlers.insert(scheme);
        }
      }
    }

    for (const auto& entry : split(this->userConfig["webview_protocol-handlers"], " ")) {
      const auto scheme = replace(trim(entry), ":", "");
      if (this->navigator.serviceWorkerServer->container.protocols.registerHandler(scheme)) {
        protocolHandlers.insert_or_assign(scheme, "");
      }
    }

    for (const auto& entry : this->userConfig) {
      const auto& key = entry.first;
      if (key.starts_with("webview_protocol-handlers_")) {
        const auto scheme = replace(replace(trim(key), "webview_protocol-handlers_", ""), ":", "");;
        const auto data = entry.second;
        if (this->navigator.serviceWorkerServer->container.protocols.registerHandler(scheme, { data })) {
          protocolHandlers.insert_or_assign(scheme, data);
        }
      }
    }

    for (const auto& entry : protocolHandlers) {
      const auto& scheme = entry.first;
      const auto id = rand64();

      if (
        globalUserConfig["meta_bundle_identifier"] == this->userConfig["meta_bundle_identifier"] ||
        !globalProtocolHandlers.contains(scheme)
      ) {
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

        auto env = JSON::Object::Entries {};
        for (const auto& entry : this->userConfig) {
          if (entry.first.starts_with("env_")) {
            env[entry.first.substr(4)] = entry.second;
          } else if (entry.first == "build_env") {
            const auto keys = parseStringList(entry.second, { ',', ' ' });
            for (const auto& key : keys) {
              env[key] = env::get(key);
            }
          }
        }

        if (scheme == "npm") {
          if (globalUserConfig["meta_bundle_identifier"] == this->userConfig["meta_bundle_identifier"]) {
            this->navigator.serviceWorkerServer->container.registerServiceWorker({
              .type = serviceworker::Registration::Options::Type::Module,
              .scriptURL = scriptURL,
              .scope = scope,
              .scheme = scheme,
              .serializedWorkerArgs = "",
              .id = id
            });
          }
        } else {
          this->navigator.serviceWorkerServer->container.registerServiceWorker({
            .type = serviceworker::Registration::Options::Type::Module,
            .scriptURL = scriptURL,
            .scope = scope,
            .scheme = scheme,
            .serializedWorkerArgs = encodeURIComponent(JSON::Object(JSON::Object::Entries {
              {"index", this->client.index},
              {"argv", JSON::Array {}},
              {"env", env},
              {"debug", isDebugEnabled()},
              {"headless", this->userConfig["build_headless"] == "true"},
              {"config", this->userConfig},
              {"conduit", JSON::Object::Entries {
              {"port", this->getRuntime()->services.conduit.port},
              {"hostname", this->getRuntime()->services.conduit.hostname},
              {"sharedKey", this->getRuntime()->services.conduit.sharedKey}
              }}
            }).str()),
            .id = id
          });
        }
      }

      this->schemeHandlers.registerSchemeHandler(scheme, [this](
        auto request,
        const auto& bridge,
        auto callbacks,
        auto callback
      ) {
        auto app = App::sharedApplication();
        auto window = app->runtime.windowManager.getWindowForBridge(&bridge);

        auto fetch = serviceworker::Request();
        SharedPointer<serviceworker::Server> serviceWorkerServer = nullptr;

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

        fetch.method = request->method;
        fetch.scheme = request->scheme;
        fetch.url.scheme = request->scheme;
        fetch.url.hostname = request->hostname;
        fetch.url.pathname = request->pathname;
        fetch.url.search = "?" + request->query;
        fetch.headers = request->headers;
        fetch.body = request->body;
        fetch.client = request->client;

        if (request->scheme == "npm") {
          auto app = App::sharedApplication();
          static auto userConfig = getUserConfig();
          const auto bundleIdentifier = userConfig["meta_bundle_identifier"];

          auto pathname = request->pathname;
          auto hostname = request->hostname;

          if (hostname.size() > 0) {
            fetch.url.pathname = "/" + fetch.url.hostname;
          }

          fetch.url.hostname = bundleIdentifier;
        }

        if (!fetch.headers.has("origin")) {
          fetch.headers.set("origin", this->navigator.location.origin);
        }

        const auto options = serviceworker::Fetch::Options {
          .client = request->client,
          .waitForRegistrationToFinish = request->scheme != "npm"
        };

        auto origin = webview::Origin(fetch.url.str());
        origin.scheme = "socket";
        serviceWorkerServer = app->runtime.serviceWorkerManager.get(origin.name());
        if (!serviceWorkerServer) {
          serviceWorkerServer = this->navigator.serviceWorkerServer;
        }

        const auto scope = serviceWorkerServer->container.protocols.getServiceWorkerScope(request->scheme);

        if (scope.size() > 0) {
          fetch.url.pathname = scope + fetch.url.pathname;
        }

        const auto fetched = serviceWorkerServer->fetch(fetch, options, [request, callback] (auto res) mutable {
          if (!request->isActive()) {
            return;
          }

          auto response = SchemeHandlers::Response(request);
          if (res.statusCode == 0) {
            response.fail("ServiceWorker request failed");
          } else {
            response.writeHead(res.statusCode, res.headers);
            response.write(res.body.buffer);
          }

          callback(response);
        });

        if (fetched) {
          // FIXME(@jwerle): revisit timeout
          //this->core->setTimeout(32000, [request] () mutable {
          //if (request->isActive()) {
          //auto response = SchemeHandlers::Response(request, 408);
          //response.fail("Protocol handler ServiceWorker request timed out.");
          //}
          //});
          return;
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

  Runtime* Bridge::getRuntime () {
    return this->context.getRuntime();
  }

  const Runtime* Bridge::getRuntime () const {
    return this->context.getRuntime();
  }

  bool Bridge::active () const {
    return this->getRuntime()->loop.alive();
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
    using namespace ssc::runtime;
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window requested");
      return false;
    }

    const auto event = android::StringWrap(env, eventString).str();
    const auto data = android::StringWrap(env, dataString).str();
    return window->bridge.emit(event, data);
  }
}
#endif
