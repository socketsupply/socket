#ifndef SOCKET_RUNTIME_CORE_CONDUIT_H
#define SOCKET_RUNTIME_CORE_CONDUIT_H

#include "../module.hh"

namespace SSC {
  class Core;
  class CoreConduit : public CoreModule {
    uv_tcp_t conduitSocket;
    struct sockaddr_in addr;
    mutable std::mutex clientsMutex;

    void handshake (client_t *client, const char *request);
    void processFrame (client_t *client, const char *frame, ssize_t len);

    public:
      using Options = std::unordered_map<String, String>;

      struct EncodedMessage {
        Options options;
        Vector<uint8_t> payload;

        String get (const String& key) const {
          auto it = options.find(key);
          if (it != options.end()) {
            return it->second;
          }
          return "";
        }

        String pluck (const String& key) {
          auto it = options.find(key);
          if (it != options.end()) {
            String value = it->second;
            options.erase(it);
            return value;
          }
          return "";
        }
      };

      struct Client{
        uv_tcp_t handle;
        uv_write_t write_req;
        uv_shutdown_t shutdown_req;
        uv_buf_t buffer;
        int is_handshake_done;
        unsigned char *frame_buffer;
        size_t frame_buffer_size;
        Conduit* self;
        String route = "";
        bool emit(const Options& options, const unsigned char* payload_data, size_t length);
      };

      CoreConduit (Core* core) : CoreModule(core) {};
      ~CoreConduit ();

      EncodedMessage decodeMessage (const unsigned char* payload_data, int payload_len);
      EncodedMessage decodeMessage(const Vector<uint8_t>& data);
      Vector<uint8_t> encodeMessage(const CoreConduit::Options& options, const Vector<uint8_t>& payload);
      bool has (uint64_t id);

      CoreConduit::Client get(uint64_t id) const;
      std::shared_ptr<CoreConduit::Client> get (uint64_t id) const;

      std::map<unit64_t, SharedPointer<Client>> clients;
      int port = 0;

      void open();
      bool close();
  }
}

#endif
