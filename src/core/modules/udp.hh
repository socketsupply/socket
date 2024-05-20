#ifndef SOCKET_RUNTIME_CORE_MODULE_UDP_H
#define SOCKET_RUNTIME_CORE_MODULE_UDP_H

#include "../module.hh"
#include "../socket.hh"

namespace SSC {
  class Core;
  class CoreUDP : public CoreModule {
    public:
      using ID = uint64_t;
      using Sockets = std::map<ID, SharedPointer<Socket>>;

      struct BindOptions {
        String address;
        int port;
        bool reuseAddr = false;
      };

      struct ConnectOptions {
        String address;
        int port;
      };

      struct SendOptions {
        String address = "";
        int port = 0;
        SharedPointer<char[]> bytes = nullptr;
        size_t size = 0;
        bool ephemeral = false;
      };

      Mutex mutex;
      Sockets sockets;

      CoreUDP (Core* core)
        : CoreModule(core)
      {}

      void bind (
        const String& seq,
        ID id,
        const BindOptions& options,
        const CoreModule::Callback& callback
      );

      void close (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void connect (
        const String& seq,
        ID id,
        const ConnectOptions& options,
        const CoreModule::Callback& callback
      );

      void disconnect (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void getPeerName (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void getSockName (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void getState (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void readStart (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void readStop (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void send (
        const String& seq,
        ID id,
        const SendOptions& options,
        const CoreModule::Callback& callback
      );

      void resumeAllSockets ();
      void pauseAllSockets ();
      bool hasSocket (ID id);
      void removeSocket (ID id);
      void removeSocket (ID id, bool autoClose);
      SharedPointer<Socket> getSocket (ID id);
      SharedPointer<Socket> createSocket (socket_type_t type, ID id);
      SharedPointer<Socket> createSocket (socket_type_t type, ID id, bool isEphemeral);
  };
}
#endif
