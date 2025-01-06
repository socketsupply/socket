#include "../serviceworker.hh"
#include "../runtime.hh"

namespace ssc::runtime::serviceworker {
  static const Vector<String> reserved = {
    "socket",
    "ipc",
    "node",
    "npm"
  };

  Protocols::Protocols (Container& container)
    : container(container)
  {}

  Protocols::~Protocols () {}

  bool Protocols::registerHandler (const String& scheme, const Data data) {
    Lock lock(this->mutex);

    if (scheme.size() == 0) {
      return false;
    }

    if (std::find(reserved.begin(), reserved.end(), scheme) != reserved.end()) {
      return false;
    }

    if (this->mapping.contains(scheme)) {
      return true;
    }

    this->mapping.insert_or_assign(scheme, Protocol { scheme, data });
    return true;
  }

  bool Protocols::unregisterHandler (const String& scheme) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return false;
    }

    this->mapping.erase(scheme);
    return true;
  }

  const Protocols::Data Protocols::getHandlerData (const String& scheme) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return Data {};
    }

    return this->mapping.at(scheme).data;
  }

  bool Protocols::setHandlerData (const String& scheme, const Data data) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return false;
    }

    this->mapping.at(scheme).data = data;
    return true;
  }

  bool Protocols::hasHandler (const String& scheme) {
    Lock lock(this->mutex);
    return this->mapping.contains(scheme);
  }

  const String Protocols::getServiceWorkerScope (const String& scheme) {
    for (const auto& entry : this->container.registrations) {
      const auto& registration = entry.second;
      if (registration.options.scheme == scheme) {
        return registration.options.scope;
      }
    }

    return "";
  }

  const Vector<String> Protocols::getSchemes () const {
    Vector<String> schemes;
    for (const auto& entry : this->mapping) {
      schemes.push_back(entry.first + ":");
    }
    return schemes;
  }

  const Vector<String> Protocols::getProtocols () const {
    Vector<String> protocols;
    for (const auto& entry : this->mapping) {
      protocols.push_back(entry.first);
    }
    return protocols;
  }
}
