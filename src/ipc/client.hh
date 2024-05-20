#ifndef SOCKET_RUNTIME_IPC_CLIENT_H
#define SOCKET_RUNTIME_IPC_CLIENT_H

#include "../core/core.hh"

namespace SSC::IPC {
  struct Client {
    struct CurrentRequest {
      String url;
    };

    String id;
    CurrentRequest currentRequest;
  };
}
#endif
