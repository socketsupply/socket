#ifndef SOCKET_RUNTIME_WEBVIEW_CLIENT_H
#define SOCKET_RUNTIME_WEBVIEW_CLIENT_H

#include "../unique_client.hh"
#include "preload.hh"

namespace ssc::runtime::webview {
  struct Client : public UniqueClient {
    using UniqueClient::UniqueClient;
    Preload preload;

    Client (const UniqueClient& client)
      : UniqueClient(client)
    {}
  };
}
#endif
