#include "../headers.hh"
#include "../core.hh"
#include "../ip.hh"
#include "udp.hh"

namespace SSC {
  static JSON::Object::Entries ERR_SOCKET_ALREADY_BOUND (
    const String& source,
    CoreUDP::ID id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_ALREADY_BOUND"},
        {"message", "Socket is already bound"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_IS_CONNECTED (
    const String& source,
    CoreUDP::ID id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_DGRAM_IS_CONNECTED"},
        {"message", "Already connected"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_NOT_CONNECTED (
    const String& source,
    CoreUDP::ID id
  ) {
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

  static JSON::Object::Entries ERR_SOCKET_DGRAM_CLOSED (
    const String& source,
    CoreUDP::ID id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_DGRAM_CLOSED"},
        {"message", "Socket is closed"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_CLOSING (
    const String& source,
    CoreUDP::ID id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotFoundError"},
        {"code", "ERR_SOCKET_DGRAM_CLOSING"},
        {"message", "Socket is closing"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_NOT_RUNNING (
    const String& source,
    CoreUDP::ID id
  ) {
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

  void CoreUDP::resumeAllSockets () {
    this->core->dispatchEventLoop([=, this]() {
      for (auto const &tuple : this->sockets) {
        auto socket = tuple.second;
        if (socket != nullptr && (socket->isBound() || socket->isConnected())) {
          socket->resume();
        }
      }
    });
  }

  void CoreUDP::pauseAllSockets () {
    this->core->dispatchEventLoop([=, this]() {
      for (auto const &tuple : this->sockets) {
        auto socket = tuple.second;
        if (socket != nullptr && (socket->isBound() || socket->isConnected())) {
          socket->pause();
        }
      }
    });
  }

  bool CoreUDP::hasSocket (ID id) {
    Lock lock(this->mutex);
    return this->sockets.find(id) != this->sockets.end();
  }

  void CoreUDP::removeSocket (ID id) {
    return this->removeSocket(id, false);
  }

  void CoreUDP::removeSocket (ID id, bool autoClose) {
    if (this->hasSocket(id)) {
      if (autoClose) {
        auto socket = this->getSocket(id);
        if (socket != nullptr) {
          socket->close();
        }
      }

      Lock lock(this->mutex);
      this->sockets.erase(id);
    }
  }

  SharedPointer<Socket> CoreUDP::getSocket (ID id) {
    if (!this->hasSocket(id)) return nullptr;
    Lock lock(this->mutex);
    return this->sockets.at(id);
  }

  SharedPointer<Socket> CoreUDP::createSocket (socket_type_t socketType, ID id) {
    return this->createSocket(socketType, id, false);
  }

  SharedPointer<Socket> CoreUDP::createSocket (
    socket_type_t socketType,
    ID id,
    bool isEphemeral
  ) {
    if (this->hasSocket(id)) {
      auto socket = this->getSocket(id);
      if (socket != nullptr) {
        if (isEphemeral) {
          Lock lock(socket->mutex);
          socket->flags = (socket_flag_t) (socket->flags | SOCKET_FLAG_EPHEMERAL);
        }
      }

      return socket;
    }

    Lock lock(this->mutex);
    this->sockets[id].reset(new Socket(this->core, socketType, id, isEphemeral));
    return this->sockets.at(id);
  }

  void CoreUDP::bind (
    const String& seq,
    ID id,
    const CoreUDP::BindOptions& options,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      if (this->hasSocket(id)) {
        if (this->getSocket(id)->isBound()) {
          auto json = ERR_SOCKET_ALREADY_BOUND("udp.bind", id);
          return callback(seq, json, Post{});
        }
      }

      auto socket = this->createSocket(SOCKET_TYPE_UDP, id);
      auto err = socket->bind(options.address, options.port, options.reuseAddr);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.bind"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto info = socket->getLocalPeerInfo();

      if (info->err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.bind"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", String(uv_strerror(info->err))}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.bind"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"port", (int) info->port},
          {"event" , "listening"},
          {"family", info->family},
          {"address", info->address}
        }}
      };

      callback(seq, json, Post{});
    });
  }

  void CoreUDP::connect (
    const String& seq,
    ID id,
    const CoreUDP::ConnectOptions& options,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto socket = this->createSocket(SOCKET_TYPE_UDP, id);

      if (socket->isConnected()) {
        auto json = ERR_SOCKET_DGRAM_IS_CONNECTED("udp.connect", id);
        return callback(seq, json, Post{});
      }

      auto err = socket->connect(options.address, options.port);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.connect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto info = socket->getRemotePeerInfo();

      if (info->err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.connect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", String(uv_strerror(info->err))}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.connect"},
        {"data", JSON::Object::Entries {
          {"address", info->address},
          {"family", info->family},
          {"port", (int) info->port},
          {"id", std::to_string(id)}
        }}
      };

      callback(seq, json, Post{});
    });
  }

  void CoreUDP::disconnect (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      if (!this->hasSocket(id)) {
        auto json = ERR_SOCKET_DGRAM_NOT_CONNECTED("udp.disconnect", id);
        return callback(seq, json, Post{});
      }

      auto socket = this->getSocket(id);
      auto err = socket->disconnect();

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.disconnect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.disconnect"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)}
        }}
      };

      callback(seq, json, Post{});
    });
  }

  void CoreUDP::getPeerName (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    if (!this->hasSocket(id)) {
      auto json = ERR_SOCKET_DGRAM_NOT_CONNECTED("udp.getPeerName", id);
      return callback(seq, json, Post{});
    }

    auto socket = this->getSocket(id);
    auto info = socket->getRemotePeerInfo();

    if (info->err < 0) {
      auto json = JSON::Object::Entries {
        {"source", "udp.getPeerName"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"message", String(uv_strerror(info->err))}
        }}
      };

      return callback(seq, json, Post{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getPeerName"},
      {"data", JSON::Object::Entries {
        {"address", info->address},
        {"family", info->family},
        {"port", (int) info->port},
        {"id", std::to_string(id)}
      }}
    };

    callback(seq, json, Post{});
  }

  void CoreUDP::getSockName (
    const String& seq,
    ID id,
    const Callback& callback
  ) {
    if (!this->hasSocket(id)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getSockName", id);
      return callback(seq, json, Post{});
    }

    auto socket = this->getSocket(id);
    auto info = socket->getLocalPeerInfo();

    if (info->err < 0) {
      auto json = JSON::Object::Entries {
        {"source", "udp.getSockName"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"message", String(uv_strerror(info->err))}
        }}
      };

      return callback(seq, json, Post{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getSockName"},
      {"data", JSON::Object::Entries {
        {"address", info->address},
        {"family", info->family},
        {"port", (int) info->port},
        {"id", std::to_string(id)}
      }}
    };

    callback(seq, json, Post{});
  }

  void CoreUDP::getState (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    if (!this->hasSocket(id)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getState", id);
      return callback(seq, json, Post{});
    }

    auto socket = this->getSocket(id);

    if (!socket->isUDP()) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getState", id);
      return callback(seq, json, Post{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getState"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "udp"},
        {"bound", socket->isBound()},
        {"active", socket->isActive()},
        {"closed", socket->isClosed()},
        {"closing", socket->isClosing()},
        {"connected", socket->isConnected()},
        {"ephemeral", socket->isEphemeral()}
      }}
    };

    callback(seq, json, Post{});
  }

  void CoreUDP::send (
    const String& seq,
    ID id,
    const CoreUDP::SendOptions& options,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this] {
      auto socket = this->createSocket(SOCKET_TYPE_UDP, id, options.ephemeral);
      auto size = options.size; // @TODO(jwerle): validate MTU
      auto port = options.port;
      auto bytes = options.bytes;
      auto address = options.address;
      socket->send(bytes, size, port, address, [=](auto status, auto post) {
        if (status < 0) {
          auto json = JSON::Object::Entries {
            {"source", "udp.send"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"message", String(uv_strerror(status))}
            }}
          };

          return callback(seq, json, Post{});
        }

        auto json = JSON::Object::Entries {
          {"source", "udp.send"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"status", status}
          }}
        };

        callback(seq, json, Post{});
      });
    });
  }

  void CoreUDP::readStart (const String& seq, ID id, const CoreModule::Callback& callback) {
    if (!this->hasSocket(id)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.readStart", id);
      return callback(seq, json, Post{});
    }

    auto socket = this->getSocket(id);

    if (socket->isClosed()) {
      auto json = ERR_SOCKET_DGRAM_CLOSED("udp.readStart", id);
      return callback(seq, json, Post{});
    }

    if (socket->isClosing()) {
      auto json = ERR_SOCKET_DGRAM_CLOSING("udp.readStart", id);
      return callback(seq, json, Post{});
    }

    if (socket->hasState(SOCKET_STATE_UDP_RECV_STARTED)) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"message", "Socket is already receiving"}
        }}
      };

      return callback(seq, json, Post{});
    }

    if (socket->isActive()) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)}
        }}
      };

      return callback(seq, json, Post{});
    }

    auto err = socket->recvstart([=](auto nread, auto buf, auto addr) {
      if (nread == UV_EOF) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStart"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"EOF", true}
          }}
        };

        callback("-1", json, Post{});
      } else if (nread > 0 && buf && buf->base) {
        char address[17] = {0};
        Post post;
        int port;

        IP::parseAddress((struct sockaddr *) addr, &port, address);

        const auto headers = Headers {{
          {"content-type" ,"application/octet-stream"},
          {"content-length", nread}
        }};

        post.id = rand64();
        post.body.reset(buf->base);
        post.length = (int) nread;
        post.headers = headers.str();

        const auto json = JSON::Object::Entries {
          {"source", "udp.readStart"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"port", port},
            {"bytes", std::to_string(post.length)},
            {"address", address}
          }}
        };

        callback("-1", json, post);
      }
    });

    // `UV_EALREADY || UV_EBUSY` could mean there might be
    // active IO on the underlying handle
    if (err < 0 && err != UV_EALREADY && err != UV_EBUSY) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"message", String(uv_strerror(err))}
        }}
      };

      return callback(seq, json, Post{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.readStart"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(id)}
      }}
    };

    callback(seq, json, Post {});
  }

  void CoreUDP::readStop (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this] {
      if (!this->hasSocket(id)) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.readStop", id);
        return callback(seq, json, Post{});
      }

      auto socket = this->getSocket(id);

      if (socket->isClosed()) {
        auto json = ERR_SOCKET_DGRAM_CLOSED("udp.readStop", id);
        return callback(seq, json, Post{});
      }

      if (socket->isClosing()) {
        auto json = ERR_SOCKET_DGRAM_CLOSING("udp.readStop", id);
        return callback(seq, json, Post{});
      }

      if (!socket->hasState(SOCKET_STATE_UDP_RECV_STARTED)) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStop"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", "Socket is not receiving"}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto err = socket->recvstop();

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStop"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.readStop"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)}
        }}
      };

      callback(seq, json, Post {});
    });
  }

  void CoreUDP::close (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      if (!this->hasSocket(id)) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.close", id);
        return callback(seq, json, Post{});
      }

      auto socket = this->getSocket(id);

      if (!socket->isUDP()) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.close", id);
        return callback(seq, json, Post{});
      }

      if (socket->isClosed()) {
        auto json = ERR_SOCKET_DGRAM_CLOSED("udp.close", id);
        return callback(seq, json, Post{});
      }

      if (socket->isClosing()) {
        auto json = ERR_SOCKET_DGRAM_CLOSING("udp.close", id);
        return callback(seq, json, Post{});
      }

      socket->close([=, this]() {
        auto json = JSON::Object::Entries {
          {"source", "udp.close"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)}
          }}
        };

        callback(seq, json, Post{});
      });
    });
  }
}
