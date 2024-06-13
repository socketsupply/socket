#ifndef SOCKET_RUNTIME_IPC_CLIENT_H
#define SOCKET_RUNTIME_IPC_CLIENT_H

#include "../core/unique_client.hh"
#include "preload.hh"

namespace SSC::IPC {
  struct Client : public UniqueClient {
    using ID = UniqueClient::ID;
    ID id = 0;
    IPC::Preload preload = {};
  };
}
#endif
