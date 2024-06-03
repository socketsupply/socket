#ifndef SOCKET_RUNTIME_CORE_POST_H
#define SOCKET_RUNTIME_CORE_POST_H

#include "../platform/types.hh"

namespace SSC {
  struct Post {
    using EventStreamCallback = Function<bool(
      const char*,
      const char*,
      bool
    )>;

    using ChunkStreamCallback = Function<bool(
      const char*,
      size_t,
      bool
    )>;

    uint64_t id = 0;
    uint64_t ttl = 0;
    SharedPointer<char[]> body = nullptr;
    size_t length = 0;
    String headers = "";
    String workerId = "";
    SharedPointer<EventStreamCallback> eventStream = nullptr;
    SharedPointer<ChunkStreamCallback> chunkStream = nullptr;
  };

  using Posts = std::map<uint64_t, Post>;
}
#endif
