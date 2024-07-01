#ifndef SOCKET_RUNTIME_CORE_CONDUIT_H
#define SOCKET_RUNTIME_CORE_CONDUIT_H

#include "../module.hh"
#include <iostream>

namespace SSC {
  class Core;
  class CoreConduit;


  class CoreConduit : public CoreModule {
    uv_tcp_t conduitSocket;
    struct sockaddr_in addr;
    mutable std::mutex clientsMutex;

    public:
      using Options = std::unordered_map<String, String>;

      struct EncodedMessage {
        Options options;
        std::vector<uint8_t> payload;

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

        std::map<String, String> getOptionsAsMap () {
          std::map<String, String> omap;

          for (const auto& pair : this->options) {
            omap.insert(pair);
          }
          return omap;
        }
      };

      class Client {
        public:
          uint64_t id;
          uint64_t clientId;
          uv_tcp_t handle;
          uv_write_t write_req;
          uv_shutdown_t shutdown_req;
          uv_buf_t buffer;
          int is_handshake_done;
          unsigned char *frame_buffer;
          size_t frame_buffer_size;

          CoreConduit* self;
          bool emit(const CoreConduit::Options& options, std::shared_ptr<char[]> payload_data, size_t length);

          Client(CoreConduit* self) : self(self), id(0), clientId(0), is_handshake_done(0) {
          }

          ~Client() {
            if (frame_buffer) {
              delete[] frame_buffer;
            }
            uv_close((uv_handle_t*)&handle, nullptr);
          }
      };

      CoreConduit (Core* core) : CoreModule(core) {};
      ~CoreConduit ();

      EncodedMessage decodeMessage (std::vector<uint8_t>& data);
      std::vector<uint8_t> encodeMessage (const CoreConduit::Options& options, const std::vector<uint8_t>& payload);
      bool has (uint64_t id) const;
      CoreConduit::Client* get (uint64_t id) const;

      std::map<uint64_t, Client*> clients;
      int port = 0;

      void open();
      void close();

    private:
      void handshake (Client *client, const char *request);
      void processFrame (Client *client, const char *frame, ssize_t len);
  };
}

#endif
