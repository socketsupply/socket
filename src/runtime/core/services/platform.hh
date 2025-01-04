#ifndef SOCKET_RUNTIME_CORE_SERVICES_PLATFORM_H
#define SOCKET_RUNTIME_CORE_SERVICES_PLATFORM_H

#include "../../ipc.hh"
#include "../../core.hh"

namespace ssc::runtime::core::services {
  class Platform : public core::Service {
    public:
      Atomic<bool> wasFirstDOMContentLoadedEventDispatched = false;

      Platform (const Options& options)
        : core::Service(options)
      {}

      void event (
        const ipc::Message::Seq&,
        const String&,
        const String&,
        const String&,
        const String&,
        const Callback
      );
      void openExternal (const ipc::Message::Seq&, const String&, const Callback);
      void revealFile ( const ipc::Message::Seq&, const String&, const Callback);
  };
}
#endif
