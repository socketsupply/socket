#ifndef SOCKET_RUNTIME_CORE_MODULE_PERMISSIONS_H
#define SOCKET_RUNTIME_CORE_MODULE_PERMISSIONS_H

#include "../module.hh"

namespace SSC {
  class CorePermissions : public CoreModule {
    public:
      CorePermissions (Core* core)
        : CoreModule(core)
      {}

      bool hasRuntimePermission (const String& permission) const;

      void query (
        const String& seq,
        const String& name,
        const Callback& callback
      ) const;

      void request (
        const String& seq,
        const String& name,
        const Map& options,
        const Callback& callback
      ) const;
  };
}
#endif
