#include "container.hh"
#include "protocols.hh"
#include "../core/debug.hh"

namespace SSC {
  static Vector<String> reserved = {
    "socket",
    "ipc",
    "node",
    "npm"
  };

  ServiceWorkerProtocols::ServiceWorkerProtocols (
    ServiceWorkerContainer* serviceWorkerContainer
  )
    : serviceWorkerContainer(serviceWorkerContainer)
  {}

  ServiceWorkerProtocols::~ServiceWorkerProtocols () {}

  bool ServiceWorkerProtocols::registerHandler (const String& scheme, const Data data) {
    Lock lock(this->mutex);

    if (scheme.size() == 0) {
      return false;
    }

    if (std::find(reserved.begin(), reserved.end(), scheme) != reserved.end()) {
      return false;
    }

    if (this->mapping.contains(scheme)) {
      return false;
    }

    this->mapping.insert_or_assign(scheme, Protocol { scheme, data });
    return true;
  }

  bool ServiceWorkerProtocols::unregisterHandler (const String& scheme) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return false;
    }

    this->mapping.erase(scheme);
    return true;
  }

  const ServiceWorkerProtocols::Data ServiceWorkerProtocols::getHandlerData (const String& scheme) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return Data {};
    }

    return this->mapping.at(scheme).data;
  }

  bool ServiceWorkerProtocols::setHandlerData (const String& scheme, const Data data) {
    Lock lock(this->mutex);

    if (!this->mapping.contains(scheme)) {
      return false;
    }

    this->mapping.at(scheme).data = data;
    return true;
  }

  bool ServiceWorkerProtocols::hasHandler (const String& scheme) {
    Lock lock(this->mutex);
    return this->mapping.contains(scheme);
  }

  const String ServiceWorkerProtocols::getServiceWorkerScope (const String& scheme) {
    for (const auto& entry : this->serviceWorkerContainer->registrations) {
      const auto& scope = entry.first;
      const auto& registration = entry.second;
      if (registration.options.scheme == scheme) {
        return scope;
      }
    }

    return "";
  }

  const Vector<String> ServiceWorkerProtocols::getSchemes () const {
    Vector<String> schemes;
    for (const auto& entry : this->mapping) {
      schemes.push_back(entry.first + ":");
    }
    return schemes;
  }

  const Vector<String> ServiceWorkerProtocols::getProtocols () const {
    Vector<String> protocols;
    for (const auto& entry : this->mapping) {
      protocols.push_back(entry.first);
    }
    return protocols;
  }
}
