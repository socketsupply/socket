#include "media_devices.hh"

namespace ssc::runtime::core::services {
  MediaDevices::MediaDevices (const Options& options)
    : core::Service(options),
      permissionChangeObservers()
  {}

  MediaDevices::~MediaDevices () {}

  bool MediaDevices::addPermissionChangeObserver (
    const PermissionChangeObserver& observer,
    const PermissionChangeObserver::Callback callback
  ) {
    return this->permissionChangeObservers.add(observer, callback);
  }

  bool MediaDevices::removePermissionChangeObserver (const PermissionChangeObserver& observer) {
    return this->permissionChangeObservers.remove(observer);
  }
}
