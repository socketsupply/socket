#ifndef SOCKET_RUNTIME_UNIQUE_CLIENT_H
#define SOCKET_RUNTIME_UNIQUE_CLIENT_H

#include "platform.hh"
#include "crypto.hh"

namespace ssc::runtime {
  struct UniqueClient {
    using ID = uint64_t;
    ID id = crypto::rand64();
    int index = -1;

    UniqueClient () = default;
    UniqueClient (ID id)
      : id(id)
    {}

    UniqueClient (ID id, int index)
      : id(id), index(index)
    {}

    UniqueClient (const UniqueClient& client)
      : id(client.id),
        index(client.index)
    {}

    UniqueClient (UniqueClient&& client)
      : id(client.id),
        index(client.index)
    {
      client.id = 0;
      client.index = -1;
    }

    virtual ~UniqueClient () {}

    UniqueClient& operator = (const UniqueClient& client) {
      this->id = client.id;
      this->index = client.index;
      return *this;
    }

    UniqueClient& operator = (UniqueClient&& client) {
      this->id = client.id;
      this->index = client.index;
      client.id = 0;
      client.index = -1;
      return *this;
    }
  };
}
#endif
