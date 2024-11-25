#include "../app/app.hh"
#include "../ipc/ipc.hh"
#include "../core/debug.hh"

#include "container.hh"

namespace SSC {
  static ServiceWorkerContainer* sharedServiceWorkerContainer = nullptr;

  static const String normalizeScope (const String& scope) {
    auto normalized = trim(scope);

    if (normalized.size() == 0) {
      return "/";
    }

    if (normalized.ends_with("/") && normalized.size() > 1) {
      normalized = normalized.substr(0, normalized.size() - 1);
    }

    return normalized;
  }

  const String ServiceWorkerContainer::FetchRequest::str () const {
    auto string = this->scheme + "://" + this->hostname + this->pathname;

    if (this->query.size() > 0) {
      string += String("?") + this->query;
    }

    return string;
  }

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
    return JSON::Object::Entries {
      {"id", std::to_string(this->id)},
      {"scriptURL", this->scriptURL},
      {"scope", this->options.scope},
      {"state", this->getStateString()}
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

    if (this->state == Registration::State::Error) {
      stateString = "error";
    } else if (this->state == Registration::State::Registering) {
      stateString = "registering";
    } else if (this->state == Registration::State::Registered) {
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

  const JSON::Object ServiceWorkerContainer::Registration::Storage::json () const {
    return JSON::Object { this->data };
  }

  void ServiceWorkerContainer::Registration::Storage::set (const String& key, const String& value) {
    this->data.insert_or_assign(key, value);
  }

  const String ServiceWorkerContainer::Registration::Storage::get (const String& key) const {
    if (this->data.contains(key)) {
      return this->data.at(key);
    }

    return key;
  }

  void ServiceWorkerContainer::Registration::Storage::remove (const String& key) {
    if (this->data.contains(key)) {
      this->data.erase(key);
    }
  }

  void ServiceWorkerContainer::Registration::Storage::clear () {
    this->data.clear();
  }

  ServiceWorkerContainer* ServiceWorkerContainer::sharedContainer () {
    return SSC::sharedServiceWorkerContainer;
  }

  ServiceWorkerContainer::ServiceWorkerContainer (SharedPointer<Core> core)
    : core(core),
      protocols(this)
  {
    if (SSC::sharedServiceWorkerContainer == nullptr) {
      SSC::sharedServiceWorkerContainer = this;
    }
  }

  ServiceWorkerContainer::~ServiceWorkerContainer () {
    if (this->bridge != nullptr) {
      this->bridge->emit("serviceWorker.destroy", JSON::Object {});
    }

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

      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        auto scriptURL = String("https://");
      #else
        auto scriptURL = String("socket://");
      #endif
        scriptURL += bridge->userConfig["meta_bundle_identifier"];

        if (!value.starts_with("/")) {
          scriptURL += "/";
        }

        scriptURL += value;

        const auto scope = normalizeScope(join(parts, "/"));
        const auto id = rand64();
        this->registrations.insert_or_assign(scope, Registration {
          id,
          scriptURL,
          Registration::State::Registered,
          RegistrationOptions {
          RegistrationOptions::Type::Module,
            scope,
            scriptURL,
            "*",
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
        const auto scope = normalizeScope(replace(key, "webview_service-workers_", ""));

      #if SOCKET_RUNTIME_PLATFORM_ANDROID
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
            "*",
            id
          }
        });
      }
    }

    for (const auto& entry : this->bridge->userConfig) {
      const auto& key = entry.first;
      if (key.starts_with("webview_protocol-handlers_")) {
        const auto scheme = replace(replace(trim(key), "webview_protocol-handlers_", ""), ":", "");;
        auto value = entry.second;
        if (value.starts_with(".") || value.starts_with("/")) {
          if (value.starts_with(".")) {
            value = value.substr(1, value.size());
          }

          const auto id = rand64();
          auto parts = split(value, "/");
          parts = Vector<String>(parts.begin(), parts.end() - 1);
          auto scope = normalizeScope(join(parts, "/"));

        #if SOCKET_RUNTIME_PLATFORM_ANDROID
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
              scheme,
              id
            }
          });
        }
      }
    }

    this->bridge->router.map("serviceWorker.fetch.request.body", [this](auto message, auto router, auto reply) mutable {
      FetchRequest request;
      ID id = 0;

      try {
        id = std::stoull(message.get("id"));
      } catch (...) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'id' given in parameters"}
        }});
      }

      do {
        Lock lock(this->mutex);

        if (!this->fetchRequests.contains(id)) {
          return reply(IPC::Result::Err { message, JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Callback 'id' given in parameters does not have a 'FetchRequest'"}
          }});
        }

        request = this->fetchRequests.at(id);
      } while (0);

      const auto post = Post {
        0,
        0,
        request.body.bytes,
        request.body.size
      };

      reply(IPC::Result { message.seq, message, JSON::Object {}, post });
    });

    this->bridge->router.map("serviceWorker.fetch.response", [this](auto message, auto router, auto reply) mutable {
      FetchCallback callback;
      FetchRequest request;
      ID clientId = 0;
      ID id = 0;

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

      do {
        Lock lock(this->mutex);
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

        callback = this->fetchCallbacks.at(id);
        request = this->fetchRequests.at(id);
      } while (0);

      try {
        statusCode = std::stoi(message.get("statusCode"));
      } catch (...) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'statusCode' given in parameters"}
        }});
      }

      const auto headers = Headers(message.get("headers"));
      auto response = FetchResponse {
        id,
        statusCode,
        headers,
        { message.buffer.size, message.buffer.bytes },
        { clientId }
      };

    // XXX(@jwerle): we handle this in the android runtime
      const auto extname = Path(request.pathname).extension().string();
      auto html = (message.buffer.bytes != nullptr && message.buffer.size > 0)
        ? String(response.body.bytes.get(), response.body.size)
        : String("");

      if (
        html.size() > 0 &&
        message.get("runtime-preload-injection") != "disabled" &&
        (
          message.get("runtime-preload-injection") == "always" ||
          (extname.ends_with("html") || headers.get("content-type").value.string == "text/html") ||
          (html.find("<!doctype html") != String::npos || html.find("<!DOCTYPE HTML") != String::npos) ||
          (html.find("<html") != String::npos || html.find("<HTML") != String::npos) ||
          (html.find("<body") != String::npos || html.find("<BODY") != String::npos) ||
          (html.find("<head") != String::npos || html.find("<HEAD") != String::npos) ||
          (html.find("<script") != String::npos || html.find("<SCRIPT") != String::npos)
        )
      ) {
        auto preloadOptions = request.client.preload.options;
        preloadOptions.metadata["runtime-frame-source"] = "serviceworker";

        auto preload = IPC::Preload::compile(preloadOptions);
        auto begin = String("<meta name=\"begin-runtime-preload\">");
        auto end = String("<meta name=\"end-runtime-preload\">");
        auto x = html.find(begin);
        auto y = html.find(end);

        if (x != String::npos && y != String::npos) {
          html.erase(x, (y - x) + end.size());
        }

        html = preload.insertIntoHTML(html, {
          .protocolHandlerSchemes = this->protocols.getSchemes()
        });

        response.body.bytes = std::make_shared<char[]>(html.size());
        response.body.size = html.size();

        memcpy(response.body.bytes.get(), html.c_str(), html.size());
      }

      callback(response);
      reply(IPC::Result { message.seq, message });

      Lock lock(this->mutex);
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
    auto scope = options.scope;
    auto scriptURL = options.scriptURL;
    auto userConfig = this->bridge != nullptr
      ? this->bridge->userConfig
      : getUserConfig();

    if (scope.size() == 0) {
      auto tmp = trim(options.scriptURL);
      tmp = replace(tmp, "https://", "");
      tmp = replace(tmp, "socket://", "");
      tmp = replace(tmp, userConfig["meta_bundle_identifier"], "");

      auto parts = split(tmp, "/");
      parts = Vector<String>(parts.begin(), parts.end() - 1);

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      scriptURL = String("https://");
    #else
      scriptURL = String("socket://");
    #endif

      scriptURL += userConfig["meta_bundle_identifier"];
      scriptURL += tmp;

      scope = join(parts, "/");
    }

    scope = normalizeScope(scope);

    if (this->registrations.contains(scope)) {
      const auto& registration = this->registrations.at(scope);

      if (this->bridge != nullptr) {
        this->bridge->emit("serviceWorker.register", registration.json().str());
      }

      return registration;
    }

    const auto id = options.id > 0 ? options.id : rand64();
    this->registrations.insert_or_assign(scope, Registration {
      id,
      options.scriptURL,
      Registration::State::Registered,
      RegistrationOptions {
        options.type,
        scope,
        options.scriptURL,
        options.scheme,
        id
      }
    });

    const auto& registration = this->registrations.at(scope);

    if (this->bridge != nullptr) {
      App::sharedApplication()->core->setImmediate([&, this]() {
        this->bridge->emit("serviceWorker.register", registration.json().str());
      });
    }

    return registration;
  }

  bool ServiceWorkerContainer::unregisterServiceWorker (String scopeOrScriptURL) {
    Lock lock(this->mutex);

    const auto& scope = normalizeScope(scopeOrScriptURL);
    const auto& scriptURL = scopeOrScriptURL;

    if (this->registrations.contains(scope)) {
      const auto& registration = this->registrations.at(scope);
      if (this->bridge != nullptr) {
        return this->bridge->emit("serviceWorker.unregister", registration.json().str());
      }

      this->registrations.erase(scope);
      return true;
    }

    for (const auto& entry : this->registrations) {
      if (entry.second.scriptURL == scriptURL) {
        const auto& registration = this->registrations.at(entry.first);

        if (this->bridge != nullptr) {
          return this->bridge->emit("serviceWorker.unregister", registration.json().str());
        }

        this->registrations.erase(entry.first);
      }
    }

    return false;
  }

  bool ServiceWorkerContainer::unregisterServiceWorker (ID id) {
    Lock lock(this->mutex);

    for (const auto& entry : this->registrations) {
      if (entry.second.id == id) {
        return this->unregisterServiceWorker(entry.first);
      }
    }

    return false;
  }

  void ServiceWorkerContainer::skipWaiting (ID id) {
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
            this->bridge->emit("serviceWorker.skipWaiting", registration.json().str());
          }
        }
        break;
      }
    }
  }

  void ServiceWorkerContainer::updateState (ID id, const String& stateString) {
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
          this->bridge->emit("serviceWorker.updateState", registration.json().str());
        }

        break;
      }
    }
  }

  bool ServiceWorkerContainer::fetch (const FetchRequest& request, FetchCallback callback) {
    Lock lock(this->mutex);
    String pathname = request.pathname;
    String scope;

    if (this->bridge == nullptr || !this->isReady) {
      return false;
    }

    if (request.headers.get("runtime-serviceworker-fetch-mode") == "ignore") {
      return false;
    }

    // TODO(@jwerle): this prevents nested service worker fetches - do we want to prevent this?
    if (request.headers.get("runtime-worker-type") == "serviceworker") {
      return false;
    }

    for (const auto& entry : this->registrations) {
      const auto& registration = entry.second;

      if (
        (registration.options.scheme == "*" || registration.options.scheme == request.scheme) &&
        request.pathname.starts_with(registration.options.scope)
      ) {
        if (entry.first.size() > scope.size()) {
          scope = entry.first;
        }
      }
    }

    if (scope.size() == 0) {
      return false;
    }

    scope = normalizeScope(scope);

    if (scope.size() > 0 && this->registrations.contains(scope)) {
      auto& registration = this->registrations.at(scope);
      if (
        !registration.isActive() &&
        (
          registration.options.scheme == "*" ||
          registration.options.scheme == request.scheme
        ) &&
        (
          registration.state == Registration::State::Registering ||
          registration.state == Registration::State::Registered
        )
      ) {
        this->core->dispatchEventLoop([this, request, callback, &registration]() {
          const auto interval = this->core->setInterval(8, [this, request, callback, &registration] (auto cancel) {
            if (registration.state == Registration::State::Activated) {
              cancel();
              this->core->setTimeout(8, [this, request, callback, &registration] {
                if (!this->fetch(request, callback)) {
                  debug(
                  #if SOCKET_RUNTIME_PLATFORM_APPLE
                    "ServiceWorkerContainer: Failed to dispatch fetch request '%s %s%s' for client '%llu'",
                  #else
                    "ServiceWorkerContainer: Failed to dispatch fetch request '%s %s%s' for client '%lu'",
                  #endif
                    request.method.c_str(),
                    request.pathname.c_str(),
                    (request.query.size() > 0 ? String("?") + request.query : String("")).c_str(),
                    request.client.id
                  );
                }
              });
            }
          });

          this->core->setTimeout(32000, [this, interval] {
            this->core->clearInterval(interval);
          });
        });

        return true;
      }

      // the ID of the fetch request
      const auto id = rand64();
      const auto client = JSON::Object::Entries {
        {"id", std::to_string(request.client.id)}
      };

      if (this->protocols.hasHandler(request.scheme)) {
        pathname = replace(pathname, scope, "");
      }

      const auto fetch = JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"method", request.method},
        {"host", request.hostname},
        {"scheme", request.scheme},
        {"pathname", pathname},
        {"query", request.query},
        {"headers", request.headers.json()},
        {"client", client}
      };

      auto json = registration.json();
      json.set("fetch", fetch);

      this->fetchCallbacks.insert_or_assign(id, callback);
      this->fetchRequests.insert_or_assign(id, request);

      return this->bridge->emit("serviceWorker.fetch", json.str());
    }

    return false;
  }
}
