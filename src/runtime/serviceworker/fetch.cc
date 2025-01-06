#include "../serviceworker.hh"
#include "../runtime.hh"

using ssc::runtime::string::replace;

namespace ssc::runtime::serviceworker {
  Fetch::Fetch (Container& container, const Request& request, const Options& options)
    : container(container),
      response(404),
      request(request),
      options(options)
  {
    this->request.client = options.client;
    this->response.client = options.client;
  }

  bool Fetch::init (const Callback callback) {
    if (this->request.headers.get("runtime-serviceworker-fetch-mode") == "ignore") {
      return false;
    }

    String scope;
    String pathname = this->request.url.pathname;
    Lock lock(this->container.mutex);

    if (this->container.bridge == nullptr || !this->container.ready()) {
      return false;
    }

    this->callback = callback;

    for (const auto& entry : this->container.registrations) {
      const auto& registration = entry.second;
      const auto origin = Origin(this->request.url.str());

      if (
        (
          (registration.options.scheme == "*") ||
          registration.options.scheme == this->request.url.scheme
        ) &&
        this->request.url.pathname.starts_with(registration.options.scope)
      ) {
        if (entry.first.size() > scope.size()) {
          scope = entry.first;
          break;
        }
      }
    }

    if (scope.size() > 0) {
      scope = normalizeScope(scope);
    } else {
      return false;
    }

    const auto key = Registration::key(scope, this->container.origin, this->request.scheme);
    if (this->container.registrations.contains(key)) {
      auto& registration = this->container.registrations.at(key);
      if (
        this->options.waitForRegistrationToFinish &&
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
        auto runtime = this->container.bridge->getRuntime();
        runtime->dispatch([this, runtime, callback, &registration]() {
          const auto interval = runtime->services.timers.setInterval(8, [this, runtime, callback, &registration] (auto cancel) {
            if (registration.state == Registration::State::Activated) {
              if (!this->init(callback)) {
                debug(
                #if SOCKET_RUNTIME_PLATFORM_APPLE
                  "ServiceWorkerContainer: Failed to dispatch fetch request '%s %s%s' for client '%llu'",
                #else
                  "ServiceWorkerContainer: Failed to dispatch fetch request '%s %s%s' for client '%lu'",
                #endif
                  request.method.c_str(),
                  request.url.pathname.c_str(),
                  request.url.search.c_str(),
                  request.client.id
                );
              }

              cancel();
            }
          });

          runtime->services.timers.setTimeout(32000, [this, runtime, interval] {
            runtime->services.timers.clearInterval(interval);
          });
        });

        return true;
      }

      // the ID of the fetch request
      const auto client = JSON::Object::Entries {
        {"id", std::to_string(request.client.id)}
      };

      if (this->container.protocols.hasHandler(request.scheme)) {
        const auto url = URL(scope, this->container.origin);
        pathname = replace(pathname, url.pathname, "");
      }

      const auto fetch = JSON::Object::Entries {
        {"id", std::to_string(this->id)},
        {"method", request.method},
        {"host", request.url.hostname},
        {"scheme", request.url.scheme},
        {"pathname", pathname},
        {"query", request.url.query},
        {"headers", request.headers.json()},
        {"client", client}
      };

      auto json = registration.json();
      json.set("fetch", fetch);

      return this->container.bridge->emit("serviceWorker.fetch", json);
    }

    debug("failed to dispatch fetch: %s %s", pathname.c_str(), this->request.str().c_str());
    return false;
  }

  bool Fetch::write (const bytes::Buffer& buffer) {
    return this->writeQueue.push(buffer);
  }

  bool Fetch::finish () {
    this->response.body = bytes::Buffer::from(this->writeQueue);
    return true;
  }
}
