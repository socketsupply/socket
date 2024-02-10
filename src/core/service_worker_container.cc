#include "service_worker_container.hh"

#include "../ipc/ipc.hh"

namespace SSC {
  static IPC::Bridge* serviceWorkerBridge = nullptr;

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
  }

  void ServiceWorkerContainer::init (IPC::Bridge* bridge) {
    this->bridge = bridge;
    this->bridge->router.map("serviceWorker.fetch.response", [this](auto message, auto router, auto reply) mutable {
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

      if (!this->fetches.contains(id)) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"type", "NotFoundError"},
          {"message", "Callback 'id' given in parameters does not have a 'FetchCallback'"}
        }});
      }

      const auto callback = this->fetches.at(id);

      try {
        statusCode = std::stoi(message.get("statusCode"));
      } catch (...) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid 'statusCode' given in parameters"}
        }});
      }

      const auto headers = split(message.get("headers"), '\n');
      const auto response = FetchResponse {
        id,
        statusCode,
        headers,
        { message.buffer.size, message.buffer.bytes },
        { clientId }
      };

      this->fetches.erase(id);
      callback(response);
    });
  }

  const ServiceWorkerContainer::Registration ServiceWorkerContainer::registerServiceWorker (
    const RegistrationOptions& options
  ) {
    if (this->registrations.contains(options.scope)) {
      auto& registration = this->registrations.at(options.scope);

      if (this->bridge != nullptr) {
        this->bridge->router.emit("serviceWorker.register", registration.json().str());
      }

      return registration;
    }

    const auto id = rand64();
    const auto registration = Registration {
      id,
      options.scriptURL,
      Registration::State::Registered,
      options
    };

    this->registrations.insert_or_assign(options.scope, registration);

    if (this->bridge != nullptr) {
      this->bridge->router.emit("serviceWorker.register", registration.json().str());
    }

    return registration;
  }

  bool ServiceWorkerContainer::unregisterServiceWorker (String scopeOrScriptURL) {
    const auto& scope = scopeOrScriptURL;
    const auto& scriptURL = scopeOrScriptURL;

    if (this->registrations.contains(scope)) {
      const auto registration = this->registrations.at(scope);
      this->registrations.erase(scope);
      if (this->bridge != nullptr) {
        return this->bridge->router.emit("serviceWorker.unregister", registration.json().str());
      }
    }

    for (const auto& entry : this->registrations) {
      if (entry.second.scriptURL == scriptURL) {
        const auto registration = this->registrations.at(entry.first);
        this->registrations.erase(entry.first);
        if (this->bridge != nullptr) {
          return this->bridge->router.emit("serviceWorker.unregister", registration.json().str());
        }
      }
    }

    return false;
  }

  bool ServiceWorkerContainer::unregisterServiceWorker (uint64_t id) {
    for (const auto& entry : this->registrations) {
      if (entry.second.id == id) {
        return this->unregisterServiceWorker(entry.first);
      }
    }

    return false;
  }

  void ServiceWorkerContainer::skipWaiting (uint64_t id) {
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

  bool ServiceWorkerContainer::fetch (FetchRequest request, FetchCallback callback) {
    if (this->bridge == nullptr) {
      return false;
    }

    for (const auto& entry : this->registrations) {
      const auto& registration = entry.second;
      if (request.pathname.starts_with(registration.options.scope)) {
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
          {"pathname", request.pathname},
          {"query", request.query},
          {"headers", headers},
          {"client", client}
        };

        auto json = registration.json();
        json.set("fetch", fetch);
        this->fetches.insert_or_assign(id, callback);
        return this->bridge->router.emit("serviceWorker.fetch", json.str());
      }
    }

    return false;
  }
}
