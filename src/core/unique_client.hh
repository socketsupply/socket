#ifndef SOCKET_RUNTIME_UNIQUE_CLIENT_H
#define SOCKET_RUNTIME_UNIQUE_CLIENT_H

#include "../platform/platform.hh"

namespace SSC {
  struct UniqueClient {
    using ID = uint64_t;
    ID id = 0;
    int index = 0;
  };
}
#endif
