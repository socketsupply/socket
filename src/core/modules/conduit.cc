#include "conduit.hh"
#include "../core.hh"
#include "../../app/app.hh"
#include "../codec.hh"

#define SHA_DIGEST_LENGTH 20

namespace SSC {
  const char *WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  std::shared_ptr<char[]> vectorToShared(const std::vector<uint8_t>& vec) {
    std::shared_ptr<char[]> sharedArray(new char[vec.size()]);
    std::memcpy(sharedArray.get(), vec.data(), vec.size());
    return sharedArray;
  }

  CoreConduit::~CoreConduit() {
    this->close();
  }

  Vector<uint8_t> CoreConduit::encodeMessage (const CoreConduit::Options& options, const Vector<uint8_t>& payload) {
    Vector<uint8_t> encodedMessage;

    encodedMessage.push_back(static_cast<uint8_t>(options.size()));

    for (const auto& option : options) {
      const String& key = option.first;
      const String& value = option.second;

      // length
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

    // len
    uint16_t bodyLength = static_cast<uint16_t>(payload.size());
    encodedMessage.push_back(static_cast<uint8_t>((bodyLength >> 8) & 0xFF));
    encodedMessage.push_back(static_cast<uint8_t>(bodyLength & 0xFF));

    // body
    encodedMessage.insert(encodedMessage.end(), payload.begin(), payload.end());

    return encodedMessage;
  }

  CoreConduit::EncodedMessage CoreConduit::decodeMessage (std::vector<uint8_t>& data) {
    CoreConduit::EncodedMessage message;
    size_t offset = 0;

    uint8_t numOpts = data[offset++];

    for (uint8_t i = 0; i < numOpts; ++i) {
      // len
      uint8_t keyLength = data[offset++];
      // key
      String key(data.begin() + offset, data.begin() + offset + keyLength);
      offset += keyLength;

      // len
      uint16_t valueLength = (data[offset] << 8) | data[offset + 1];
      offset += 2;

      // val
      String value(data.begin() + offset, data.begin() + offset + valueLength);
      offset += valueLength;

      message.options[key] = value;
    }

    // len
    uint16_t bodyLength = (data[offset] << 8) | data[offset + 1];
    offset += 2;

    // body
    message.payload = Vector<uint8_t>(data.begin() + offset, data.begin() + offset + bodyLength);

    return message;
  }

  bool CoreConduit::has (uint64_t id) const {
    // std::lock_guard<std::mutex> lock(clientsMutex);
    return this->clients.find(id) != this->clients.end();
  }

  std::shared_ptr<CoreConduit::Client> CoreConduit::get (uint64_t id) const {
    // std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = clients.find(id);

    if (it != clients.end()) {
      return it->second;
    }
    return nullptr;
  }

  void CoreConduit::handshake (std::shared_ptr<CoreConduit::Client> client, const char *request) {
    String req(request);

    size_t reqeol = req.find("\r\n");
    if (reqeol == String::npos) return; // nope

    std::istringstream iss(req.substr(0, reqeol));
    String method;
    String url;
    String version;

    iss >> method >> url >> version;

    URL parsed(url);
    if (parsed.searchParams.count("id") == 0) {
      return;
    }

    if (parsed.searchParams.count("clientId") == 0) {
      return;
    }

    Headers headers(request);
    auto keyHeader = headers["Sec-WebSocket-Key"];

    if (keyHeader.empty()) {
      // debug("Sec-WebSocket-Key is required but missing.");
      return;
    }

    auto id = std::stoll(parsed.searchParams["id"]);
    auto clientId = std::stoll(parsed.searchParams["clientId"]);

    client->id = id;
    client->clientId = clientId;

    this->clients[id] = client;

    // debug("Received key: %s\n", keyHeader.c_str());

    String acceptKey = keyHeader + WS_GUID;
    char calculatedHash[SHA_DIGEST_LENGTH];
    shacalc(acceptKey.c_str(), calculatedHash);

    size_t base64_len;
    unsigned char *base64_accept_key = base64Encode((unsigned char*)calculatedHash, SHA_DIGEST_LENGTH, &base64_len);

    // debug("Generated Accept Key: %s\n", base64_accept_key);  // Debugging statement

    std::ostringstream oss;

    oss << "HTTP/1.1 101 Switching Protocols\r\n"
       << "Upgrade: websocket\r\n"
       << "Connection: Upgrade\r\n"
       << "Sec-WebSocket-Accept: " << base64_accept_key << "\r\n\r\n";

    String response = oss.str();
    // debug(response.c_str());

    uv_buf_t wrbuf = uv_buf_init(strdup(response.c_str()), response.size());
    uv_write_t *write_req = new uv_write_t;
    write_req->bufs = &wrbuf;

    uv_write(write_req, (uv_stream_t*)&client->handle, &wrbuf, 1, [](uv_write_t *req, int status) {
      // if (status) debug(stderr, "write error %s\n", uv_strerror(status));

      if (req->bufs != nullptr) {
        free(req->bufs->base);
      }

      free(req);
    });

    free(base64_accept_key);

    client->is_handshake_done = 1;
  }

  void CoreConduit::processFrame (std::shared_ptr<CoreConduit::Client> client, const char *frame, ssize_t len) {
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

    unsigned char *payload = client->frame_buffer;

    for (uint64_t i = 0; i < payload_len; i++) {
      payload[i] = data[pos + i] ^ masking_key[i % 4];
    }

    pos += payload_len;

    Vector<uint8_t> vec(payload, payload + payload_len);
    auto decoded = this->decodeMessage(vec);

    const auto uri = URL::Builder()
      .setProtocol("ipc")
      .setHostname(decoded.pluck("route"))
      .setSearchParam("id", client->id)
      .setSearchParams(decoded.getOptionsAsMap())
      .build();

    auto app = App::sharedApplication();
    auto window = app->windowManager.getWindowForClient({ .id = client->clientId });
  
    window->bridge.router.invoke(uri.str(), vectorToShared(decoded.payload), decoded.payload.size());
  }

  bool SSC::CoreConduit::Client::emit(const CoreConduit::Options& options, std::shared_ptr<char[]> payload, size_t length) {
    Vector<uint8_t> payloadVec(payload.get(), payload.get() + length);
    Vector<uint8_t> encodedMessage = this->self->encodeMessage(options, payloadVec);

    this->self->core->dispatchEventLoop([this, encodedMessage = std::move(encodedMessage)]() mutable {
      size_t encodedLength = encodedMessage.size();
      Vector<unsigned char> frame(2 + encodedLength);

      frame[0] = 0x81; // FIN and opcode 1 (text)
      frame[1] = static_cast<unsigned char>(encodedLength);
      std::memcpy(frame.data() + 2, encodedMessage.data(), encodedLength);

      uv_buf_t wrbuf = uv_buf_init(reinterpret_cast<char*>(frame.data()), frame.size());
      auto writeReq = new uv_write_t;
      writeReq->bufs = &wrbuf;
      writeReq->data = new Vector<unsigned char>(std::move(frame));

      uv_write(writeReq, (uv_stream_t *)&this->handle, &wrbuf, 1, [](uv_write_t *req, int status) {
        if (status) {
          std::cerr << "Write error: " << uv_strerror(status) << std::endl;
        }
        delete static_cast<Vector<unsigned char>*>(req->data);
        delete req;
      });
    });

    return true;
  }

  void CoreConduit::open () {
    std::promise<int> p;
    std::future<int> future = p.get_future();

    this->core->dispatchEventLoop([&, this]() mutable {
      auto loop = this->core->getEventLoop();

      uv_tcp_init(loop, &this->conduitSocket);
      uv_ip4_addr("0.0.0.0", 0, &addr);
      uv_tcp_bind(&this->conduitSocket, (const struct sockaddr *)&this->addr, 0);

      this->conduitSocket.data = this;

      int r = uv_listen((uv_stream_t*)&this->conduitSocket, 128, [](uv_stream_t *stream, int status) {
        if (status < 0) {
          // debug("New connection error %s\n", uv_strerror(status));
          return;
        }

        auto client = std::make_shared<CoreConduit::Client>();
        client->self = static_cast<CoreConduit*>(stream->data);
        client->is_handshake_done = 0;
        client->frame_buffer = nullptr;
        client->frame_buffer_size = 0;
        client->handle.data = new std::shared_ptr<CoreConduit::Client>(client);

        uv_tcp_init(uv_default_loop(), &client->handle);

        if (uv_accept(stream, (uv_stream_t*)&client->handle) == 0) {
          uv_read_start(
            (uv_stream_t *)&client->handle,
            [](uv_handle_t *handle, size_t size, uv_buf_t *buf) {
              buf->base = (char*) new char[size]{0};
              buf->len = size;
            },
            [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
              if (nread > 0) {
                auto handle = uv_handle_get_data((uv_handle_t*)stream);
                auto shared_client_ptr = static_cast<std::shared_ptr<CoreConduit::Client>*>(handle);
                auto client = *shared_client_ptr;

                if (!client->is_handshake_done) {
                  client->self->handshake(client, buf->base);
                } else {
                  client->self->processFrame(client, buf->base, nread);
                }
              } else if (nread < 0) {
                if (nread != UV_EOF) {
                  // debug("Read error %s\n", uv_err_name(nread));
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
      });

      if (r) {
        // debug("Listen error %s\n", uv_strerror(r));
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

  void CoreConduit::close () {
    this->core->dispatchEventLoop([this]() mutable {
      if (!uv_is_closing((uv_handle_t*)&this->conduitSocket)) {
        uv_close((uv_handle_t*)&this->conduitSocket, [](uv_handle_t* handle) {
          auto conduit = static_cast<CoreConduit*>(handle->data);
        });
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
    });
  }
}
