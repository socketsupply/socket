#include "../bridge.hh"
#include "../serviceworker.hh"

namespace ssc::runtime::serviceworker {
  Manager::Manager (
    context::RuntimeContext& context,
    const Options& options
  ) : windowManager(options.windowManager),
      context(context)
  {}

  SharedPointer<Server> Manager::init (const String& origin, const Server::Options& options) {
    Lock lock(this->mutex);

    if (this->servers.contains(origin)) {
      return this->servers.at(origin);
    }

    const auto server = std::make_shared<Server>(*this, Server::Options {
      .origin = origin,
      .userConfig = options.userConfig,
      .window = options.window
    });

    this->servers.insert_or_assign(server->origin.name(), server);

    if (server->init()) {
      return this->servers.at(server->origin.name());
    }

    this->servers.erase(server->origin.name());

    return nullptr;
  }

  SharedPointer<Server> Manager::get (const String& origin) {
    Lock lock(this->mutex);
    const auto key = webview::Origin(origin).name();
    return this->servers.contains(key) ? this->servers.at(key) : nullptr;
  }

  SharedPointer<Server> Manager::get (const bridge::Bridge* bridge) {
    Lock lock(this->mutex);
    for (const auto& entry : this->servers) {
      if (dynamic_cast<bridge::Bridge*>(entry.second->bridge.get()) == bridge) {
        return entry.second;
      }
    }

    return nullptr;
  }

  bool Manager::destroy (const String& origin) {
    Lock lock(this->mutex);
    const auto key = webview::Origin(origin).name();
    if (this->servers.contains(key)) {
      this->servers.erase(key);
      return true;
    }

    return false;
  }

  bool Manager::fetch (
    const Request& request,
    const Fetch::Options& options,
    const Fetch::Callback callback
  ) {
    Lock lock(this->mutex);
    const auto key = webview::Origin(request.url.origin).name();
    if (this->servers.contains(key)) {
      return this->servers.at(key)->fetch(request, options, callback);
    }

    return false;
  }
}
