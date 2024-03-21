#include "service_worker_container.hh"
#include "debug.hh"

#include "../ipc/ipc.hh"

// TODO(@jwerle): create a better platform macro to drop this garbage below
#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__)) || (defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR)
#include "../app/app.hh"
#endif

namespace SSC {
  static IPC::Bridge* serviceWorkerBridge = nullptr;
  ServiceWorkerContainer::Registration::Registration (
    const ID id,
    const String& scriptURL,
    const State state,
    const RegistrationOptions& options
  ) : id(id),
      scriptURL(scriptURL),
      state(state),
      options(options)
  {}

  ServiceWorkerContainer::Registration::Registration (const Registration& registration) {
    this->id = registration.id;
    this->state = registration.state.load();
    this->options = registration.options;
    this->scriptURL = registration.scriptURL;
  }

  ServiceWorkerContainer::Registration::Registration (Registration&& registration) {
    this->id = registration.id;
    this->state = registration.state.load();
    this->options = registration.options;
    this->scriptURL = registration.scriptURL;

    registration.id = 0;
    registration.state = State::None;
    registration.options = RegistrationOptions {};
    registration.scriptURL = "";
  }

  ServiceWorkerContainer::Registration& ServiceWorkerContainer::Registration::operator= (const Registration& registration) {
    this->id = registration.id;
    this->state = registration.state.load();
    this->options = registration.options;
    this->scriptURL = registration.scriptURL;
    return *this;
  }

  ServiceWorkerContainer::Registration& ServiceWorkerContainer::Registration::operator= (Registration&& registration) {
    this->id = registration.id;
    this->state = registration.state.load();
    this->options = registration.options;
    this->scriptURL = registration.scriptURL;

    registration.id = 0;
    registration.state = State::None;
    registration.options = RegistrationOptions {};
    registration.scriptURL = "";
    return *this;
  }

  const JSON::Object ServiceWorkerContainer::Registration::json () const {
    String stateString = "registered";

    if (this->state == Registration::State::Installing) {
      stateString = "installing";
    } else if (this->state == Registration::State::Installed) {
      stateString = "installed";
    } else if (this->state == Registration::State::Activating) {
      stateString = "activating";
    } else if (this->state == Registration::State::Activated) {
      stateString = "activated";
    }

    return JSON::Object::Entries {
      {"id", std::to_string(this->id)},
      {"scriptURL", this->scriptURL},
      {"scope", this->options.scope},
      {"state", stateString}
    };
  }

  bool ServiceWorkerContainer::Registration::isActive () const {
    return (
      this->state == Registration::State::Activating ||
      this->state == Registration::State::Activated
    );
  }

  bool ServiceWorkerContainer::Registration::isWaiting () const {
    return this->state == Registration::State::Installed;
  }

  bool ServiceWorkerContainer::Registration::isInstalling () const {
    return this->state == Registration::State::Installing;
  }

  const String ServiceWorkerContainer::Registration::getStateString () const {
    String stateString = "none";

    if (this->state == Registration::State::Registered) {
      stateString = "registered";
    } else if (this->state == Registration::State::Installing) {
      stateString = "installing";
    } else if (this->state == Registration::State::Installed) {
      stateString = "installed";
    } else if (this->state == Registration::State::Activating) {
      stateString = "activating";
    } else if (this->state == Registration::State::Activated) {
      stateString = "activated";
    }

    return stateString;
  };

  ServiceWorkerContainer::ServiceWorkerContainer (Core* core) {
    this->core = core;
  }

  ServiceWorkerContainer::~ServiceWorkerContainer () {
    if (this->bridge != nullptr) {
      this->bridge->router.emit("serviceWorker.destroy", "{}");
    }

    this->core = nullptr;
    this->bridge = nullptr;
    this->registrations.clear();
    this->fetchRequests.clear();
    this->fetchCallbacks.clear();
  }

  void ServiceWorkerContainer::reset () {
    Lock lock(this->mutex);

    for (auto& entry : this->registrations) {
      entry.second.state = Registration::State::Registered;
      this->registerServiceWorker(entry.second.options);
    }
  }

  void ServiceWorkerContainer::init (IPC::Bridge* bridge) {
    Lock lock(this->mutex);

    this->reset();
    this->bridge = bridge;
    this->isReady = true;

    if (bridge->userConfig["webview_service-workers"].size() > 0) {
      const auto scripts = split(bridge->userConfig["webview_service-workers"], " ");
      for (const auto& value : scripts) {
        auto parts = split(value, "/");
        parts = Vector<String>(parts.begin(), parts.end() - 1);

      #if defined(__ANDROID__)
        auto scriptURL = String("https://");
      #else
        auto scriptURL = String("socket://");
      #endif
        scriptURL += bridge->userConfig["meta_bundle_identifier"];

        if (!value.starts_with("/")) {
          scriptURL += "/";
        }

        scriptURL += value;

        const auto scope = join(parts, "/");
        const auto id = rand64();
        this->registrations.insert_or_assign(scope, Registration {
          id,
          scriptURL,
          Registration::State::Registered,
          RegistrationOptions {
          RegistrationOptions::Type::Module,
            scope,
            scriptURL,
            id
          }
        });
      }
    }

    for (const auto& entry : bridge->userConfig) {
      const auto& key = entry.first;
      const auto& value = entry.second;

      if (key.starts_with("webview_service-workers_")) {
        const auto id = rand64();
        const auto scope = replace(key, "webview_service-workers_", "");

      #if defined(__ANDROID__)
        auto scriptURL = String("https://");
      #else
        auto scriptURL = String("socket://");
      #endif
        scriptURL += bridge->userConfig["meta_bundle_identifier"];

        if (!value.starts_with("/")) {
          scriptURL += "/";
        }

        scriptURL += trim(value);

        this->registrations.insert_or_assign(scope, Registration {
          id,
          scriptURL,
          Registration::State::Registered,
          RegistrationOptions {
            RegistrationOptions::Type::Module,
            scope,
            scriptURL,
            id
          }
        });
      }
    }

    this->bridge->router.map("serviceWorker.fetch.request.body", [this](auto message, auto router, auto reply) mutable {
      Lock lock(this->mutex);
      uint64_t id = 0;

      try {
        id = std::stoull(message.get("id"));
      } catch (...) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'id' given in parameters"}
        }});
      }

      if (!this->fetchRequests.contains(id)) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"type", "NotFoundError"},
          {"message", "Callback 'id' given in parameters does not have a 'FetchRequest'"}
        }});
      }

      const auto& request = this->fetchRequests.at(id);
      const auto post = Post { 0, 0, request.buffer.bytes, request.buffer.size };
      reply(IPC::Result { message.seq, message, JSON::Object {}, post });
    });

    this->bridge->router.map("serviceWorker.fetch.response", [this](auto message, auto router, auto reply) mutable {
      Lock lock(this->mutex);

      uint64_t clientId = 0;
      uint64_t id = 0;
      int statusCode = 200;

      try {
        id = std::stoull(message.get("id"));
      } catch (...) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'id' given in parameters"}
        }});
      }

      try {
        clientId = std::stoull(message.get("clientId"));
      } catch (...) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'clientId' given in parameters"}
        }});
      }

      if (!this->fetchCallbacks.contains(id)) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"type", "NotFoundError"},
          {"message", "Callback 'id' given in parameters does not have a 'FetchCallback'"}
        }});
      }

      if (!this->fetchRequests.contains(id)) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"type", "NotFoundError"},
          {"message", "Callback 'id' given in parameters does not have a 'FetchRequest'"}
        }});
      }

      const auto callback = this->fetchCallbacks.at(id);

      try {
        statusCode = std::stoi(message.get("statusCode"));
      } catch (...) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'statusCode' given in parameters"}
        }});
      }

      const auto headers = split(message.get("headers"), '\n');
      const auto request = this->fetchRequests.at(id);
      auto response = FetchResponse {
        id,
        statusCode,
        headers,
        { message.buffer.size, message.buffer.bytes },
        { clientId }
      };

      String contentType = "";

      for (const auto& entry : headers) {
        auto pair = split(trim(entry), ':');
        auto key = trim(pair[0]);
        auto value = trim(pair[1]);

        if (key == "content-type") {
          contentType = value;
        }
      }

      bool freeResponseBody = false;

      // XXX(@jwerle): we handle this in the android runtime
    #if !SSC_PLATFORM_ANDROID
      const auto extname = Path(request.pathname).extension().string();
      if (
        statusCode < 400 &&
        (message.buffer.bytes != nullptr && message.buffer.size > 0) &&
        (extname.ends_with("html") || contentType == "text/html")
      ) {
        const auto preload = (
          String("<meta name=\"runtime-frame-source\" content=\"serviceworker\" />\n") +
          request.client.preload
        );

        auto html = String(response.buffer.bytes, response.buffer.size);
        auto begin = String("<meta name=\"begin-runtime-preload\">");
        auto end = String("<meta name=\"end-runtime-preload\">");
        auto x = html.find(begin);
        auto y = html.find(end);

        if (x != String::npos && y != String::npos) {
          html.erase(x, (y - x) + end.size());
        }

        Vector<String> protocolHandlers = { "npm:", "node:" };
        for (const auto& entry : router->core->protocolHandlers.mapping) {
          protocolHandlers.push_back(String(entry.first) + ":");
        }

        html = tmpl(html, Map {
          {"protocol_handlers", join(protocolHandlers, " ")}
        });

        if (html.find("<head>") != String::npos) {
          html = replace(html, "<head>", String("<head>" + preload));
        } else if (html.find("<body>") != String::npos) {
          html = replace(html, "<body>", String("<body>" + preload));
        } else if (html.find("<html>") != String::npos) {
          html = replace(html, "<html>", String("<html>" + preload));
        } else {
          html = preload + html;
        }

        response.buffer.bytes = new char[html.size()]{0};
        response.buffer.size = html.size();
        freeResponseBody = true;

        memcpy(response.buffer.bytes, html.c_str(), html.size());
      }
    #endif

      // no `app` pointer or on mobile, just call callback
      callback(response);

      reply(IPC::Result { message.seq, message });

      if (freeResponseBody) {
        delete response.buffer.bytes;
      }

      this->fetchCallbacks.erase(id);
      if (this->fetchRequests.contains(id)) {
        this->fetchRequests.erase(id);
      }
    });
  }

  const ServiceWorkerContainer::Registration& ServiceWorkerContainer::registerServiceWorker (
    const RegistrationOptions& options
  ) {
    Lock lock(this->mutex);

    if (this->registrations.contains(options.scope)) {
      const auto& registration = this->registrations.at(options.scope);

      if (this->bridge != nullptr) {
        this->bridge->router.emit("serviceWorker.register", registration.json().str());
      }

      return registration;
    }

    this->registrations.insert_or_assign(options.scope, Registration {
      options.id > 0 ? options.id : rand64(),
      options.scriptURL,
      Registration::State::Registered,
      options
    });

    const auto& registration = this->registrations.at(options.scope);

    if (this->bridge != nullptr) {
      this->core->setImmediate([&, this]() {
        this->bridge->router.emit("serviceWorker.register", registration.json().str());
      });
    }

    return registration;
  }

  bool ServiceWorkerContainer::unregisterServiceWorker (String scopeOrScriptURL) {
    Lock lock(this->mutex);

    const auto& scope = scopeOrScriptURL;
    const auto& scriptURL = scopeOrScriptURL;

    if (this->registrations.contains(scope)) {
      const auto& registration = this->registrations.at(scope);
      if (this->bridge != nullptr) {
        return this->bridge->router.emit("serviceWorker.unregister", registration.json().str());
      }

      this->registrations.erase(scope);
      return true;
    }

    for (const auto& entry : this->registrations) {
      if (entry.second.scriptURL == scriptURL) {
        const auto& registration = this->registrations.at(entry.first);

        if (this->bridge != nullptr) {
          return this->bridge->router.emit("serviceWorker.unregister", registration.json().str());
        }

        this->registrations.erase(entry.first);
      }
    }

    return false;
  }

  bool ServiceWorkerContainer::unregisterServiceWorker (uint64_t id) {
    Lock lock(this->mutex);

    for (const auto& entry : this->registrations) {
      if (entry.second.id == id) {
        return this->unregisterServiceWorker(entry.first);
      }
    }

    return false;
  }

  void ServiceWorkerContainer::skipWaiting (uint64_t id) {
    Lock lock(this->mutex);

    for (auto& entry : this->registrations) {
      if (entry.second.id == id) {
        auto& registration = entry.second;
        if (
          registration.state == Registration::State::Installing ||
          registration.state == Registration::State::Installed
        ) {
          registration.state = Registration::State::Activating;

          if (this->bridge != nullptr) {
            this->bridge->router.emit("serviceWorker.skipWaiting", registration.json().str());
          }
        }
        break;
      }
    }
  }

  void ServiceWorkerContainer::updateState (uint64_t id, const String& stateString) {
    Lock lock(this->mutex);

    for (auto& entry : this->registrations) {
      if (entry.second.id == id) {
        auto& registration = entry.second;
        if (stateString == "error") {
          registration.state = Registration::State::Error;
        } else if (stateString == "registered") {
          registration.state = Registration::State::Registered;
        } else if (stateString == "installing") {
          registration.state = Registration::State::Installing;
        } else if (stateString == "installed") {
          registration.state = Registration::State::Installed;
        } else if (stateString == "activating") {
          registration.state = Registration::State::Activating;
        } else if (stateString == "activated") {
          registration.state = Registration::State::Activated;
        } else {
          break;
        }

        if (this->bridge != nullptr) {
          this->bridge->router.emit("serviceWorker.updateState", registration.json().str());
        }

        break;
      }
    }
  }

  bool ServiceWorkerContainer::fetch (const FetchRequest& request, FetchCallback callback) {
    Lock lock(this->mutex);

    if (this->bridge == nullptr || !this->isReady) {
      return false;
    }

    for (const auto& entry : request.headers) {
      const auto parts = split(trim(entry), ':');
      if (parts.size() == 2) {
        const auto key = trim(parts[0]);
        const auto value = trim(parts[1]);
        if (key == "runtime-worker-type" && value == "serviceworker") {
          return false;
        }
      }
    }

    String scope;
    for (const auto& entry : this->registrations) {
      const auto& registration = entry.second;

      if (request.pathname.starts_with(registration.options.scope)) {
        if (entry.first.size() > scope.size()) {
          scope = entry.first;
        }
      }
    }

    if (scope.size() > 0 && this->registrations.contains(scope)) {
      auto& registration = this->registrations.at(scope);
      if (!registration.isActive() && registration.state == Registration::State::Registered) {
        this->core->dispatchEventLoop([this, request, callback, &registration]() {
          const auto interval = this->core->setInterval(8, [this, request, callback, &registration] (auto cancel) {
            if (registration.state == Registration::State::Activated) {
              cancel();
              if (!this->fetch(request, callback)) {
                debug(
                  "ServiceWorkerContainer: Failed to dispatch fetch request '%s %s%s' for client '%llu'",
                  request.method.c_str(),
                  request.pathname.c_str(),
                  (request.query.size() > 0 ? String("?") + request.query : String("")).c_str(),
                  request.client.id
                );
              }
            }
          });

          this->core->setTimeout(32000, [this, interval] {
            this->core->clearInterval(interval);
          });
        });

        return true;
      }

      auto headers = JSON::Array {};

      for (const auto& header : request.headers) {
        headers.push(header);
      }

      const auto id = rand64();
      const auto client = JSON::Object::Entries {
        {"id", std::to_string(request.client.id)}
      };

      const auto fetch = JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"method", request.method},
        {"pathname", request.pathname},
        {"query", request.query},
        {"headers", headers},
        {"client", client}
      };

      auto json = registration.json();
      json.set("fetch", fetch);

      this->fetchCallbacks.insert_or_assign(id, callback);
      this->fetchRequests.insert_or_assign(id, request);

      return this->bridge->router.emit("serviceWorker.fetch", json.str());
    }

    return false;
  }
}
