#ifndef SOCKET_RUNTIME_CORE_SERVICES_DNS_H
#define SOCKET_RUNTIME_CORE_SERVICES_DNS_H

#include "../../core.hh"

namespace ssc::runtime::core::services {
  class DNS : public core::Service {
    public:
      struct LookupOptions {
        String hostname;
        int family;
        // TODO: support these options: hints, all, verbatim
      };

      DNS (const Options& options)
        : core::Service(options)
      {}

      void lookup (const String&, const LookupOptions&, const Callback) const;
  };
}
#endif
