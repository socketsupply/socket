#ifndef SOCKET_RUNTIME_CORE_SERVICES_CONDUIT_H
#define SOCKET_RUNTIME_CORE_SERVICES_CONDUIT_H

#include "../../core.hh"
#include "../../ipc.hh"

namespace ssc::runtime::core::services {
  class Conduit : public core::Service {
    public:
      using StartCallback = Function<void()>;

      struct Message {
        using Options = UnorderedMap<String, String>;
        Options options;
        Vector<uint8_t> payload;

        inline String get (const String& key) const;
        inline bool has (const String& key) const;
        inline String pluck (const String& key);
        inline Map<String, String> map () const;
        const inline bool empty () const;
        void clear ();
      };

      struct FrameBuffer {
        Vector<uint8_t> vector;

        inline const size_t size () const;
        inline const unsigned char* data () const;
        inline void resize (const size_t size);

        unsigned char operator [] (const unsigned int index) const;
        unsigned char& operator [] (const unsigned int index);

        const Vector<uint8_t> slice (Vector<uint8_t>::const_iterator&, Vector<uint8_t>::const_iterator&);
        template<typename T = uint8_t>
        const Vector<T> slice (Vector<uint8_t>::size_type, Vector<uint8_t>::size_type);
      };

      class Client {
        public:
          using CloseCallback = Function<void()>;
          using SendCallback = Function<void()>;
          using ID = uint64_t;

          ID id = 0;
          // client state
          ID clientId = 0;
          Atomic<bool> isHandshakeDone = false;
          Atomic<bool> isClosing = false;
          Atomic<bool> isClosed = false;
          Mutex mutex;

          // uv state
          uv_tcp_t handle;
          uv_buf_t buffer;
          uv_stream_t* stream = nullptr;

          // websocket frame buffer state
          FrameBuffer frameBuffer;
          Vector<Message> queue;
          Conduit* conduit = nullptr;

          Client (Conduit* conduit)
            : conduit(conduit),
              id(0),
              clientId(0),
              isHandshakeDone(0)
          {}

          ~Client ();

          bool send ( const Message::Options&, SharedPointer<unsigned char[]>, size_t, int opcode = 2, const SendCallback = nullptr);
          void close (const CloseCallback callback = nullptr);
      };

      // state
      std::map<uint64_t, Client*> clients;
      String sharedKey;
      Atomic<bool> isStarting = false;
      Atomic<int> port = 0;
      String hostname = "0.0.0.0";
      Mutex mutex;

      Conduit (const Service::Options& options);
      ~Conduit () noexcept override;

      // codec
      Message decodeMessage (const Vector<uint8_t>& data);
      Vector<uint8_t> encodeMessage (const Message::Options&, const Vector<uint8_t>&);

      // client access
      bool has (uint64_t id);
      Conduit::Client* get (uint64_t id);

      // lifecycle
      bool start (const StartCallback callback);
      bool start () override;
      bool stop () override;
      bool isActive ();

    private:
      uv_tcp_t socket;
      struct sockaddr_in addr;

      void handshake (Client*, const char*);
      void processFrame (Client*, const char*, ssize_t);
  };
}
#endif
