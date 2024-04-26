#include "protocol_handlers.hh"
#include "core.hh"

namespace SSC {
  static Vector<String> reserved = {
    "socket",
    "ipc",
    "node",
    "npm"
  };

  ProtocolHandlers::ProtocolHandlers (Core* core)
    : Module(core)
  {}

  ProtocolHandlers::~ProtocolHandlers () {}

  bool ProtocolHandlers::registerHandler (const String& scheme, const Data data) {
    Lock lock(this->mutex);

    if (std::find(reserved.begin(), reserved.end(), scheme) != reserved.end()) {
      return false;
    }

    if (this->mapping.contains(scheme)) {
      return false;
    }

    this->mapping.insert_or_assign(scheme, Protocol { scheme, data });
    return true;
  }

  bool ProtocolHandlers::unregisterHandler (const String& scheme) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return false;
    }

    this->mapping.erase(scheme);
    return true;
  }

  const ProtocolHandlers::Data ProtocolHandlers::getHandlerData (const String& scheme) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return Data {};
    }

    return this->mapping.at(scheme).data;
  }

  bool ProtocolHandlers::setHandlerData (const String& scheme, const Data data) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return false;
    }

    this->mapping.at(scheme).data = data;
    return true;
  }

  bool ProtocolHandlers::hasHandler (const String& scheme) {
    Lock lock(this->mutex);
    return this->mapping.contains(scheme);
  }

  const String ProtocolHandlers::getServiceWorkerScope (const String& scheme) {
    for (const auto& entry : this->core->serviceWorker.registrations) {
      const auto& scope = entry.first;
      const auto& registration = entry.second;
      if (registration.options.scheme == scheme) {
        return scope;
      }
    }

    return "";
  }
}
