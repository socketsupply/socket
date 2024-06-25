#include "conduit.hh"
#include "core.hh"

namespace SSC {
  //
  // ~~1. where to create this instance~~
  // ~~2. how to listen to the stream (listeners)~~
  // 3. how to write data to the stream (emit)
  //
  const char *WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  CoreConduit::~CoreCoreConduit() {
    this->close();
  }

  void CoreConduit::handshake(CoreConduit::Client *client, const char *request) {
    Headers headers(request);
    auto protocolHeader = headers["Sec-WebSocket-Protocol"];
    auto keyHeader = headers["Sec-WebSocket-Key"];

    if (protocolHeader.empty() || keyHeader.empty()) {
      // Handle missing headers appropriately, e.g., close connection or send error
      return;
    }

    uint64_t id = std::stoll(protocolHeader);
    client->id = id;
    this->clients[id] = client;

    debug("Received key: %s\n", keyHeader.c_str());

    std::string acceptKey = keyHeader + WS_GUID;
    char calculatedHash[SHA_DIGEST_LENGTH];
    shacalc(acceptKey.c_str(), calculatedHash);

    size_t base64_len;
    unsigned char *base64_accept_key = base64Encode((unsigned char*)calculatedHash, SHA_DIGEST_LENGTH, &base64_len);

    debug("Generated Accept Key: %s\n", base64_accept_key);  // Debugging statement

    std::ostringstream ss;

    ss << "HTTP/1.1 101 Switching Protocols\r\n"
       << "Upgrade: websocket\r\n"
       << "Connection: Upgrade\r\n"
       << "Sec-WebSocket-Accept: " << base64_accept_key << "\r\n\r\n";

    std::string response = ss.str();
    debug(response.c_str());

    uv_buf_t wrbuf = uv_buf_init(strdup(response.c_str()), response.size());
    uv_write_t *write_req = new uv_write_t;
    write_req->bufs = &wrbuf;

    uv_write(write_req, (uv_stream_t*)&client->handle, &wrbuf, 1, [](uv_write_t *req, int status) {
      if (status) debug(stderr, "write error %s\n", uv_strerror(status));

      if (req->bufs != nullptr) {
        free(req->bufs->base);
      }

      free(req);
    });

    free(base64_accept_key);

    client->is_handshake_done = 1;
  }

  void CoreConduit::processFrame(CoreConduit::Client *client, const char *frame, ssize_t len) {
    if (len < 2) return; // Frame too short to be valid

    unsigned char *data = (unsigned char *)frame;
    int fin = data[0] & 0x80;
    int opcode = data[0] & 0x0F;
    int mask = data[1] & 0x80;
    uint64_t payload_len = data[1] & 0x7F;
    size_t pos = 2;

    if (payload_len == 126) {
      if (len < 4) return; // too short to be valid
      payload_len = (data[2] << 8) | data[3];
      pos = 4;
    } else if (payload_len == 127) {
      if (len < 10) return; // too short to be valid
      payload_len = 0;
      for (int i = 0; i < 8; i++) {
        payload_len = (payload_len << 8) | data[2 + i];
      }
      pos = 10;
    }

    if (!mask) return;
    if (len < pos + 4 + payload_len) return; // too short to be valid

    unsigned char masking_key[4];
    memcpy(masking_key, data + pos, 4);
    pos += 4;

    if (payload_len > client->frame_buffer_size) {
      client->frame_buffer = (unsigned char *)realloc(client->frame_buffer, payload_len);
      client->frame_buffer_size = payload_len;
    }

    unsigned char *payload_data = client->frame_buffer;

    for (uint64_t i = 0; i < payload_len; i++) {
      payload_data[i] = data[pos + i] ^ masking_key[i % 4];
    }

    pos += payload_len;

    debug("Received message: %.*s\n", (int)payload_len, payload_data);

    auto cb = this->listeners[client->id];
    if (cb) cb((int)payload_len, payload_data);
  }

  bool CoreConduit::Client::emit(std::shared_ptr<char[]> message, size_t length) {
    this->conduit->core->dispatchEventLoop([this, message, length]() mutable {
      size_t frame_size = 2 + length;
      std::vector<unsigned char> frame(frame_size);
      frame[0] = 0x81; // FIN and opcode 1 (text)
      frame[1] = length;
      memcpy(frame.data() + 2, message, length);

      uv_buf_t wrbuf = uv_buf_init(reinterpret_cast<char*>(frame.data()), frame_size);
      uv_write_t *write_req = new uv_write_t;
      write_req->bufs = &wrbuf;
      write_req->data = new std::vector<unsigned char>(std::move(frame));

      uv_write(write_req, (uv_stream_t *)&client->handle, &wrbuf, 1, [](uv_write_t *req, int status) {
        if (status) debug("Write error %s\n", uv_strerror(status));
        delete static_cast<std::vector<unsigned char>*>(req->data);
        delete req;
      });
    });

    return true;
  }

  void CoreConduit::addListener(uint64_t key, Callback callback) {
    listeners[key] = callback;
  }

  void CoreConduit::removeListener(uint64_t key) {
    listeners.erase(key);
  }

  void CoreConduit::open() {
    std::promise<int> p;
    std::future<int> future = p.get_future();

    this->core->dispatchEventLoop([this, p]() mutable {
      auto loop = this->core->getEventLoop();

      uv_tcp_init(loop, &this->conduitSocket);
      uv_ip4_addr("0.0.0.0", 0, &addr);
      uv_tcp_bind(&this->conduitSocket, (const struct sockaddr *)&this->addr, 0);

      this->conduitSocket.data = this;

      int r = uv_listen((uv_stream_t*)&this->conduitSocket, 128, [](uv_stream_t *stream, int status) {
        if (status < 0) {
          debug("New connection error %s\n", uv_strerror(status));
          return;
        }

        auto client = new CoreConduit::Client();
        client->self = static_cast<CoreConduit*>(stream->data);
        client->is_handshake_done = 0;
        client->frame_buffer = nullptr;
        client->frame_buffer_size = 0;
        client->handle.data = client;

        uv_tcp_init(uv_default_loop(), &client->handle);

        if (uv_accept(stream, (uv_stream_t*)&client->handle) == 0) {
          uv_read_start(
            (uv_stream_t *)&client->handle,
            [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
              buf->base = (char *) new char[size]{0};
              buf->len = size;
            },
            [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
              auto client = static_cast<CoreConduit::Client*>(stream->data);
              if (nread > 0) {
                if (!client->is_handshake_done) {
                  client->self->handshake(client, buf->base);
                } else {
                  client->self->processFrame(client, buf->base, nread);
                }
              } else if (nread < 0) {
                if (nread != UV_EOF) {
                  debug("Read error %s\n", uv_err_name(nread));
                }
                uv_close((uv_handle_t *)stream, nullptr);
              }

              if (buf->base) {
                delete buf->base;
              }
            }
          );
          return;
        }

        uv_close((uv_handle_t *)&client->handle, nullptr);
        delete client;
      });

      if (r) {
        debug("Listen error %s\n", uv_strerror(r));
        p.set_value(0);
        return;
      }

      struct sockaddr_in sockname;
      int namelen = sizeof(sockname);
      uv_tcp_getsockname(&this->conduitSocket, (struct sockaddr *)&sockname, &namelen);

      p.set_value(ntohs(sockname.sin_port));
    });

    this->port = future.get();
  }

  bool CoreConduit::close() {
    std::promise<bool> p;
    std::future<bool> future = p.get_future();

    this->core->dispatchEventLoop([this, p]() mutable {
      if (!uv_is_closing((uv_handle_t*)&this->conduitSocket)) {
        uv_close((uv_handle_t*)&this->conduitSocket, [](uv_handle_t* handle) {
          auto conduit = static_cast<CoreConduit*>(handle->data);
          p.set_value(true);
        });
      } else {
        p.set_value(true);
      }

      for (auto& clientPair : this->clients) {
        auto client = clientPair.second;

        if (client && !uv_is_closing((uv_handle_t*)&client->handle)) {
          uv_close((uv_handle_t*)&client->handle, [](uv_handle_t* handle) {
            auto client = static_cast<CoreConduit::Client*>(handle->data);

            if (client->frame_buffer) {
              free(client->frame_buffer);
              client->frame_buffer = nullptr;
              client->frame_buffer_size = 0;
            }
            delete client;
          });
        }
      }

      this->clients.clear();
      this->listeners.clear();
    });

    return future.get();
  }
}
