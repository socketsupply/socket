#ifndef SOCKET_RUNTIME_CORE_MODULE_OS_H
#define SOCKET_RUNTIME_CORE_MODULE_OS_H

#include "../module.hh"

namespace SSC {
  class Core;
  class CoreOS : public CoreModule {
    public:
      static const int RECV_BUFFER = 1;
      static const int SEND_BUFFER = 0;

      CoreOS (Core* core)
        : CoreModule(core)
      {}

      void constants (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void bufferSize (
        const String& seq,
        uint64_t peerId,
        size_t size,
        int buffer, // RECV_BUFFER, SEND_BUFFER
        const CoreModule::Callback& callback
      ) const;

      void cpus  (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void networkInterfaces (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void rusage (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void uname (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void uptime (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void hrtime (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void availableMemory (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;
  };
}
#endif
