#include "core.hh"

namespace SSC {
  static JSON::Object::Entries ERR_SOCKET_ALREADY_BOUND (uint64_t id) {
    return JSON::Object::Entries {
      {"source", "udp.bind"},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_ALREADY_BOUND"},
        {"message", "Socket is already bound"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_IS_CONNECTED (uint64_t id) {
    return JSON::Object::Entries {
      {"source", "udp.connect"},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_DGRAM_IS_CONNECTED"},
        {"message", "Already connected"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_NOT_CONNECTED (String source, uint64_t id) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_DGRAM_NOT_CONNECTED"},
        {"message", "Not connected"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_CLOSED (String source, uint64_t id) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotFoundError"},
        {"code", "ERR_SOCKET_DGRAM_NOT_RUNNING"},
        {"message", "Socket is closed"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_NOT_RUNNING (String source, uint64_t id) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotFoundError"},
        {"code", "ERR_SOCKET_DGRAM_NOT_RUNNING"},
        {"message", "Not running"}
      }}
    };
  }

  void Core::UDP::bind (
    String seq,
    uint64_t peerId,
    BindOptions options,
    Module::Callback cb
  ) {
    auto address = options.address;
    auto port = options.port;
    auto reuseAddr = options.reuseAddr;

    if (this->core->hasPeer(peerId) && this->core->getPeer(peerId)->isBound()) {
      auto json = ERR_SOCKET_ALREADY_BOUND(peerId);
      cb(seq, json, Post{});
      return;
    }

    this->core->dispatchEventLoop([=, this]() {
      auto peer = this->core->createPeer(PEER_TYPE_UDP, peerId);
      auto err = peer->bind(address, port, reuseAddr);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.bind"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto info = peer->getLocalPeerInfo();

      if (info->err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.bind"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(info->err))}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.bind"},
        {"data", JSON::Object::Entries {
          {"event" , "listening" },
          {"address", info->address},
          {"family", info->family},
          {"port", info->port},
          {"id", std::to_string(peerId)}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void Core::UDP::connect (
    String seq,
    uint64_t peerId,
    ConnectOptions options,
    Module::Callback cb
  ) {
    auto address = options.address;
    auto port = options.port;
    this->core->dispatchEventLoop([=, this]() {
      auto peer = this->core->createPeer(PEER_TYPE_UDP, peerId);
      auto err = peer->connect(address, port);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.connect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto info = peer->getRemotePeerInfo();

      if (info->err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.connect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(info->err))}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.connect"},
        {"data", JSON::Object::Entries {
          {"address", info->address},
          {"family", info->family},
          {"port", info->port},
          {"id", std::to_string(peerId)}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void Core::UDP::disconnect (String seq, uint64_t peerId, Module::Callback cb) {
    this->core->dispatchEventLoop([=, this]() {
      if (!this->core->hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_CONNECTED("udp.disconnect", peerId);
        cb(seq, json, Post{});
        return;
      }

      auto peer = this->core->getPeer(peerId);
      auto err = peer->disconnect();

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.disconnect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.disconnect"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void Core::UDP::getPeerName (String seq, uint64_t peerId, Module::Callback cb) {
    if (!this->core->hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_CONNECTED("udp.getPeerName", peerId);
      cb(seq, json, Post{});
      return;
    }

    auto peer = this->core->getPeer(peerId);
    auto info = peer->getRemotePeerInfo();

    if (info->err < 0) {
      auto json = JSON::Object::Entries {
        {"source", "udp.getPeerName"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", String(uv_strerror(info->err))}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getPeerName"},
      {"data", JSON::Object::Entries {
        {"address", info->address},
        {"family", info->family},
        {"port", info->port},
        {"id", std::to_string(peerId)}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::UDP::getSockName (String seq, uint64_t peerId, Callback cb) {
    if (!this->core->hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getSockName", peerId);
      cb(seq, json, Post{});
      return;
    }

    auto peer = this->core->getPeer(peerId);
    auto info = peer->getLocalPeerInfo();

    if (info->err < 0) {
      auto json = JSON::Object::Entries {
        {"source", "udp.getSockName"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", String(uv_strerror(info->err))}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getSockName"},
      {"data", JSON::Object::Entries {
        {"address", info->address},
        {"family", info->family},
        {"port", info->port},
        {"id", std::to_string(peerId)}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::UDP::getState (String seq, uint64_t peerId,  Callback cb) {
    if (!this->core->hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getState", peerId);
      cb(seq, json, Post{});
      return;
    }

    auto peer = this->core->getPeer(peerId);

    if (!peer->isUDP()) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getState", peerId);
      cb(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getState"},
      {"data", JSON::Object::Entries {
        {"type", "udp"},
        {"ephemeral", peer->isEphemeral()},
        {"connected", peer->isConnected()},
        {"closing", peer->isClosing()},
        {"closed", peer->isClosed()},
        {"active", peer->isActive()},
        {"bound", peer->isBound()},
        {"id", std::to_string(peerId)}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::UDP::send (
    String seq,
    uint64_t peerId,
    SendOptions options,
    Module::Callback cb
  ) {
    auto size = options.size;
    auto port = options.port;
    auto peer = this->core->createPeer(PEER_TYPE_UDP, peerId, options.ephemeral);
    auto buffer = new char[size > 0 ? size : 1]{0};
    auto address = options.address;

    memcpy(buffer, options.bytes, size);

    this->core->dispatchEventLoop([=, this]() {
      peer->send(buffer, size, port, address, [=](auto status, auto post) {
        delete [] buffer;

        if (status < 0) {
          auto json = JSON::Object::Entries {
            {"source", "udp.send"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(peerId)},
              {"message", String(uv_strerror(status))}
            }}
          };

          cb(seq, json, Post{});
          return;
        }

        auto json = JSON::Object::Entries {
          {"source", "udp.send"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"status", status}
          }}
        };

        cb(seq, json, Post{});
      });
    });
  }

  void Core::UDP::readStart (String seq, uint64_t peerId, Module::Callback cb, Peer::UDPReceiveCallback receive) {
    if (!this->core->hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.readStart", peerId);
      cb(seq, json, Post{});
      return;
    }

    auto peer = this->core->getPeer(peerId);

    if (peer->isActive()) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };
      cb(seq, json, Post{});
      return;
    }

    if (peer->isClosing()) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", "Peer is closing"}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    if (peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", "Peer is already receiving"}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto err = peer->recvstart(receive);

    // `UV_EALREADY || UV_EBUSY` means there is active IO on the underlying handle
    if (err < 0 && err != UV_EALREADY && err != UV_EBUSY) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", String(uv_strerror(err))}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.readStart"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(peerId)}
      }}
    };

    cb(seq, json, Post {});
  }

  void Core::UDP::readStop (String seq, uint64_t peerId, Callback cb) {
    this->core->dispatchEventLoop([=, this] {
      if (!this->core->hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.readStart", peerId);
        cb(seq, json, Post{});
        return;
      }

      auto peer = this->core->getPeer(peerId);

      if (peer->isClosing()) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStart"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", "Peer is closing"}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      if (!peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStart"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", "Peer is not receiving"}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto err = peer->recvstop();

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStop"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.readStop"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };

      cb(seq, json, Post {});
    });
  }

  void Core::UDP::close (String seq, uint64_t peerId, Module::Callback cb) {
    this->core->dispatchEventLoop([=, this]() {
      if (!this->core->hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.close", peerId);
        cb(seq, json, Post{});
        return;
      }

      auto peer = this->core->getPeer(peerId);

      if (!peer->isUDP()) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.close", peerId);
        cb(seq, json, Post{});
        return;
      }

      if (peer->isClosed() || peer->isClosing()) {
        auto json = ERR_SOCKET_DGRAM_CLOSED("udp.close", peerId);
        cb(seq, json, Post{});
        return;
      }

      peer->close([=, this]() {
        auto json = JSON::Object::Entries {
          {"source", "udp.close"},
          {"data", {
            {"id", std::to_string(peerId)}
          }}
        };

        cb(seq, json, Post{});
      });
    });
  }
}
