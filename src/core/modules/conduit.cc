#include "../../app/app.hh"
#include "../core.hh"
#include "../codec.hh"

#include "conduit.hh"

#define SHA_DIGEST_LENGTH 20

namespace SSC {
  static constexpr char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  static SharedPointer<char[]> vectorToSharedPointer (const Vector<uint8_t>& vector) {
    const auto size = vector.size();
    const auto data = vector.data();
    const auto pointer = std::make_shared<char[]>(size);
    std::memcpy(pointer.get(), data, size);
    return std::move(pointer);
  }

  CoreConduit::CoreConduit (Core* core) : CoreModule(core) {
    auto userConfig = getUserConfig();
    const auto sharedKey = Env::get(
      "SOCKET_RUNTIME_CONDUIT_SHARED_KEY",
      userConfig["application_conduit_shared_key"]
    );

    if (sharedKey.size() >= 8) {
      this->sharedKey = sharedKey;
    } else {
      this->sharedKey = std::to_string(rand64());
    }
  }

  CoreConduit::~CoreConduit () {
    this->stop();
  }

  Vector<uint8_t> CoreConduit::encodeMessage (
    const CoreConduit::Options& options,
    const Vector<uint8_t>& payload
  ) {
    Vector<uint8_t> encodedMessage;

    Vector<std::pair<String, String>> sortedOptions(options.begin(), options.end());
    std::sort(sortedOptions.begin(), sortedOptions.end());

    // the total number of options
    encodedMessage.push_back(static_cast<uint8_t>(sortedOptions.size()));

    for (const auto& option : sortedOptions) {
      const String& key = option.first;
      const String& value = option.second;

      // ket length
      encodedMessage.push_back(static_cast<uint8_t>(key.length()));

      // key
      encodedMessage.insert(encodedMessage.end(), key.begin(), key.end());

      // value length
      uint16_t valueLength = static_cast<uint16_t>(value.length());
      encodedMessage.push_back(static_cast<uint8_t>((valueLength >> 8) & 0xFF));
      encodedMessage.push_back(static_cast<uint8_t>(valueLength & 0xFF));

      // value
      encodedMessage.insert(encodedMessage.end(), value.begin(), value.end());
    }

    // payload length
    uint16_t bodyLength = static_cast<uint16_t>(payload.size());
    encodedMessage.push_back(static_cast<uint8_t>((bodyLength >> 8) & 0xFF));
    encodedMessage.push_back(static_cast<uint8_t>(bodyLength & 0xFF));

    // actual payload
    encodedMessage.insert(encodedMessage.end(), payload.begin(), payload.end());
    return encodedMessage;
  }

  CoreConduit::DecodedMessage CoreConduit::decodeMessage (
    const Vector<uint8_t>& data
  ) {
    DecodedMessage message;

    if (data.size() < 1) return message;

    size_t offset = 0;

    uint8_t numOpts = data[offset++];

    for (uint8_t i = 0; i < numOpts; ++i) {
      if (offset >= data.size()) continue;

      // len
      uint8_t keyLength = data[offset++];
      if (offset + keyLength > data.size()) continue;
      // key
      String key(data.begin() + offset, data.begin() + offset + keyLength);
      offset += keyLength;

      if (offset + 2 > data.size()) continue;

      // len
      uint16_t valueLength = (data[offset] << 8) | data[offset + 1];
      offset += 2;

      if (offset + valueLength > data.size()) continue;

      // val
      String value(data.begin() + offset, data.begin() + offset + valueLength);
      offset += valueLength;

      message.options[key] = value;
    }

    if (offset + 2 > data.size()) return message;

    // len
    uint16_t bodyLength = (data[offset] << 8) | data[offset + 1];
    offset += 2;

    if (offset + bodyLength > data.size()) return message;

    // body
    message.payload = Vector<uint8_t>(data.begin() + offset, data.begin() + offset + bodyLength);

    return message;
  }

  bool CoreConduit::has (uint64_t id) {
    Lock lock(this->mutex);
    return this->clients.find(id) != this->clients.end();
  }

  CoreConduit::Client::~Client () {
    auto handle = reinterpret_cast<uv_handle_t*>(&this->handle);

    if (
      this->isClosing == false &&
      this->isClosed == false &&
      handle->loop != nullptr &&
      !uv_is_closing(handle) &&
      uv_is_active(handle)
    ) {
      // XXX(@jwerle): figure out a gracefull close
    }
  }

  CoreConduit::Client* CoreConduit::get (uint64_t id) {
    Lock lock(this->mutex);
    const auto it = clients.find(id);

    if (it != clients.end()) {
      return it->second;
    }

    return nullptr;
  }

  void CoreConduit::handshake (
    CoreConduit::Client* client,
    const char* buffer
  ) {
    const auto request = String(buffer);
    const auto crlf = request.find("\r\n");

    if (crlf == String::npos) {
      // TODO(@jwerle); handle malformed handshake request
      return;
    }

    String method;
    String uri;
    String version;

    auto inputStream = std::istringstream(request.substr(0, crlf));
    inputStream
      >> method
      >> uri
      >> version;

    const auto url = URL(uri);
    const auto headers = Headers(request);
    const auto webSocketKey = headers.get("Sec-WebSocket-Key");
    const auto sharedKey = url.searchParams.get("key");

    if (webSocketKey.empty()) {
      // debug("Sec-WebSocket-Key is required but missing.");
      return;
    }

    if (url.pathComponents.size() >= 2) {
      try {
        client->id = url.pathComponents.get<int>(0);
      } catch (...) {
        // debug("Unable to parse socket id");
      }

      try {
        client->clientId = url.pathComponents.get<int>(1);
      } catch (...) {
        // debug("Unable to parse client id");
      }
    }

    do {
      Lock lock(this->mutex);
      if (this->clients.contains(client->id)) {
        auto existingClient = this->clients.at(client->id);
        this->clients.erase(client->id);
        existingClient->close([existingClient]() {
          if (existingClient->isClosed) {
            delete existingClient;
          }
        });
      }

      this->clients.emplace(client->id, client);

      // std::cout << "added client " << this->clients.size() << std::endl;
    } while (0);

    if (sharedKey != this->sharedKey) {
      debug("CoreConduit::Client failed auth");
      client->close();
      return;
    }

    // debug("Received key: %s", webSocketKey.c_str());

    const auto acceptKey = webSocketKey + WS_GUID;
    const auto acceptKeyHash = shacalc(acceptKey);
    const auto encodedAcceptKeyHash = encodeBase64(acceptKeyHash);

    // debug("Generated Accept Key: %s\n", base64_accept_key);  // Debugging statement

    StringStream oss;
    oss
      << "HTTP/1.1 101 Switching Protocols\r\n"
      << "Upgrade: websocket\r\n"
      << "Connection: Upgrade\r\n"
      << "Sec-WebSocket-Accept: " << encodedAcceptKeyHash.data() << "\r\n\r\n";

    const auto response = oss.str();
    const auto size = response.size();
    const auto data = new char[size]{0};
    memcpy(data, response.c_str(), size);

    const auto buf = uv_buf_init(data, size);

    auto req = new uv_write_t;
    auto handle = reinterpret_cast<uv_handle_t*>(req);
    auto stream = reinterpret_cast<uv_stream_t*>(&client->handle);

    uv_handle_set_data(handle, data);
    uv_write(req, stream, &buf, 1, [](uv_write_t *req, int status) {
      const auto data = reinterpret_cast<char*>(
        uv_handle_get_data(reinterpret_cast<uv_handle_t*>(req))
      );

      if (data != nullptr) {
        delete [] data;
      }

      delete req;
    });

    client->isHandshakeDone = 1;
  }

  void CoreConduit::processFrame (
    Client* client,
    const char* frame,
    ssize_t len
  ) {
    if (len < 2) return; // Frame too short to be valid

    unsigned char *data = (unsigned char *)frame;
    int fin = data[0] & 0x80;
    int opcode = data[0] & 0x0F;
    int mask = data[1] & 0x80;
    uint64_t payloadSize = data[1] & 0x7F;
    size_t pos = 2;

    if (opcode == 0x08) {
      client->close();
      return;
    }

    if (payloadSize == 126) {
      if (len < 4) return; // too short to be valid
      payloadSize = (data[2] << 8) | data[3];
      pos = 4;
    } else if (payloadSize == 127) {
      if (len < 10) return; // too short to be valid
      payloadSize = 0;
      for (int i = 0; i < 8; i++) {
        payloadSize = (payloadSize << 8) | data[2 + i];
      }
      pos = 10;
    }

    if (!mask) return;
    if (len < pos + 4 + payloadSize) return; // too short to be valid

    unsigned char maskingKey[4];
    memcpy(maskingKey, data + pos, 4);
    pos += 4;

    if (client->frameBuffer.size() == 0) {
      client->frameBuffer.resize(payloadSize + (2 * 1024 * 1024));
    }

    // resize client frame buffer if payload size is too big to fit
    if (payloadSize + client->frameBufferOffset > client->frameBuffer.size()) {
      client->frameBuffer.resize(payloadSize + client->frameBufferOffset);
    }

    for (uint64_t i = 0; i < payloadSize; ++i) {
      client->frameBuffer[client->frameBufferOffset + i] = data[pos + i] ^ maskingKey[i % 4];
    }

    auto decoded = this->decodeMessage(client->frameBuffer.slice<uint8_t>(
      client->frameBufferOffset,
      client->frameBufferOffset + payloadSize
    ));

    pos += payloadSize;
    client->frameBufferOffset += payloadSize;

    client->queue.push_back(decoded);

    if (!decoded.has("route")) {
      if (decoded.has("to")) {
        try {
          const auto from = client->id;
          const auto to = std::stoull(decoded.get("to"));
          if (to != from) {
            const auto app = App::sharedApplication();
            const auto options = decoded.options;
            size_t size = 0;
            size_t offset = 0;
            Vector<uint8_t> buffer;
            for (const auto& entry : client->queue) {
              size += entry.payload.size();
            }
            buffer.resize(size);
            for (const auto& entry : client->queue) {
              for (int i = 0; i < entry.payload.size(); ++i) {
                buffer[offset + i] = entry.payload[i];
              }

              offset += entry.payload.size();
            }

            const auto bytes = vectorToSharedPointer(buffer);
            const auto payload = std::make_shared<char[]>(size);

            memcpy(payload.get(), bytes.get(), size);
            app->dispatch([this, options, size, payload, from, to] () {
              Lock lock(this->mutex);
              auto recipient = this->clients[to];
              auto client = this->clients[from];
              if (client != nullptr && recipient != nullptr) {
                recipient->send(options, payload, size);
              }
            });
          }
        } catch (...) {
          debug("Invalid 'to' parameter in encoded message");
        }
      }

      // TODO(@jwerle,@heapwolf): handle this
      return;
    }

    /* const auto uri = URL::Builder()
      .setProtocol("ipc")
      .setHostname(decoded.pluck("route"))
      .setSearchParam("id", client->id)
      .setSearchParams(decoded.getOptionsAsMap())
      .build(); */
    std::stringstream ss;

    ss << "ipc://";
    ss << decoded.pluck("route");
    ss << "/?id=" << std::to_string(client->id);

    for (auto& option : decoded.getOptionsAsMap()) {
      auto key = option.first;
      auto value = option.second == "value" ? encodeURIComponent(option.second) : option.second;
      ss << "&" << key << "=" << value;
    }

    size_t size = 0;
    size_t offset = 0;
    Vector<uint8_t> buffer;
    for (const auto& entry : client->queue) {
      size += entry.payload.size();
    }
    buffer.resize(size);
    for (const auto& entry : client->queue) {
      for (int i = 0; i < entry.payload.size(); ++i) {
        buffer[offset + i] = entry.payload[i];
      }

      offset += entry.payload.size();
    }

    const auto bytes = vectorToSharedPointer(buffer);
    const auto app = App::sharedApplication();
    const auto uri = ss.str();

    auto window = client && client->clientId > 0
      ? app->windowManager.getWindowForClient({ .id = client->clientId })
      : nullptr;

    client->queue.clear();
    client->frameBufferOffset = 0;

    if (window != nullptr) {
      app->dispatch([window, uri, bytes, size]() {
        const auto invoked = window->bridge.router.invoke(
          uri,
          bytes,
          size
        );

        if (!invoked) {
          // TODO(@jwerle,@heapwolf): handle this
          // debug("there was a problem invoking the router %s", ss.str().c_str());
        }
      });
    } else {
      window = app->windowManager.getWindow(0);
      app->dispatch([window, uri, client, bytes, size]() {
        const auto invoked = window->bridge.router.invoke(
          uri,
          bytes,
          size,
          [client](const auto result) {
            auto token = result.token;
            if (result.post.body != nullptr && result.post.length > 0) {
              client->send({{"token", token}}, result.post.body, result.post.length);
            } else {
              const auto data = result.json().str();
              const auto size = data.size();
              const auto payload = std::make_shared<char[]>(size);
              memcpy(payload.get(), data.c_str(), size);
              client->send({{"token", token}}, payload, size);
            }
          }
        );
      });
    }
  }

  struct ClientWriteContext {
    CoreConduit::Client* client = nullptr;
    const Function<void()> callback = nullptr;
  };

  bool CoreConduit::Client::send (
    const CoreConduit::Options& options,
    SharedPointer<char[]> bytes,
    size_t length,
    int opcode,
    const Function<void()> callback
  ) {
    auto handle = reinterpret_cast<uv_handle_t*>(&this->handle);

    if (!this->conduit) {
      return false;
    }

    Vector<uint8_t> payload(bytes.get(), bytes.get() + length);
    Vector<uint8_t> encodedMessage;

    try {
      encodedMessage = this->conduit->encodeMessage(options, payload);
    } catch (const std::exception& e) {
      debug("CoreConduit::Client: Error - Failed to encode message payload: %s", e.what());
      return false;
    }

    this->conduit->core->dispatchEventLoop([this, opcode, callback, handle, encodedMessage = std::move(encodedMessage)]() mutable {
      size_t encodedLength = encodedMessage.size();
      Vector<unsigned char> frame;

      if (encodedLength <= 125) {
        frame.resize(2 + encodedLength);
        frame[1] = static_cast<unsigned char>(encodedLength);
      } else if (encodedLength <= 65535) {
        frame.resize(4 + encodedLength);
        frame[1] = 126;
        frame[2] = (encodedLength >> 8) & 0xFF;
        frame[3] = encodedLength & 0xFF;
      } else {
        frame.resize(10 + encodedLength);
        frame[1] = 127;
        for (int i = 0; i < 8; i++) {
          frame[9 - i] = (encodedLength >> (i * 8)) & 0xFF;
        }
      }

      frame[0] = 0x80 | opcode; // FIN and opcode 2 (binary)
      std::memcpy(frame.data() + frame.size() - encodedLength, encodedMessage.data(), encodedLength);

      auto req = new uv_write_t;
      auto data = reinterpret_cast<char*>(frame.data());
      auto size = frame.size();

      uv_buf_t buf = uv_buf_init(data, size);

      if (callback != nullptr) {
        uv_handle_set_data(
          reinterpret_cast<uv_handle_t*>(req),
          new ClientWriteContext { this, callback }
        );
      } else {
        uv_handle_set_data(
          reinterpret_cast<uv_handle_t*>(req),
          nullptr
        );
      }

      uv_write(
        req,
        reinterpret_cast<uv_stream_t*>(&this->handle),
        &buf,
        1,
        [](uv_write_t* req, int status) {
          const auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(req));
          const auto context = static_cast<ClientWriteContext*>(data);

          delete req;

          if (context != nullptr) {
            context->client->conduit->core->dispatchEventLoop([=]() mutable {
              context->callback();
              delete context;
            });
          }
        }
      );
    });

    return true;
  }

  struct ClientCloseContext {
    CoreConduit::Client* client = nullptr;
    CoreConduit::Client::CloseCallback callback = nullptr;
  };

  void CoreConduit::Client::close (const CloseCallback& callback) {
    auto handle = reinterpret_cast<uv_handle_t*>(&this->handle);

    if (this->isClosing || this->isClosed || !uv_is_active(handle)) {
      if (callback != nullptr) {
        this->conduit->core->dispatchEventLoop(callback);
      }
      return;
    }

    this->isClosing = true;

    do {
      Lock lock(this->conduit->mutex);
      if (this->conduit->clients.contains(this->id)) {
        this->conduit->clients.erase(this->id);
      }
    } while (0);

    if (handle->loop == nullptr || uv_is_closing(handle)) {
      this->isClosed = true;
      this->isClosing = false;

      if (uv_is_active(handle)) {
        uv_read_stop(reinterpret_cast<uv_stream_t*>(handle));
      }

      if (callback != nullptr) {
        this->conduit->core->dispatchEventLoop(callback);
      }
      return;
    }

    const auto closeHandle = [=, this]() {
      if (uv_is_closing(handle)) {
        this->isClosed = true;
        this->isClosing = false;

        if (callback != nullptr) {
          this->conduit->core->dispatchEventLoop(callback);
        }
        return;
      }

      auto shutdown = new uv_shutdown_t;
      uv_handle_set_data(
        reinterpret_cast<uv_handle_t*>(shutdown),
        new ClientCloseContext { this, callback }
      );

      if (uv_is_active(handle)) {
        uv_read_stop(reinterpret_cast<uv_stream_t*>(handle));
      }

      uv_shutdown(shutdown, reinterpret_cast<uv_stream_t*>(handle), [](uv_shutdown_t* shutdown, int status) {
        auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(shutdown));
        auto context = static_cast<ClientCloseContext*>(data);
        auto client = context->client;
        auto handle = reinterpret_cast<uv_handle_t*>(&client->handle);
        auto callback = context->callback;

        delete shutdown;

        uv_handle_set_data(
          reinterpret_cast<uv_handle_t*>(handle),
          context
        );

        uv_close(handle, [](uv_handle_t* handle) {
          auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(handle));
          auto context = static_cast<ClientCloseContext*>(data);
          auto client = context->client;
          auto conduit = client->conduit;
          auto callback = context->callback;

          client->isClosed = true;
          client->isClosing = false;

          delete context;

          if (callback != nullptr) {
            client->conduit->core->dispatchEventLoop(callback);
          }
        });
      });
    };

    this->send({}, vectorToSharedPointer({ 0x00 }), 1, 0x08, closeHandle);
  }

  void CoreConduit::start (const StartCallback& callback) {
    if (this->isActive() || this->isStarting) {
      if (callback != nullptr) {
        this->core->dispatchEventLoop(callback);
      }
      return;
    }

    auto loop = this->core->getEventLoop();

    this->isStarting = true;

    this->hostname = Env::get("SOCKET_RUNTIME_CONDUIT_HOSTNAME", this->hostname);
    auto port = this->port.load();

    if (Env::has("SOCKET_RUNTIME_CONDUIT_PORT")) {
      try {
        port = std::stoi(Env::get("SOCKET_RUNTIME_CONDUIT_PORT"));
      } catch (...) {}
    }

    uv_ip4_addr(this->hostname.c_str(), port, &this->addr);
    uv_tcp_init(loop, &this->socket);
    uv_tcp_bind(
      &this->socket,
      reinterpret_cast<const struct sockaddr*>(&this->addr),
      0
    );

    struct sockaddr_in sockname;
    int namelen = sizeof(sockname);
    uv_tcp_getsockname(
      &this->socket,
      reinterpret_cast<struct sockaddr *>(&sockname),
      reinterpret_cast<int*>(&namelen)
    );

    uv_handle_set_data(
      reinterpret_cast<uv_handle_t*>(&this->socket),
      this
    );

    this->port = ntohs(sockname.sin_port);
    const auto result = uv_listen(reinterpret_cast<uv_stream_t*>(&this->socket), 128, [](uv_stream_t* stream, int status) {
      if (status < 0) {
        // debug("New connection error %s\n", uv_strerror(status));
        return;
      }

      auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(stream));
      auto conduit = static_cast<CoreConduit*>(data);
      auto client = new CoreConduit::Client(conduit);
      auto loop = uv_handle_get_loop(reinterpret_cast<uv_handle_t*>(stream));

      uv_tcp_init(
        loop,
        &client->handle
      );

      const auto accepted = uv_accept(
        stream,
        reinterpret_cast<uv_stream_t*>(&client->handle)
      );

      if (accepted != 0) {
        return uv_close(
          reinterpret_cast<uv_handle_t *>(&client->handle),
          nullptr
        );
      }

      uv_handle_set_data(
        reinterpret_cast<uv_handle_t*>(&client->handle),
        client
      );

      uv_read_start(
        reinterpret_cast<uv_stream_t*>(&client->handle),
        [](uv_handle_t* handle, size_t size, uv_buf_t* buf) {
          buf->base = new char[size]{0};
          buf->len = size;
        },
        [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
          auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(stream));
          auto client = static_cast<CoreConduit::Client*>(data);

          if (client && !client->isClosing && !client->isClosed && nread > 0) {
            if (client->isHandshakeDone) {
              do {
                Lock lock(client->conduit->mutex);
                if (!client->conduit->clients.contains(client->id)) {
                  if (buf->base) {
                    delete [] buf->base;
                  }
                  client->close([client]() {
                    if (client->isClosed) {
                      delete client;
                    }
                  });
                  return;
                }
              } while (0);

              client->conduit->processFrame(client, buf->base, nread);
            } else {
              client->conduit->handshake(client, buf->base);
            }
          } else if (nread < 0) {
            if (nread != UV_EOF) {
              // debug("Read error %s\n", uv_err_name(nread));
            }

            if (!client->isClosing && !client->isClosed) {
              client->close([client]() {
                if (client->isClosed) {
                  delete client;
                }
              });
            }
          }

          if (buf->base) {
            delete [] buf->base;
          }
        }
      );
    });

    if (result) {
      debug("CoreConduit: Listen error %s\n", uv_strerror(result));
    }

    this->isStarting = false;

    if (callback != nullptr) {
      this->core->dispatchEventLoop(callback);
    }
  }

  void CoreConduit::stop () {
    if (!this->isActive()) {
      return;
    }

    this->core->dispatchEventLoop([this]() {
      Lock lock(this->mutex);
      auto handle = reinterpret_cast<uv_handle_t*>(&this->socket);
      const auto closeHandle = [=, this] () {
        if (!uv_is_closing(handle)) {
          auto shutdown = new uv_shutdown_t;
          uv_handle_set_data(
            reinterpret_cast<uv_handle_t*>(shutdown),
            this
          );

          uv_shutdown(
            shutdown,
            reinterpret_cast<uv_stream_t*>(&this->socket),
            [](uv_shutdown_t* shutdown, int status) {
              auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(shutdown));
              auto conduit = reinterpret_cast<CoreConduit*>(data);

              delete shutdown;

              conduit->core->dispatchEventLoop([=]() {
                uv_close(
                  reinterpret_cast<uv_handle_t*>(&conduit->socket),
                  nullptr
                );
              });
            }
          );
        }
      };

      if (this->clients.size() == 0) {
        if (!uv_is_closing(handle)) {
          closeHandle();
        }
      } else {
        for (const auto& entry : this->clients) {
          auto client = entry.second;

          client->close([=, this] () {
            Lock lock(this->mutex);

            for (auto& entry : this->clients) {
              if (entry.second && !entry.second->isClosed) {
                return;
              }
            }

            for (auto& entry : this->clients) {
              delete entry.second;
            }

            this->clients.clear();
            closeHandle();
          });
        }
      }
    });
  }

  bool CoreConduit::isActive () {
    Lock lock(this->mutex);
    return (
      this->port > 0 &&
      uv_is_active(reinterpret_cast<uv_handle_t*>(&this->socket)) &&
      !uv_is_closing(reinterpret_cast<uv_handle_t*>(&this->socket))
    );
  }
}
