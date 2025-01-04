#ifndef SOCKET_RUNTIME_CORE_SERVICES_UDP_H
#define SOCKET_RUNTIME_CORE_SERVICES_UDP_H

#include "../../core.hh"
#include "../../udp.hh"
#include "../../ipc.hh"

namespace ssc::runtime::core::services {
  class UDP : public core::Service {
    public:
      using ID = uint64_t;

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
        SharedPointer<unsigned char[]> bytes = nullptr;
        size_t size = 0;
        bool ephemeral = false;
      };

      Mutex mutex;
      udp::SocketManager manager;

      UDP (const Options& options)
        : core::Service(options),
          manager({ options.loop })
      {}

      void bind (const ipc::Message::Seq&, ID, const BindOptions&, const Callback);
      void close (const ipc::Message::Seq&, ID, const Callback);
      void connect (const ipc::Message::Seq&, ID, const ConnectOptions&, const Callback);
      void disconnect (const ipc::Message::Seq&, ID, const Callback);
      void getPeerName (const ipc::Message::Seq&, ID, const Callback);
      void getSockName (const ipc::Message::Seq&, ID, const Callback);
      void getState (const ipc::Message::Seq&, ID, const Callback);
      void readStart (const ipc::Message::Seq&, ID, const Callback);
      void readStop (const ipc::Message::Seq&, ID, const Callback);
      void send (const ipc::Message::Seq&, ID, const SendOptions& options, const Callback);
      bool start ();
      bool stop ();
      void resumeAllSockets ();
      void pauseAllSockets ();
      bool hasSocket (ID);
      void removeSocket (ID);
      void removeSocket (ID, bool autoClose);
      SharedPointer<udp::Socket> getSocket (ID);
      SharedPointer<udp::Socket> createSocket (udp::socket_type_t type, ID);
      SharedPointer<udp::Socket> createSocket (udp::socket_type_t type, ID, bool isEphemeral);
  };
}
#endif
