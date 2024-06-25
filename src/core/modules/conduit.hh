#ifndef SOCKET_RUNTIME_CORE_CONDUIT_H
#define SOCKET_RUNTIME_CORE_CONDUIT_H

#include "../module.hh"

namespace SSC {
  class Core;
  class CoreConduit : public CoreModule {
    using Callback = std::function<void(int, unsigned char*)>;

    uv_tcp_t conduitSocket;
    struct sockaddr_in addr;

    typedef struct {
      uv_tcp_t handle;
      uv_write_t write_req;
      uv_shutdown_t shutdown_req;
      uv_buf_t buffer;
      int is_handshake_done;
      unsigned char *frame_buffer;
      size_t frame_buffer_size;
      Conduit* self;

      bool emit(std::shared_ptr<char[]> message, size_t length);
    } Client;

    std::map<unit64_t, SharedPointer<Client>> clients;
    std::map<uint64_t, Callback> listeners;

    void handshake(client_t *client, const char *request);
    void processFrame(client_t *client, const char *frame, ssize_t len);

    public:
      CoreConduit (Core* core) : CoreModule(core) {};
      ~CoreConduit ();

      int port = 0;

      void open();
      bool close();
      void addListener(uint64_t key, Callback callback);
      void removeListener(uint64_t key);
  }
}

#endif
