#ifndef SOCKET_RUNTIME_IPC_CLIENT_H
#define SOCKET_RUNTIME_IPC_CLIENT_H

#include "../core/core.hh"
#include "preload.hh"

namespace SSC::IPC {
  struct Client {
    using ID = uint64_t;
    ID id = 0;
    IPC::Preload preload;
  };
}
#endif
