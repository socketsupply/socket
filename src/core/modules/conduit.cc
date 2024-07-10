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

    std::vector<std::pair<String, String>> sortedOptions(options.begin(), options.end());
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

  CoreConduit::EncodedMessage CoreConduit::decodeMessage (std::vector<uint8_t>& data) {
    CoreConduit::EncodedMessage message;

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

  bool CoreConduit::has (uint64_t id) const {
    return this->clients.find(id) != this->clients.end();
  }

  CoreConduit::Client* CoreConduit::get (uint64_t id) const {
    auto it = clients.find(id);

    if (it != clients.end()) {
      return it->second;
    }
    return nullptr;
  }

  void CoreConduit::handshake (CoreConduit::Client *client, const char *request) {
    String req(request);

    size_t reqeol = req.find("\r\n");
    if (reqeol == String::npos) return; // nope

    std::istringstream iss(req.substr(0, reqeol));
    String method;
    String url;
    String version;

    iss >> method >> url >> version;

    URL parsed(url);
    Headers headers(request);
    auto keyHeader = headers["Sec-WebSocket-Key"];

    if (keyHeader.empty()) {
      // debug("Sec-WebSocket-Key is required but missing.");
      return;
    }

    auto parts = split(parsed.pathname, "/");
    uint64_t socketId = 0;
    uint64_t clientId = 0;

    try {
      socketId = std::stoull(trim(parts[1]));
    } catch (...) {
      // debug("Unable to parse socket id");
    }

    try {
      clientId = std::stoull(trim(parts[2]));
    } catch (...) {
      // debug("Unable to parse client id");
    }

    client->id = socketId;
    client->clientId = clientId;

    this->clients.emplace(socketId, client);

    std::cout << "added client " << this->clients.size() << std::endl;

    // debug("Received key: %s", keyHeader.c_str());

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

  void CoreConduit::processFrame (CoreConduit::Client *client, const char *frame, ssize_t len) {
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

    /* const auto uri = URL::Builder()
      .setProtocol("ipc")
      .setHostname(decoded.pluck("route"))
      .setSearchParam("id", client->id)
      .setSearchParams(decoded.getOptionsAsMap())
      .build(); */

    auto app = App::sharedApplication();
    auto window = app->windowManager.getWindowForClient({ .id = client->clientId });

    std::stringstream ss;

    ss << "ipc://";
    ss << decoded.pluck("route");
    ss << "/?id=" << std::to_string(client->id);

    for (auto& option : decoded.getOptionsAsMap()) {
      auto key = option.first;
      auto value = option.second == "value" ? encodeURIComponent(option.second) : option.second;
      ss << "&" << key << "=" << value;
    }

    const auto invoked = window->bridge.router.invoke(ss.str(), vectorToShared(decoded.payload), decoded.payload.size());
    if (!invoked) {
      // debug("there was a problem invoking the router %s", ss.str().c_str());
    }
  }

  bool SSC::CoreConduit::Client::emit(const CoreConduit::Options& options, std::shared_ptr<char[]> payload, size_t length) {
    if (!this->self) {
      std::cout << "Error: 'self' is a null pointer." << std::endl;
      return false;
    }

    Vector<uint8_t> payloadVec(payload.get(), payload.get() + length);
    Vector<uint8_t> encodedMessage;

    try {
      encodedMessage = this->self->encodeMessage(options, payloadVec);
    } catch (const std::exception& e) {
      std::cerr << "Error in encodeMessage: " << e.what() << std::endl;
      return false;
    }

    this->self->core->dispatchEventLoop([this, encodedMessage = std::move(encodedMessage)]() mutable {
      size_t encodedLength = encodedMessage.size();
      std::vector<unsigned char> frame;

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

      frame[0] = 0x82; // FIN and opcode 2 (binary)
      std::memcpy(frame.data() + frame.size() - encodedLength, encodedMessage.data(), encodedLength);

      uv_buf_t wrbuf = uv_buf_init(reinterpret_cast<char*>(frame.data()), frame.size());
      auto writeReq = new uv_write_t;
      writeReq->bufs = &wrbuf;
      writeReq->data = new std::vector<unsigned char>(std::move(frame));

      uv_write(writeReq, (uv_stream_t*)&this->handle, &wrbuf, 1, [](uv_write_t* req, int status) {
        if (status) {
          // debug("Write error: %s", uv_strerror(status));
        }
        delete req;
      });
    });

    return true;
  }

  void CoreConduit::open () {
    if (this->port != 0) {
      return;
    }

    auto loop = this->core->getEventLoop();

    uv_tcp_init(loop, &this->conduitSocket);
    uv_ip4_addr("0.0.0.0", 0, &addr);
    uv_tcp_bind(&this->conduitSocket, (const struct sockaddr *)&this->addr, 0);

    this->conduitSocket.data = (void*)this;
    struct sockaddr_in sockname;
    int namelen = sizeof(sockname);
    uv_tcp_getsockname(&this->conduitSocket, (struct sockaddr *)&sockname, &namelen);

    this->port = ntohs(sockname.sin_port);

    this->core->dispatchEventLoop([=, this]() mutable {
      int r = uv_listen((uv_stream_t*)&this->conduitSocket, 128, [](uv_stream_t *stream, int status) {
        if (status < 0) {
          // debug("New connection error %s\n", uv_strerror(status));
          return;
        }

        auto self = static_cast<CoreConduit*>(stream->data);
        auto client = new CoreConduit::Client(self);

        client->is_handshake_done = 0;
        client->frame_buffer = nullptr;
        client->frame_buffer_size = 0;
        client->handle.data = client;

        uv_loop_t *loop = uv_handle_get_loop((uv_handle_t*)stream);
        uv_tcp_init(loop, &client->handle);

        auto accepted = uv_accept(stream, (uv_stream_t*)&client->handle);

        if (accepted == 0) {
          uv_read_start(
            (uv_stream_t *)&client->handle,
            [](uv_handle_t *handle, size_t size, uv_buf_t *buf) {
              buf->base = (char*) new char[size]{0};
              buf->len = size;
            },
            [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
              if (nread > 0) {
                auto handle = uv_handle_get_data((uv_handle_t*)stream);
                auto client = (CoreConduit::Client*)(handle);

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
        } else {
          // debug("uv_accept error: %s\n", uv_strerror(accepted));
          // delete static_cast<std::shared_ptr<CoreConduit::Client>*>(client->handle.data);
        }

        uv_close((uv_handle_t *)&client->handle, nullptr);
      });

      if (r) {
        this->port = -1;
        // debug("Listen error %s\n", uv_strerror(r));
      }
    });
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
