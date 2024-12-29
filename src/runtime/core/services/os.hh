#ifndef SOCKET_RUNTIME_CORE_SERVICES_OS_H
#define SOCKET_RUNTIME_CORE_SERVICES_OS_H

#include "../../ipc.hh"
#include "../../core.hh"

namespace ssc::runtime::core::services {
  class OS : public core::Service {
    public:
      static const int RECV_BUFFER = 1;
      static const int SEND_BUFFER = 0;

      OS (const Options& options)
        : core::Service(options)
      {}

      void cpus ( const ipc::Message::Seq&, const Callback) const;
      void rusage (const ipc::Message::Seq&, const Callback) const;
      void uname (const ipc::Message::Seq&, const Callback) const;
      void uptime (const ipc::Message::Seq&, const Callback) const;
      void hrtime (const ipc::Message::Seq&, const Callback) const;
      void constants (const ipc::Message::Seq&, const Callback) const;
      void bufferSize (const ipc::Message::Seq&, uint64_t, size_t, int /* RECV_BUFFER|SEND_BUFFER */, const Callback) const;
      void availableMemory (const ipc::Message::Seq&, const Callback) const;
      void networkInterfaces (const ipc::Message::Seq&, const Callback) const;
  };
}
#endif
