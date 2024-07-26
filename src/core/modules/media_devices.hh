#ifndef SOCKET_RUNTIME_CORE_MODULE_MEDIA_DEVICES_H
#define SOCKET_RUNTIME_CORE_MODULE_MEDIA_DEVICES_H

#include "../module.hh"

namespace SSC {
  class CoreMediaDevices : public CoreModule {
    public:
      using PermissionChangeObserver = CoreModule::Observer<JSON::Object>;
      using PermissionChangeObservers = CoreModule::Observers<PermissionChangeObserver>;

      PermissionChangeObservers permissionChangeObservers;

      CoreMediaDevices (Core* core);
      ~CoreMediaDevices ();

      bool removePermissionChangeObserver (
        const PermissionChangeObserver& observer
      );

      bool addPermissionChangeObserver (
        const PermissionChangeObserver& observer,
        const PermissionChangeObserver::Callback callback
      );
  };
}
#endif
