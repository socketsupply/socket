#include "media_devices.hh"
#include "../debug.hh"

namespace SSC {
  CoreMediaDevices::CoreMediaDevices (Core* core)
    : CoreModule(core),
      permissionChangeObservers()
  {}

  CoreMediaDevices::~CoreMediaDevices () {}

  template<> bool CoreModule::template Observers<CoreModule::template Observer<JSON::Object>>::add(
    const CoreModule::Observer<JSON::Object>&,
    CoreModule::Observer<JSON::Object>::Callback
  );

  template<> bool CoreMediaDevices::PermissionChangeObservers::remove(
    const CoreMediaDevices::PermissionChangeObserver&
  );

  bool CoreMediaDevices::addPermissionChangeObserver (
    const PermissionChangeObserver& observer,
    const PermissionChangeObserver::Callback callback
  ) {
    return this->permissionChangeObservers.add(observer, callback);
  }

  bool CoreMediaDevices::removePermissionChangeObserver (const PermissionChangeObserver& observer) {
    return this->permissionChangeObservers.remove(observer);
  }
}
