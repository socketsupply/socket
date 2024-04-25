#include <regex>
#include <unordered_map>

#if defined(__APPLE__)
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

#if defined(__linux__) && !defined(__ANDROID__)
#include <fstream>
#include "../app/app.hh"
#endif

#include "../extension/extension.hh"
#include "../window/window.hh"
#include "../core/protocol_handlers.hh"
#include "ipc.hh"

#define SOCKET_MODULE_CONTENT_TYPE "text/javascript"
#define IPC_BINARY_CONTENT_TYPE "application/octet-stream"
#define IPC_JSON_CONTENT_TYPE "text/json"

extern const SSC::Map SSC::getUserConfig ();
extern bool SSC::isDebugEnabled ();

using namespace SSC;
using namespace SSC::IPC;

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
  "fs/promises",
  "http",
  "https",
  "net",
  "os",
  "path",
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
  "util",
  "url",
  "vm",
  "worker_threads"
};

static void registerSchemeHandler (Router *router) {
  static const auto MAX_BODY_BYTES = 4 * 1024 * 1024;
  static const auto devHost = SSC::getDevHost();
  static Atomic<bool> isInitialized = false;

  if (isInitialized) {
    return;
  }

  isInitialized = true;

#if defined(__linux__) && !defined(__ANDROID__)
  auto ctx = webkit_web_context_get_default();
  auto security = webkit_web_context_get_security_manager(ctx);

  webkit_web_context_register_uri_scheme(ctx, "ipc", [](auto request, auto ptr) {
    IPC::Router* router = nullptr;

    auto webview = webkit_uri_scheme_request_get_web_view(request);
    auto windowManager = App::instance()->getWindowManager();

    for (auto& window : windowManager->windows) {
      if (
        window != nullptr &&
        window->bridge != nullptr &&
        WEBKIT_WEB_VIEW(window->webview) == webview
      ) {
        router = &window->bridge->router;
        break;
      }
    }

    auto uri = String(webkit_uri_scheme_request_get_uri(request));
    auto method = String(webkit_uri_scheme_request_get_http_method(request));
    auto message = IPC::Message{ uri };
    char bytes[MAX_BODY_BYTES]{0};

    if ((method == "POST" || method == "PUT")) {
      auto body = webkit_uri_scheme_request_get_http_body(request);
      if (body) {
        GError* error = nullptr;
        message.buffer.bytes = new char[MAX_BODY_BYTES]{0};

        const auto success = g_input_stream_read_all(
          body,
          message.buffer.bytes,
          MAX_BODY_BYTES,
          &message.buffer.size,
          nullptr,
          &error
        );

        if (!success) {
          delete message.buffer.bytes;
          webkit_uri_scheme_request_finish_error(
            request,
            error
          );
          return;
        }
      }
    }

    auto invoked = router->invoke(message, message.buffer.bytes, message.buffer.size, [=](auto result) {
      auto json = result.str();
      auto size = result.post.body != nullptr ? result.post.length : json.size();
      auto body = result.post.body != nullptr ? result.post.body : json.c_str();

      char* data = nullptr;

      if (size > 0) {
        data = new char[size]{0};
        memcpy(data, body, size);
      }

      auto stream = g_memory_input_stream_new_from_data(data, size, nullptr);
      auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
      auto response = webkit_uri_scheme_response_new(stream, size);

      soup_message_headers_append(headers, "cache-control", "no-cache");
      for (const auto& header : result.headers.entries) {
        soup_message_headers_append(headers, header.key.c_str(), header.value.c_str());
      }

      if (result.post.body) {
        webkit_uri_scheme_response_set_content_type(response, IPC_BINARY_CONTENT_TYPE);
      } else {
        webkit_uri_scheme_response_set_content_type(response, IPC_JSON_CONTENT_TYPE);
      }

      webkit_uri_scheme_request_finish_with_response(request, response);
      g_input_stream_close_async(stream, 0, nullptr, +[](
        GObject* object,
        GAsyncResult* asyncResult,
        gpointer userData
      ) {
        auto stream = (GInputStream*) object;
        g_input_stream_close_finish(stream, asyncResult, nullptr);
        g_object_unref(stream);
        g_idle_add_full(
          G_PRIORITY_DEFAULT_IDLE,
          (GSourceFunc) [](gpointer userData) {
            return G_SOURCE_REMOVE;
          },
          userData,
           [](gpointer userData) {
            delete [] static_cast<char *>(userData);
          }
        );
      }, data);
    });

    if (!invoked) {
      auto err = JSON::Object::Entries {
        {"source", uri},
        {"err", JSON::Object::Entries {
          {"message", "Not found"},
          {"type", "NotFoundError"},
          {"url", uri}
        }}
      };

      auto msg = JSON::Object(err).str();
      auto size = msg.size();
      auto bytes = msg.c_str();
      auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);
      auto response = webkit_uri_scheme_response_new(stream, msg.size());

      webkit_uri_scheme_response_set_status(response, 404, "Not found");
      webkit_uri_scheme_response_set_content_type(response, IPC_JSON_CONTENT_TYPE);
      webkit_uri_scheme_request_finish_with_response(request, response);
      g_object_unref(stream);
    }
  },
  router,
  0);

  webkit_web_context_register_uri_scheme(ctx, "socket", [](auto request, auto ptr) {
    IPC::Router* router = nullptr;

    auto webview = webkit_uri_scheme_request_get_web_view(request);
    auto windowManager = App::instance()->getWindowManager();

    for (auto& window : windowManager->windows) {
      if (
        window != nullptr &&
        window->bridge != nullptr &&
        WEBKIT_WEB_VIEW(window->webview) == webview
      ) {
        router = &window->bridge->router;
        break;
      }
    }

    auto userConfig = router->bridge->userConfig;
    bool isModule = false;
    auto method = String(webkit_uri_scheme_request_get_http_method(request));
    auto uri = String(webkit_uri_scheme_request_get_uri(request));
    auto cwd = getcwd();
    uint64_t clientId = router->bridge->id;

    if (uri.starts_with("socket:///")) {
      uri = uri.substr(10);
    } else if (uri.starts_with("socket://")) {
      uri = uri.substr(9);
    } else if (uri.starts_with("socket:")) {
      uri = uri.substr(7);
    }

    const auto bundleIdentifier = userConfig["meta_bundle_identifier"];
    auto path = String(
      uri.starts_with(bundleIdentifier)
        ? uri.substr(bundleIdentifier.size())
        : "socket/" + uri
    );

    auto parsedPath = Router::parseURLComponents(path);
    auto ext = fs::path(parsedPath.path).extension().string();

    if (ext.size() > 0 && !ext.starts_with(".")) {
      ext = "." + ext;
    }

    if (!uri.starts_with(bundleIdentifier)) {
      path = parsedPath.path;
      if (ext.size() == 0 && !path.ends_with(".js")) {
        path += ".js";
        ext = ".js";
      }

      if (parsedPath.queryString.size() > 0) {
        path += String("?") + parsedPath.queryString;
      }

      if (parsedPath.fragment.size() > 0) {
        path += String("#") + parsedPath.fragment;
      }

      uri = "socket://" + bundleIdentifier + "/" + path;
      auto moduleSource = trim(tmpl(
        moduleTemplate,
        Map { {"url", String(uri)} }
      ));

      auto size = moduleSource.size();
      auto bytes = moduleSource.data();
      auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);

      if (stream) {
        auto response = webkit_uri_scheme_response_new(stream, size);
        webkit_uri_scheme_response_set_content_type(response, SOCKET_MODULE_CONTENT_TYPE);
        webkit_uri_scheme_request_finish_with_response(request, response);
        g_object_unref(stream);
      } else {
        webkit_uri_scheme_request_finish_error(
          request,
          g_error_new(
            g_quark_from_string(userConfig["meta_bundle_identifier"].c_str()),
            1,
            "Failed to create response stream"
          )
        );
      }
      return;
    }

    auto resolved = Router::resolveURLPathForWebView(parsedPath.path, cwd);
    auto mount = Router::resolveNavigatorMountForWebView(parsedPath.path);
    path = resolved.path;

    if (mount.path.size() > 0) {
      if (mount.resolution.redirect) {
        auto redirectURL = resolved.path;
        if (parsedPath.queryString.size() > 0) {
          redirectURL += "?" + parsedPath.queryString;
        }

        if (parsedPath.fragment.size() > 0) {
          redirectURL += "#" + parsedPath.fragment;
        }

        auto redirectSource = String(
          "<meta http-equiv=\"refresh\" content=\"0; url='" + redirectURL + "'\" />"
        );

        auto size = redirectSource.size();
        auto bytes = redirectSource.data();
        auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);

        if (stream) {
          auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
          auto response = webkit_uri_scheme_response_new(stream, (gint64) size);
          auto contentLocation = replace(redirectURL, "socket://" + bundleIdentifier, "");

          soup_message_headers_append(headers, "location", redirectURL.c_str());
          soup_message_headers_append(headers, "content-location", contentLocation.c_str());

          webkit_uri_scheme_response_set_http_headers(response, headers);
          webkit_uri_scheme_response_set_content_type(response, "text/html");
          webkit_uri_scheme_request_finish_with_response(request, response);

          g_object_unref(stream);
        } else {
          webkit_uri_scheme_request_finish_error(
            request,
            g_error_new(
              g_quark_from_string(userConfig["meta_bundle_identifier"].c_str()),
              1,
              "Failed to create response stream"
            )
          );
        }

        return;
      }
    } else if (path.size() == 0) {
      if (userConfig.contains("webview_default_index")) {
        path = userConfig["webview_default_index"];
      } else {
        if (router->core->serviceWorker.registrations.size() > 0) {
          auto requestHeaders = webkit_uri_scheme_request_get_http_headers(request);
          auto fetchRequest = ServiceWorkerContainer::FetchRequest {};

          fetchRequest.client.id = clientId;
          fetchRequest.client.preload = router->bridge->preload;

          fetchRequest.method = method;
          fetchRequest.scheme = "socket";
          fetchRequest.host = userConfig["meta_bundle_identifier"];
          fetchRequest.pathname = parsedPath.path;
          fetchRequest.query = parsedPath.queryString;

          soup_message_headers_foreach(
            requestHeaders,
            [](auto name, auto value, auto userData) {
              auto fetchRequest = reinterpret_cast<ServiceWorkerContainer::FetchRequest*>(userData);
              const auto entry = String(name) + ": " + String(value);
              fetchRequest->headers.push_back(entry);
            },
            &fetchRequest
          );

          if ((method == "POST" || method == "PUT")) {
            auto body = webkit_uri_scheme_request_get_http_body(request);
            if (body) {
              GError* error = nullptr;
              fetchRequest.buffer.bytes = new char[MAX_BODY_BYTES]{0};

              const auto success = g_input_stream_read_all(
                body,
                fetchRequest.buffer.bytes,
                MAX_BODY_BYTES,
                &fetchRequest.buffer.size,
                nullptr,
                &error
              );

              if (!success) {
                delete fetchRequest.buffer.bytes;
                webkit_uri_scheme_request_finish_error(
                  request,
                  error
                );
                return;
              }
            }
          }

          const auto fetched = router->core->serviceWorker.fetch(fetchRequest, [=] (auto res) mutable {
            if (res.statusCode == 0) {
              webkit_uri_scheme_request_finish_error(
                request,
                g_error_new(
                  g_quark_from_string(userConfig["meta_bundle_identifier"].c_str()),
                  1,
                  "%.*s",
                  (int) res.buffer.size,
                  res.buffer.bytes
                )
              );
              return;
            }

            const auto webviewHeaders = split(userConfig["webview_headers"], '\n');
            auto stream = g_memory_input_stream_new_from_data(res.buffer.bytes, res.buffer.size, 0);

            if (!stream) {
              webkit_uri_scheme_request_finish_error(
                request,
                g_error_new(
                  g_quark_from_string(userConfig["meta_bundle_identifier"].c_str()),
                  1,
                  "Failed to create response stream"
                )
              );
              return;
            }

            auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
            auto response = webkit_uri_scheme_response_new(stream, (gint64) res.buffer.size);

            for (const auto& line : webviewHeaders) {
              auto pair = split(trim(line), ':');
              auto key = trim(pair[0]);
              auto value = trim(pair[1]);
              soup_message_headers_append(headers, key.c_str(), value.c_str());
            }

            for (const auto& line : res.headers) {
              auto pair = split(trim(line), ':');
              auto key = trim(pair[0]);
              auto value = trim(pair[1]);

              if (key == "content-type" || key == "Content-Type") {
                webkit_uri_scheme_response_set_content_type(response, value.c_str());
              }

              soup_message_headers_append(headers, key.c_str(), value.c_str());
            }

            webkit_uri_scheme_response_set_http_headers(response, headers);
            webkit_uri_scheme_request_finish_with_response(request, response);

            g_object_unref(stream);
          });

          if (fetched) {
            return;
          } else {
            if (fetchRequest.buffer.bytes != nullptr) {
              delete fetchRequest.buffer.bytes;
            }
          }
        }
      }
    } else if (resolved.redirect) {
      auto redirectURL = resolved.path;
      if (parsedPath.queryString.size() > 0) {
        redirectURL += "?" + parsedPath.queryString;
      }

      if (parsedPath.fragment.size() > 0) {
        redirectURL += "#" + parsedPath.fragment;
      }

      auto redirectSource = String(
        "<meta http-equiv=\"refresh\" content=\"0; url='" + redirectURL + "'\" />"
      );

      auto size = redirectSource.size();
      auto bytes = redirectSource.data();
      auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);

      if (!stream) {
        webkit_uri_scheme_request_finish_error(
          request,
          g_error_new(
            g_quark_from_string(userConfig["meta_bundle_identifier"].c_str()),
            1,
            "Failed to create response stream"
          )
        );
        return;
      }

      auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
      auto response = webkit_uri_scheme_response_new(stream, (gint64) size);
      auto contentLocation = replace(redirectURL, "socket://" + bundleIdentifier, "");

      soup_message_headers_append(headers, "location", redirectURL.c_str());
      soup_message_headers_append(headers, "content-location", contentLocation.c_str());

      webkit_uri_scheme_response_set_http_headers(response, headers);
      webkit_uri_scheme_response_set_content_type(response, "text/html");
      webkit_uri_scheme_request_finish_with_response(request, response);

      g_object_unref(stream);
      return;
    }

    if (mount.path.size() > 0) {
      path = mount.path;
    } else if (path.size() > 0) {
      path = fs::absolute(fs::path(cwd) / path.substr(1)).string();
    }

    if (path.size() == 0 || !fs::exists(path)) {
      auto stream = g_memory_input_stream_new_from_data(nullptr, 0, 0);

      if (!stream) {
        webkit_uri_scheme_request_finish_error(
          request,
          g_error_new(
            g_quark_from_string(userConfig["meta_bundle_identifier"].c_str()),
            1,
            "Failed to create response stream"
          )
        );
        return;
      }

      auto response = webkit_uri_scheme_response_new(stream, 0);

      webkit_uri_scheme_response_set_status(response, 404, "Not found");
      webkit_uri_scheme_request_finish_with_response(request, response);
      g_object_unref(stream);
      return;
    }

    WebKitURISchemeResponse* response = nullptr;
    GInputStream* stream = nullptr;
    gchar* mimeType = nullptr;
    GError* error = nullptr;
    char* data = nullptr;

    auto webviewHeaders = split(userConfig["webview_headers"], '\n');
    auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);

    if (path.ends_with(".html")) {
      auto script = router->bridge->preload;

      if (userConfig["webview_importmap"].size() > 0) {
        const auto file = Path(userConfig["webview_importmap"]);

        if (fs::exists(file)) {
          String string;
          std::ifstream stream(file.string().c_str());
          auto buffer = std::istreambuf_iterator<char>(stream);
          auto end = std::istreambuf_iterator<char>();
          string.assign(buffer, end);
          stream.close();

          script = (
            String("<script type=\"importmap\">\n") +
            string +
            String("\n</script>\n") +
            script
          );
        }
      }

      const auto file = Path(path);

      if (fs::exists(file)) {
        char* contents = nullptr;
        gsize size = 0;
        if (g_file_get_contents(file.c_str(), &contents, &size, &error)) {
          String html = contents;
          Vector<String> protocolHandlers = { "npm:", "node:" };
          for (const auto& entry : router->core->protocolHandlers.mapping) {
            protocolHandlers.push_back(String(entry.first) + ":");
          }

          html = tmpl(html, Map {
            {"protocol_handlers", join(protocolHandlers, " ")}
          });

          if (html.find("<meta name=\"runtime-preload-injection\" content=\"disabled\"") != String::npos) {
            script = "";
          } else if (html.find("<meta content=\"disabled\" name=\"runtime-preload-injection\"") != String::npos) {
            script = "";
          }

          if (html.find("<head>") != String::npos) {
            html = replace(html, "<head>", String("<head>" + script));
          } else if (html.find("<body>") != String::npos) {
            html = replace(html, "<body>", String("<body>" + script));
          } else if (html.find("<html>") != String::npos) {
            html = replace(html, "<html>", String("<html>" + script));
          } else {
            html = script + html;
          }

          data = new char[html.size()]{0};
          memcpy(data, html.data(), html.size());
          g_free(contents);

          stream = g_memory_input_stream_new_from_data(data, (gint64) html.size(), nullptr);

          if (stream) {
            response = webkit_uri_scheme_response_new(stream, -1);
          } else {
            delete [] data;
            data = nullptr;
          }
        }
      }
    } else {
      auto file = g_file_new_for_path(path.c_str());
      auto size = fs::file_size(path);

      if (file) {
        stream = (GInputStream*) g_file_read(file, nullptr, &error);
        g_object_unref(file);
      }

      if (stream) {
        response = webkit_uri_scheme_response_new(stream, (gint64) size);
        g_object_unref(stream);
      }
    }

    if (!stream) {
      webkit_uri_scheme_request_finish_error(request, error);
      g_error_free(error);
      return;
    }

    soup_message_headers_append(headers, "cache-control", "no-cache");
    soup_message_headers_append(headers, "access-control-allow-origin", "*");
    soup_message_headers_append(headers, "access-control-allow-methods", "*");
    soup_message_headers_append(headers, "access-control-allow-headers", "*");
    soup_message_headers_append(headers, "access-control-allow-credentials", "true");

    for (const auto& line : webviewHeaders) {
      auto pair = split(trim(line), ':');
      auto key = trim(pair[0]);
      auto value = trim(pair[1]);
      soup_message_headers_append(headers, key.c_str(), value.c_str());
    }

    webkit_uri_scheme_response_set_http_headers(response, headers);

    if (path.ends_with(".wasm")) {
      webkit_uri_scheme_response_set_content_type(response, "application/wasm");
    } else if (path.ends_with(".cjs") || path.ends_with(".mjs")) {
      webkit_uri_scheme_response_set_content_type(response, "text/javascript");
    } else if (path.ends_with(".ts")) {
      webkit_uri_scheme_response_set_content_type(response, "application/typescript");
    } else {
      mimeType = g_content_type_guess(path.c_str(), nullptr, 0, nullptr);
      if (mimeType) {
        webkit_uri_scheme_response_set_content_type(response, mimeType);
      } else {
        webkit_uri_scheme_response_set_content_type(response, SOCKET_MODULE_CONTENT_TYPE);
      }
    }

    webkit_uri_scheme_request_finish_with_response(request, response);

    if (data) {
      g_input_stream_close_async(stream, 0, nullptr, +[](
        GObject* object,
        GAsyncResult* asyncResult,
        gpointer userData
      ) {
        auto stream = (GInputStream*) object;
        g_input_stream_close_finish(stream, asyncResult, nullptr);
        g_object_unref(stream);
        g_idle_add_full(
          G_PRIORITY_DEFAULT_IDLE,
          (GSourceFunc) [](gpointer userData) {
            return G_SOURCE_REMOVE;
          },
          userData,
           [](gpointer userData) {
            delete [] static_cast<char *>(userData);
          }
        );
      }, data);
    } else {
      g_object_unref(stream);
    }

    if (mimeType) {
      g_free(mimeType);
    }
  },
  router,
  0);

  webkit_web_context_register_uri_scheme(ctx, "node", [](auto request, auto ptr) {
    auto uri = String(webkit_uri_scheme_request_get_uri(request));
    auto router = reinterpret_cast<IPC::Router*>(ptr);
    auto userConfig = router->bridge->userConfig;

    const auto bundleIdentifier = userConfig["meta_bundle_identifier"];

    if (uri.starts_with("node:///")) {
      uri = uri.substr(10);
    } else if (uri.starts_with("node://")) {
      uri = uri.substr(9);
    } else if (uri.starts_with("node:")) {
      uri = uri.substr(7);
    }

    auto path = String("socket/" + uri);
    auto ext = fs::path(path).extension().string();

    if (ext.size() > 0 && !ext.starts_with(".")) {
      ext = "." + ext;
    }

    if (ext.size() == 0 && !path.ends_with(".js")) {
      path += ".js";
      ext = ".js";
    }

    uri = "socket://" + bundleIdentifier + "/" + path;

    auto moduleSource = trim(tmpl(
      moduleTemplate,
      Map { {"url", String(uri)} }
    ));

    auto size = moduleSource.size();
    auto bytes = moduleSource.data();
    auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);
    auto response = webkit_uri_scheme_response_new(stream, size);

    webkit_uri_scheme_response_set_content_type(response, SOCKET_MODULE_CONTENT_TYPE);
    webkit_uri_scheme_request_finish_with_response(request, response);
    g_object_unref(stream);
  },
  router,
  0);

  webkit_security_manager_register_uri_scheme_as_display_isolated(security, "ipc");
  webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "ipc");
  webkit_security_manager_register_uri_scheme_as_secure(security, "ipc");
  webkit_security_manager_register_uri_scheme_as_local(security, "ipc");

  if (devHost.starts_with("http:")) {
    webkit_security_manager_register_uri_scheme_as_display_isolated(security, "http");
    webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "http");
    webkit_security_manager_register_uri_scheme_as_secure(security, "http");
    webkit_security_manager_register_uri_scheme_as_local(security, "http");
  }

  webkit_security_manager_register_uri_scheme_as_display_isolated(security, "socket");
  webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "socket");
  webkit_security_manager_register_uri_scheme_as_secure(security, "socket");
  webkit_security_manager_register_uri_scheme_as_local(security, "socket");

  webkit_security_manager_register_uri_scheme_as_display_isolated(security, "node");
  webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "node");
  webkit_security_manager_register_uri_scheme_as_secure(security, "node");
  webkit_security_manager_register_uri_scheme_as_local(security, "node");
#endif
}

namespace SSC::IPC {
  Mutex Router::notificationMapMutex;
  std::map<String, Router*> Router::notificationMap;

  static Vector<Bridge*> instances;
  static Mutex mutex;

#if SSC_PLATFORM_DESKTOP
  static FileSystemWatcher* fileSystemWatcher = nullptr;
#endif

  Bridge::Bridge (Core *core, Map userConfig)
    : userConfig(userConfig),
      router(),
      navigator(this)
  {
    Lock lock(SSC::IPC::mutex);
    instances.push_back(this);

    this->id = rand64();
    this->core = core;
    this->router.core = core;

    this->bluetooth.sendFunction = [this](
      const String& seq,
      const JSON::Any value,
      const SSC::Post post
    ) {
      this->router.send(seq, value.str(), post);
    };

    this->bluetooth.emitFunction = [this](
      const String& seq,
      const JSON::Any value
    ) {
      this->router.emit(seq, value.str());
    };

    core->networkStatus.addObserver(this->networkStatusObserver, [this](auto json) {
      if (json.has("name")) {
        this->router.emit(json["name"].str(), json.str());
      }
    });

    core->geolocation.addPermissionChangeObserver(this->geolocationPermissionChangeObserver, [this] (auto json) {
      JSON::Object event = JSON::Object::Entries {
        {"name", "geolocation"},
        {"state", json["state"]}
      };
      this->router.emit("permissionchange", event.str());
    });

    core->notifications.addPermissionChangeObserver(this->notificationsPermissionChangeObserver, [this](auto json) {
      JSON::Object event = JSON::Object::Entries {
        {"name", "notifications"},
        {"state", json["state"]}
      };
      this->router.emit("permissionchange", event.str());
    });

    if (userConfig["permissions_allow_notifications"] != "false") {
      core->notifications.addNotificationResponseObserver(this->notificationResponseObserver, [this](auto json) {
        this->router.emit("notificationresponse", json.str());
      });

      core->notifications.addNotificationPresentedObserver(this->notificationPresentedObserver, [this](auto json) {
        this->router.emit("notificationpresented", json.str());
      });
    }

  #if SSC_PLATFORM_DESKTOP
    auto defaultUserConfig = SSC::getUserConfig();
    if (
      fileSystemWatcher == nullptr &&
      isDebugEnabled() &&
      defaultUserConfig["webview_watch"] == "true"
    ) {
      fileSystemWatcher = new FileSystemWatcher(getcwd());
      fileSystemWatcher->core = this->core;
      fileSystemWatcher->start([=](
        const auto& path,
        const auto& events,
        const auto& context
      ) mutable {
        Lock lock(SSC::IPC::mutex);

        static const auto cwd = getcwd();
        const auto relativePath = std::filesystem::relative(path, cwd).string();
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
            for (const auto& entry : fileSystemWatcher->core->serviceWorker.registrations) {
              const auto& registration = entry.second;
            #if defined(__ANDROID__)
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
                bridge->core->serviceWorker.unregisterServiceWorker(entry.first);
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
                        bridge->router.emit("filedidchange", result.json().str());
                      });
                    }
                  });

                  bridge->core->serviceWorker.registerServiceWorker(registration.options);
                });
                return;
              }
            }
          }

          bridge->router.emit("filedidchange", result.json().str());
        }
      });
    }
  #endif

    this->router.init(this);
  }

  Bridge::~Bridge () {
    Lock lock(SSC::IPC::mutex);

    // remove observers
    core->networkStatus.removeObserver(this->networkStatusObserver);
    core->geolocation.removePermissionChangeObserver(this->geolocationPermissionChangeObserver);

    const auto cursor = std::find(instances.begin(), instances.end(), this);
    if (cursor != instances.end()) {
      instances.erase(cursor);
    }

    #if SSC_PLATFORM_DESKTOP
      if (instances.size() == 0) {
        if (fileSystemWatcher) {
          fileSystemWatcher->stop();
          delete fileSystemWatcher;
        }
      }
    #endif
  }

  bool Bridge::route (const String& uri, const char *bytes, size_t size) {
    return this->route(uri, bytes, size, nullptr);
  }

  bool Bridge::route (
    const String& uri,
    const char* bytes,
    size_t size,
    Router::ResultCallback callback
  ) {
    if (callback != nullptr) {
      return this->router.invoke(uri, bytes, size, callback);
    } else {
      return this->router.invoke(uri, bytes, size);
    }
  }

  const Vector<String>& Bridge::getAllowedNodeCoreModules () const {
    return allowedNodeCoreModules;
  }
}
