#ifndef SOCKET_RUNTIME_CORE_MODULE_DNS_H
#define SOCKET_RUNTIME_CORE_MODULE_DNS_H

#include "../module.hh"

namespace SSC {
  class Core;
  class CoreDNS : public CoreModule {
    public:
      struct LookupOptions {
        String hostname;
        int family;
        // TODO: support these options
        // - hints
        // - all
        // -verbatim
      };

      CoreDNS (Core* core)
        : CoreModule(core)
      {}

      void lookup (
        const String& seq,
        const LookupOptions& options,
        const CoreModule::Callback& callback
      ) const;
  };
}
#endif
