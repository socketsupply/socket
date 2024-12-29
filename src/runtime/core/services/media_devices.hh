#ifndef SOCKET_RUNTIME_CORE_SERVICES_MEDIA_DEVICES_H
#define SOCKET_RUNTIME_CORE_SERVICES_MEDIA_DEVICES_H

#include "../../core.hh"

namespace ssc::runtime::core::services {
  class MediaDevices : public core::Service {
    public:
      using PermissionChangeObserver = Observer<JSON::Object>;
      using PermissionChangeObservers = Observers<PermissionChangeObserver>;

      PermissionChangeObservers permissionChangeObservers;

      MediaDevices (const Options& options);
      ~MediaDevices ();

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
