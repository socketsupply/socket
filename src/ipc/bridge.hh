#ifndef SSC_IPC_BRIDGE_H
#define SSC_IPC_BRIDGE_H

#include "../core/core.hh"
#include "navigator.hh"
#include "router.hh"
#include "scheme_handlers.hh"

namespace SSC::IPC {
  class Bridge {
    public:
      static Vector<Bridge*> getInstances();

      const NetworkStatus::Observer networkStatusObserver;
      const Geolocation::PermissionChangeObserver geolocationPermissionChangeObserver;
      const Notifications::PermissionChangeObserver notificationsPermissionChangeObserver;
      const Notifications::NotificationResponseObserver notificationResponseObserver;
      const Notifications::NotificationPresentedObserver notificationPresentedObserver;

      Bluetooth bluetooth;
      Navigator navigator;
      SchemeHandlers schemeHandlers;
      Router router;
      Map userConfig = getUserConfig();

      Core *core = nullptr;
      String preload = "";
      uint64_t id = 0;

    #if SSC_PLATFORM_ANDROID
      bool isAndroidEmulator = false;
    #endif

      Bridge (Core *core, Map userConfig = getUserConfig());
      ~Bridge ();
      bool route (const String& msg, const char *bytes, size_t size);
      bool route (
        const String& msg,
        const char *bytes,
        size_t size,
        Router::ResultCallback
      );

      const Vector<String>& getAllowedNodeCoreModules () const;
      void configureHandlers (const SchemeHandlers::Configuration& configuration);
  };
}
#endif
