#include "core.hh"

namespace SSC {
  void Core::udpBind (String seq, uint64_t peerId, String address, int port, bool reuseAddr, Callback cb) {
    if (hasPeer(peerId) && getPeer(peerId)->isBound()) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.bind",
        "err": {
          "id": "$S",
          "code": "ERR_SOCKET_ALREADY_BOUND",
          "message": "Socket is already bound"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    dispatchEventLoop([=, this]() {
      auto peer = createPeer(PEER_TYPE_UDP, peerId);
      auto err = peer->bind(address, port, reuseAddr);

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.bind",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peerId), SSC::String(uv_strerror(err)));
        cb(seq, msg, Post{});
        return;
      }

      auto info = peer->getLocalPeerInfo();

      if (info->err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.bind",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peerId), SSC::String(uv_strerror(info->err)));
        cb(seq, msg, Post{});
        return;
      }

      auto msg = SSC::format(R"MSG({
        "source": "udp.bind",
        "data": {
          "event": "listening",
          "id": "$S",
          "address": "$S",
          "port": $i,
          "family": "$S"
        }
      })MSG", std::to_string(peerId), info->address, info->port, info->family);

      cb(seq, msg, Post{});
    });
  }

  void Core::udpConnect (String seq, uint64_t peerId, String address, int port, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto peer = createPeer(PEER_TYPE_UDP, peerId);
      auto err = peer->connect(address, port);

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.connect",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peerId), SSC::String(uv_strerror(err)));
        cb(seq, msg, Post{});
        return;
      }

      auto info = peer->getRemotePeerInfo();

      if (info->err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.getPeerName",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peerId), SSC::String(uv_strerror(info->err)));
        cb(seq, msg, Post{});
        return;
      }

      auto msg = SSC::format(R"MSG({
        "source": "udp.connect",
        "data": {
          "id": "$S",
          "address": "$S",
          "port": $i,
          "family": "$S"
        }
      })MSG", std::to_string(peerId), info->address, info->port, info->family);

      cb(seq, msg, Post{});
    });
  }

  void Core::udpDisconnect (String seq, uint64_t peerId, Callback cb) {
    dispatchEventLoop([=, this]() {
      if (!hasPeer(peerId)) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.disconnect",
          "err": {
            "id": "$S",
            "message": "No such peer"
          }
        })MSG", std::to_string(peerId));
        cb(seq, msg, Post{});
        return;
      }

      auto peer = getPeer(peerId);
      auto err = peer->disconnect();

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.disconnect",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peerId), SSC::String(uv_strerror(err)));
        cb(seq, msg, Post{});
        return;
      }

      auto msg = SSC::format(R"MSG({
        "source": "udp.disconnect",
        "data": {
          "id": "$S"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
    });
  }

  void Core::udpGetPeerName (String seq, uint64_t peerId, Callback cb) {
    if (!hasPeer(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getPeerName",
        "err": {
          "id": "$S",
          "message": "No such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = getPeer(peerId);
    auto info = peer->getRemotePeerInfo();

    if (info->err < 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getPeerName",
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), SSC::String(uv_strerror(info->err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "source": "udp.getPeerName",
      "data": {
        "id": "$S",
        "address": "$S",
        "port": $i,
        "family": "$S"
      }
    })MSG", std::to_string(peerId), info->address, info->port, info->family);

    cb(seq, msg, Post{});
  }

  void Core::udpGetSockName (String seq, uint64_t peerId, Callback cb) {
    if (!hasPeer(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getSockName",
        "err": {
          "id": "$S",
          "message": "No such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = getPeer(peerId);
    auto info = peer->getLocalPeerInfo();

    if (info->err < 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getSockName",
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), SSC::String(uv_strerror(info->err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(
      R"MSG({
        "source": "udp.getSockName",
        "data": {
          "id": "$S",
          "address": "$S",
          "port": $i,
          "family": "$S"
        }
      })MSG",
      std::to_string(peerId),
      info->address,
      info->port,
      info->family
    );

    cb(seq, msg, Post{});
  }

  void Core::udpGetState (String seq, uint64_t peerId,  Callback cb) {
    if (!hasPeer(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getState",
        "err": {
          "id": "$S",
          "code": "NOT_FOUND_ERR",
          "message": "No such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = getPeer(peerId);

    if (!peer->isUDP()) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getState",
        "err": {
          "id": "$S",
          "code": "NOT_FOUND_ERR",
          "message": "No such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(
      R"MSG({
        "source": "udp.getState",
        "data": {
          "id": "$S",
          "type": "udp",
          "ephemeral": $S,
          "bound": $S,
          "active": $S,
          "closing": $S,
          "closed": $S,
          "connected": $S
        }
      })MSG",
      std::to_string(peerId),
      SSC::String(peer->isEphemeral() ? "true" : "false"),
      SSC::String(peer->isBound() ? "true" : "false"),
      SSC::String(peer->isActive() ? "true" : "false"),
      SSC::String(peer->isClosing() ? "true" : "false"),
      SSC::String(peer->isClosed() ? "true" : "false"),
      SSC::String(peer->isConnected() ? "true" : "false")
    );

    cb(seq, msg, Post{});
  }

  void Core::udpSend (String seq, uint64_t peerId, char* buf, int len, int port, String address, bool ephemeral, Callback cb) {
    auto buffer = new char[len > 0 ? len : 1]{0};
    auto peer = createPeer(PEER_TYPE_UDP, peerId, ephemeral);
    auto ctx = new PeerRequestContext(seq, cb);

    memcpy(buffer, buf, len);
    ctx->bufsize = len;
    ctx->buf = buffer;
    ctx->port = port;
    ctx->address = address;
    ctx->peer = peer;

    dispatchEventLoop([=, this]() {
      ctx->peer->send(ctx->seq, ctx->buf, ctx->bufsize, ctx->port, ctx->address, [ctx](auto seq, auto msg, auto post) {
        ctx->cb(seq, msg, post);
        delete [] ctx->buf;
      });
    });
  }

  void Core::udpReadStart (String seq, uint64_t peerId, Callback cb) {
    if (!hasPeer(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.readStart",
        "err": {
          "id": "$S",
          "message": "No such peer"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    auto peer = getPeer(peerId);

    if (peer->isActive()) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.readStart",
        "data": {
          "id": "$S"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    if (peer->isClosing()) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.readStart",
        "err": {
          "id": "$S",
          "message": "Peer is closing"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    if (peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      auto msg = SSC::format(R"MSG({
       "source": "udp.readStart",
        "err": {
          "id": "$S",
          "message": "Peer is already receiving"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    auto err = peer->recvstart(cb);

    // `UV_EALREADY || UV_EBUSY` means there is active IO on the underlying handle
    if (err < 0 && err != UV_EALREADY && err != UV_EBUSY) {
      auto msg = SSC::format(
        R"MSG({
          "source": "udp.readStart",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(peerId),
        SSC::String(uv_strerror(err))
      );

      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "source": "udp.readStart",
      "data": {
        "id": "$S"
      }
    })MSG", std::to_string(peerId));
    cb(seq, msg, Post{});
  }

  void Core::udpReadStop (String seq, uint64_t peerId, Callback cb) {
    dispatchEventLoop([=, this] {
      if (!hasPeer(peerId)) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.readStop",
          "err": {
            "id": "$S",
            "message": "No such peer"
          }
        })MSG", std::to_string(peerId));

        cb(seq, msg, Post{});
        return;
      }

      auto peer = getPeer(peerId);

      if (peer->isClosing()) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.readStop",
          "err": {
            "id": "$S",
            "message": "Peer is closing"
          }
        })MSG", std::to_string(peerId));

        cb(seq, msg, Post{});
        return;
      }

      if (!peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
        auto msg = SSC::format(R"MSG({
         "source": "udp.readStop",
          "err": {
            "id": "$S",
            "message": "Peer is not receiving"
          }
        })MSG", std::to_string(peerId));

        cb(seq, msg, Post{});
        return;
      }

      auto err = peer->recvstop();

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.readStop",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(peerId),
        SSC::String(uv_strerror(err)));

        cb(seq, msg, Post{});
        return;
      }

      auto msg = SSC::format(R"MSG({
        "source": "udp.readStop",
        "data": {
          "id": "$S"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
    });
  }

  void Core::udpClose (String seq, uint64_t peerId, Callback cb) {
    dispatchEventLoop([=, this]() {
      if (!hasPeer(peerId)) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.close",
          "err": {
            "id": "$S",
            "code": "NOT_FOUND_ERR",
            "type": "NotFoundError",
            "message": "No peer with specified id"
          }
        })MSG", std::to_string(peerId));
        cb(seq, msg, Post{});
        return;
      }

      auto peer = getPeer(peerId);

      if (peer->isClosed() || peer->isClosing() || !peer->isUDP()) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.close",
          "err": {
            "id": "$S",
            "code": "ERR_SOCKET_DGRAM_NOT_RUNNING",
            "type": "InternalError",
            "message": "The socket has already been closed"
          }
        })MSG", std::to_string(peerId));
        cb(seq, msg, Post{});
        return;
      }

      peer->close([=, this]() {
        auto msg = SSC::format(R"MSG({
          "source": "close",
          "data": {
            "id": "$S"
          }
        })MSG", std::to_string(peerId));
        cb(seq, msg, Post{});
      });
    });
  }
}
