#ifndef SOCKET_RUNTIME_CORE_SERVICES_PERMISSIONS_H
#define SOCKET_RUNTIME_CORE_SERVICES_PERMISSIONS_H

#include "../../ipc.hh"
#include "../../core.hh"

namespace ssc::runtime::core::services {
  class Permissions : public core::Service {
    public:
      Permissions (const Options& options)
        : core::Service(options)
      {}

      void query (const ipc::Message::Seq&, const String&, const Callback) const;
      void request (const ipc::Message::Seq&, const String&, const Map<String, String>&, const Callback) const;
      bool hasRuntimePermission (const String& permission) const;
  };
}
#endif
