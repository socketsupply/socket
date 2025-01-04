#include "../runtime.hh"
#include "../config.hh"
#include "../crypto.hh"
#include "../string.hh"
#include "../env.hh"

#include "../serviceworker.hh"

using ssc::runtime::url::encodeURIComponent;
using ssc::runtime::crypto::rand64;
using ssc::runtime::config::isDebugEnabled;
using ssc::runtime::config::getUserConfig;
using ssc::runtime::string::parseStringList;
using ssc::runtime::string::replace;
using ssc::runtime::string::split;
using ssc::runtime::string::trim;
using ssc::runtime::string::join;

namespace ssc::runtime::serviceworker {
  Container::Container ()
    : protocols(*this)
  {}

  Container::~Container () {
    if (this->bridge != nullptr) {
      this->bridge->emit("serviceWorker.destroy", JSON::Object {});
    }
  }

  bool Container::ready () {
    return this->isReady.load(std::memory_order_relaxed);
  }

  void Container::reset () {
    Lock lock(this->mutex);

    for (auto& entry : this->registrations) {
      entry.second.state = Registration::State::Registered;
      this->registerServiceWorker(entry.second.options);
    }
  }

  void Container::init (SharedPointer<ipc::IBridge> bridge) {
    Lock lock(this->mutex);

    this->reset();
    this->bridge = bridge;
    this->isReady = true;

    auto env = JSON::Object::Entries {};
    for (const auto& entry : bridge->userConfig) {
      if (entry.first.starts_with("env_")) {
        env[entry.first.substr(4)] = entry.second;
      } else if (entry.first == "build_env") {
        const auto keys = parseStringList(entry.second, { ',', ' ' });
        for (const auto& key : keys) {
          env[key] = env::get(key);
        }
      }
    }

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
        this->registrations.insert_or_assign(scope, Registration(
          id,
          Registration::State::Registered,
          this->origin,
          Registration::Options {
            Registration::Options::Type::Module,
            scriptURL,
            scope,
            "*",
            encodeURIComponent(JSON::Object(JSON::Object::Entries {
              {"index", bridge->index},
              {"env", env},
              {"debug", isDebugEnabled()},
              {"config", bridge->userConfig},
              {"headless", bridge->userConfig["build_headless"] == "true"},
              {"conduit", JSON::Object::Entries {
                {"port", this->bridge->getRuntime()->services.conduit.port},
                {"hostname", this->bridge->getRuntime()->services.conduit.hostname},
                {"sharedKey", this->bridge->getRuntime()->services.conduit.sharedKey}
              }}
            }).str()),
            id
          }
        ));
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

        this->registrations.insert_or_assign(scope, Registration(
          id,
          Registration::State::Registered,
          this->origin,
          Registration::Options {
            Registration::Options::Type::Module,
            scriptURL,
            scope,
            "*",
            encodeURIComponent(JSON::Object(JSON::Object::Entries {
              {"index", bridge->index},
              {"env", env},
              {"debug", isDebugEnabled()},
              {"headless", bridge->userConfig["build_headless"] == "true"},
              {"config", bridge->userConfig},
              {"conduit", JSON::Object::Entries {
                {"port", this->bridge->getRuntime()->services.conduit.port},
                {"hostname", this->bridge->getRuntime()->services.conduit.hostname},
                {"sharedKey", this->bridge->getRuntime()->services.conduit.sharedKey}
              }}
            }).str()),
            id
          }
        ));
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

          this->registrations.insert_or_assign(scope, Registration(
            id,
            Registration::State::Registered,
            this->origin,
            Registration::Options {
              Registration::Options::Type::Module,
              scriptURL,
              scope,
              scheme,
              encodeURIComponent(JSON::Object(JSON::Object::Entries {
                {"index", bridge->index},
                {"env", env},
                {"debug", isDebugEnabled()},
                {"config", bridge->userConfig},
                {"headless", bridge->userConfig["build_headless"] == "true"},
                {"conduit", JSON::Object::Entries {
                  {"port", this->bridge->getRuntime()->services.conduit.port},
                  {"hostname", this->bridge->getRuntime()->services.conduit.hostname},
                  {"sharedKey", this->bridge->getRuntime()->services.conduit.sharedKey}
                }}
              }).str()),
              id
            }
          ));
        }
      }
    }

    this->bridge->router.map("serviceWorker.fetch.request.body", [this](auto message, auto router, auto reply) mutable {
      SharedPointer<Fetch> fetch = nullptr;
      ID id = 0;

      try {
        id = std::stoull(message.get("id"));
      } catch (...) {
        return reply(ipc::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'id' given in parameters"}
        }});
      }

      do {
        Lock lock(this->mutex);

        if (!this->fetches.contains(id)) {
          return reply(ipc::Result::Err { message, JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Callback 'id' given in parameters does not have a 'Request'"}
          }});
        }

        fetch = this->fetches.at(id);
      } while (0);

      const auto queuedResponse = QueuedResponse {
        0,
        0,
        fetch->request.body.buffer.as<unsigned char>(),
        fetch->request.body.size()
      };

      reply(ipc::Result { message.seq, message, JSON::Object {}, queuedResponse });
    });

    this->bridge->router.map("serviceWorker.fetch.response.write", [this](auto message, auto router, auto reply) mutable {
      SharedPointer<Fetch> fetch = nullptr;
      ID clientId = 0;
      ID id = 0;

      int statusCode = 200;

      try {
        id = std::stoull(message.get("id"));
      } catch (...) {
        return reply(ipc::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'id' given in parameters"}
        }});
      }

      try {
        clientId = std::stoull(message.get("clientId"));
      } catch (...) {
        return reply(ipc::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'clientId' given in parameters"}
        }});
      }

      do {
        Lock lock(this->mutex);
        if (!this->fetches.contains(id)) {
          return reply(ipc::Result::Err { message, JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Callback 'id' given in parameters does not have a 'Request'"}
          }});
        }

        fetch = this->fetches.at(id);
      } while (0);

      try {
        statusCode = std::stoi(message.get("statusCode"));
      } catch (...) {
        return reply(ipc::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'statusCode' given in parameters"}
        }});
      }

      do {
        Lock lock(fetch->mutex);
        if (fetch->response.headers.size() == 0 && message.has("headers")) {
          fetch->response.headers = http::Headers(message.get("headers"));
          fetch->response.client.id = clientId;
          fetch->response.status = statusCode;
        }

        if (message.buffer.size() > 0) {
          fetch->write(message.buffer);
        }
      } while (0);

      reply(ipc::Result { message.seq, message });
    });

    this->bridge->router.map("serviceWorker.fetch.response.finish", [this](auto message, auto router, auto reply) mutable {
      SharedPointer<Fetch> fetch = nullptr;
      ID clientId = 0;
      ID id = 0;

      int statusCode = 200;

      try {
        id = std::stoull(message.get("id"));
      } catch (...) {
        return reply(ipc::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'id' given in parameters"}
        }});
      }

      if (message.has("statusCode")) {
        try {
          statusCode = std::stoi(message.get("statusCode"));
        } catch (...) {
          return reply(ipc::Result::Err { message, JSON::Object::Entries {
            {"message", "Invalid 'statusCode' given in parameters"}
          }});
        }
      }

      try {
        clientId = std::stoull(message.get("clientId"));
      } catch (...) {
        return reply(ipc::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'clientId' given in parameters"}
        }});
      }

      do {
        Lock lock(this->mutex);
        if (!this->fetches.contains(id)) {
          return reply(ipc::Result::Err { message, JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Callback 'id' given in parameters does not have a 'Request'"}
          }});
        }

        fetch = this->fetches.at(id);
      } while (0);

      do {
        Lock lock(fetch->mutex);

        if (fetch->response.headers.size() == 0 && message.has("headers")) {
          fetch->response.headers = http::Headers(message.get("headers"));
          fetch->response.client.id = clientId;
          fetch->response.status = statusCode;
        }

        if (message.buffer.size() > 0) {
          fetch->write(message.buffer);
        }
      } while (0);

      fetch->finish();

      // XXX(@jwerle): we handle this in the android runtime
      const auto extname = Path(fetch->request.url.pathname).extension().string();
      auto html = (fetch->response.body.data() != nullptr && fetch->response.body.size() > 0)
        ? String(reinterpret_cast<char*>(fetch->response.body.data()), fetch->response.body.size())
        : String("");

      if (
        html.size() > 0 &&
        message.get("runtime-preload-injection") != "disabled" &&
        (
          message.get("runtime-preload-injection") == "always" ||
          (extname.ends_with("html") || fetch->response.headers.get("content-type").value.string == "text/html") ||
          (html.find("<!doctype html") != String::npos || html.find("<!DOCTYPE HTML") != String::npos) ||
          (html.find("<html") != String::npos || html.find("<HTML") != String::npos) ||
          (html.find("<body") != String::npos || html.find("<BODY") != String::npos) ||
          (html.find("<head") != String::npos || html.find("<HEAD") != String::npos) ||
          (html.find("<script") != String::npos || html.find("<SCRIPT") != String::npos)
        )
      ) {
        auto preloadOptions = fetch->request.client.preload.options;
        preloadOptions.metadata["runtime-frame-source"] = "serviceworker";

        auto preload = webview::Preload::compile(preloadOptions);
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

        fetch->response.body = bytes::Buffer::from(html);
      }

      fetch->callback(fetch->response);
      reply(ipc::Result { message.seq, message });

      Lock lock(this->mutex);
      this->fetches.erase(id);
    });
  }

  const Registration& Container::registerServiceWorker (
    const Registration::Options& options
  ) {
    Lock lock(this->mutex);
    auto scope = options.scope;
    auto scriptURL = options.scriptURL;
    auto userConfig = this->bridge != nullptr
      ? this->bridge->getRuntime()->userConfig
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
        this->bridge->emit("serviceWorker.register", registration.json(true).str());
      }

      return registration;
    }

    const auto id = options.id > 0 ? options.id : rand64();
    this->registrations.insert_or_assign(scope, Registration(
      id,
      Registration::State::Registered,
      this->origin,
      Registration::Options {
        options.type,
        options.scriptURL,
        scope,
        options.scheme,
        options.serializedWorkerArgs,
        id
      }
    ));

    const auto& registration = this->registrations.at(scope);

    if (this->bridge != nullptr) {
      this->bridge->emit("serviceWorker.register", registration.json(true));
    }

    return registration;
  }

  bool Container::unregisterServiceWorker (String scopeOrScriptURL) {
    Lock lock(this->mutex);

    const auto& scope = normalizeScope(scopeOrScriptURL);
    const auto& scriptURL = scopeOrScriptURL;

    if (this->registrations.contains(scope)) {
      const auto& registration = this->registrations.at(scope);
      if (this->bridge != nullptr) {
        return this->bridge->emit("serviceWorker.unregister", registration.json());
      }

      this->registrations.erase(scope);
      return true;
    }

    for (const auto& entry : this->registrations) {
      if (entry.second.options.scriptURL == scriptURL) {
        const auto& registration = this->registrations.at(entry.first);

        if (this->bridge != nullptr) {
          return this->bridge->emit("serviceWorker.unregister", registration.json().str());
        }

        this->registrations.erase(entry.first);
      }
    }

    return false;
  }

  bool Container::unregisterServiceWorker (ID id) {
    Lock lock(this->mutex);

    for (const auto& entry : this->registrations) {
      if (entry.second.id == id) {
        return this->unregisterServiceWorker(entry.first);
      }
    }

    return false;
  }

  void Container::skipWaiting (ID id) {
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

  void Container::updateState (ID id, const String& stateString) {
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

  bool Container::fetch (
    const Request& request,
    const Fetch::Options& options,
    const Fetch::Callback callback
  ) {
    Lock lock(this->mutex);
    auto fetch = std::make_shared<Fetch>(*this, request, options);
    this->fetches.insert_or_assign(fetch->id, fetch);
    return fetch->init(callback);
  }
}
