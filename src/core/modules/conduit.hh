#ifndef SOCKET_RUNTIME_CORE_CONDUIT_H
#define SOCKET_RUNTIME_CORE_CONDUIT_H

#include "../module.hh"
#include <iostream>

namespace SSC {
  class Core;

  class CoreConduit : public CoreModule {
    public:
      using Options = std::unordered_map<String, String>;

      struct EncodedMessage {
        Options options;
        Vector<uint8_t> payload;

        inline String get (const String& key) const {
          const auto it = options.find(key);
          if (it != options.end()) {
            return it->second;
          }
          return "";
        }

        inline bool has (const String& key) const {
          const auto it = options.find(key);
          if (it != options.end()) {
            return true;
          }
          return false;
        }

        inline String pluck (const String& key) {
          auto it = options.find(key);
          if (it != options.end()) {
            String value = it->second;
            options.erase(it);
            return value;
          }
          return "";
        }

        inline Map getOptionsAsMap () {
          Map map;

          for (const auto& pair : this->options) {
            map.insert(pair);
          }
          return map;
        }
      };

      class Client {
        public:
          // client state
          uint64_t id;
          uint64_t clientId;
          Atomic<bool> isHandshakeDone;

          // uv statae
          uv_tcp_t handle;
          uv_buf_t buffer;

          // websocket frame buffer state
          unsigned char *frameBuffer;
          size_t frameBufferSize;

          CoreConduit* conduit;

          Client (CoreConduit* conduit)
            : conduit(conduit),
              id(0),
              clientId(0),
              isHandshakeDone(0)
          {}

          ~Client () {
            auto handle = reinterpret_cast<uv_handle_t*>(&this->handle);

            if (frameBuffer) {
              delete [] frameBuffer;
            }

            if (handle->loop != nullptr && !uv_is_closing(handle)) {
              uv_close(handle, nullptr);
            }
          }

          bool emit (
            const CoreConduit::Options& options,
            SharedPointer<char[]> payload,
            size_t length
          );
      };

      // state
      std::map<uint64_t, Client*> clients;
      Mutex mutex;
      Atomic<bool> isStarted = false;
      Atomic<int> port = 0;

      CoreConduit (Core* core) : CoreModule(core) {};
      ~CoreConduit ();

      // codec
      EncodedMessage decodeMessage (const Vector<uint8_t>& data);
      Vector<uint8_t> encodeMessage (
        const Options& options,
        const Vector<uint8_t>& payload
      );

      // client access
      bool has (uint64_t id);
      CoreConduit::Client* get (uint64_t id);

      // lifecycle
      void start ();
      void stop ();

    private:
      uv_tcp_t socket;
      struct sockaddr_in addr;

      void handshake (Client* client, const char *request);
      void processFrame (Client* client, const char *frame, ssize_t size);
  };
}
#endif
