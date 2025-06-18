#include "../../runtime.hh"
#include "../../crypto.hh"
#include "../../config.hh"
#include "../../string.hh"
#include "../../http.hh"
#include "../../env.hh"
#include "../../ipc.hh"
#include "../../url.hh"

#include "conduit.hh"

using ssc::runtime::config::getUserConfig;
using ssc::runtime::string::toUpperCase;
using ssc::runtime::crypto::rand64;
using ssc::runtime::crypto::sha1;

namespace ssc::runtime::core::services {
  static constexpr char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  static SharedPointer<unsigned char[]> vectorToSharedPointer (const Vector<uint8_t>& vector) {
    const auto size = vector.size();
    const auto data = vector.data();
    const auto pointer = std::make_shared<unsigned char[]>(size);
    memcpy(pointer.get(), data, size);
    return std::move(pointer);
  }

  inline String Conduit::Message::get (const String& key) const {
    const auto it = options.find(key);
    if (it != options.end()) {
      return it->second;
    }
    return "";
  }

  inline bool Conduit::Message::has (const String& key) const {
    const auto it = options.find(key);
    if (it != options.end()) {
      return true;
    }
    return false;
  }

  inline String Conduit::Message::pluck (const String& key) {
    auto it = options.find(key);
    if (it != options.end()) {
      String value = it->second;
      options.erase(it);
      return value;
    }
    return "";
  }

  inline Map<String, String> Conduit::Message::map () const {
    Map<String, String> map;
    for (const auto& entry : this->options) {
      map.insert(entry);
    }
    return map;
  }

  const inline bool Conduit::Message::empty () const {
    return this->options.empty() && this->payload.size() == 0;
  }

  void Conduit::Message::clear () {
    this->options.clear();
    this->payload.clear();
  }

  inline const size_t Conduit::FrameBuffer::size () const {
    return this->vector.size();
  }

  inline const unsigned char* Conduit::FrameBuffer::data () const {
    return this->vector.data();
  }

  inline void Conduit::FrameBuffer::resize (const size_t size) {
    this->vector.resize(size);
  }

  unsigned char Conduit::FrameBuffer::operator [] (const unsigned int index) const {
    if (index >= this->size()) {
      return 0;
    }

    return this->vector.at(index);
  }

  unsigned char& Conduit::FrameBuffer::operator [] (const unsigned int index) {
    if (index >= this->size()) {
      this->vector.resize(index + 1);
    }

    return this->vector.at(index);
  }

  const Vector<uint8_t> Conduit::FrameBuffer::slice (
    Vector<uint8_t>::const_iterator& begin,
    Vector<uint8_t>::const_iterator& end
  ) {
    return Vector<uint8_t>(begin, end);
  }

  template <typename T>
  const Vector<T> Conduit::FrameBuffer::slice (
    Vector<uint8_t>::size_type start,
    Vector<uint8_t>::size_type end
  ) {
    return Vector<T>(this->vector.begin() + start, this->vector.begin() + end);
  }

  Conduit::Conduit (const Service::Options& options)
    : core::Service(options)
  {
    static const auto userConfig = getUserConfig();
    static const auto sharedKey = runtime::env::get(
      "SOCKET_RUNTIME_CONDUIT_SHARED_KEY",
      userConfig.contains("application_conduit_shared_key")
        ? userConfig.at("application_conduit_shared_key")
        : ""
    );

    if (sharedKey.size() >= 8) {
      this->sharedKey = sharedKey;
    } else {
      this->sharedKey = std::to_string(rand64());
    }
  }

  Conduit::~Conduit () noexcept {
    this->stop();
  }

  Vector<uint8_t> Conduit::encodeMessage (
    const Conduit::Message::Options& options,
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

  Conduit::Message Conduit::decodeMessage (const Vector<uint8_t>& data) {
    Message message;

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

  bool Conduit::has (uint64_t id) {
    Lock lock(this->mutex);
    return this->clients.find(id) != this->clients.end();
  }

  Conduit::Client::~Client () {
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

  Conduit::Client* Conduit::get (uint64_t id) {
    Lock lock(this->mutex);
    const auto it = clients.find(id);

    if (it != clients.end()) {
      return it->second;
    }

    return nullptr;
  }

  void Conduit::handshake (
    Conduit::Client* client,
    const char* buffer
  ) {
    auto request = http::Request(buffer);
    request.url.hostname = "127.0.0.1";
    request.url.scheme = "ws";
    request.url.port = std::to_string(this->port.load());
    request.scheme = "ws";

    if (!request.valid()) {
      client->close();
      return;
    }

    const auto webSocketKey = request.headers.get("sec-websocket-key");

    if (webSocketKey.empty()) {
      // debug("Sec-WebSocket-Key is required but missing.");
      const auto buffer = bytes::Buffer(http::Response(400).str());
      client->write(buffer, [client]() {
        client->close();
      });
      return;
    }

    const auto sharedKey = request.url.searchParams.get("key").str();
    if (sharedKey != this->sharedKey) {
      // debug("Conduit::Client failed auth");
      const auto buffer = bytes::Buffer(http::Response(403).str());
      client->write(buffer, [client]() {
        client->close();
      });
      return;
    }

    if (request.url.pathComponents.size() >= 2) {
      try {
        client->id = request.url.pathComponents.get<uint64_t>(0);
      } catch (...) {
        debug("Unable to parse socket id");
      }

      try {
        client->client.id = request.url.pathComponents.get<uint64_t>(1);
      } catch (...) {
        debug("Unable to parse client id");
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

      // debug("added client: %lu", client->id);
      this->clients.emplace(client->id, client);

      // std::cout << "added client " << this->clients.size() << std::endl;
    } while (0);

    const auto response = http::Response(101)
      .setHeader("upgrade", "websocket")
      .setHeader("connection", "upgrade")
      .setHeader(
        "sec-websocket-accept",
        bytes::base64::encode(crypto::SHA1(webSocketKey + WS_GUID).finalize())
      );

    client->write(response.str());
    client->isHandshakeDone = true;
  }

  void Conduit::processFrame (
    Client* client,
    const char* frame,
    ssize_t len
  ) {
    Lock lock(client->mutex);
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

    // resize client frame buffer if payload size is too big to fit
    if (payloadSize > client->frameBuffer.size()) {
      client->frameBuffer.resize(payloadSize + 1);
    }

    for (uint64_t i = 0; i < payloadSize; ++i) {
      client->frameBuffer[i] = data[pos + i] ^ maskingKey[i % 4];
    }

    auto decoded = this->decodeMessage(client->frameBuffer.slice<uint8_t>(
      0,
      payloadSize
    ));

    client->queue.push_back(decoded);
    if (decoded.has("digest")) {
      const auto bytes = vectorToSharedPointer(decoded.payload);
      const auto inputDigest = toUpperCase(decoded.get("digest"));
      const auto computedDigest = toUpperCase(sha1(bytes, decoded.payload.size()));
      client->send({{"digest", computedDigest}}, nullptr, 0);
      return;
    }

    if (!decoded.has("route")) {
      if (decoded.has("to")) {
        try {
          const auto from = client->id;
          const auto to = std::stoull(decoded.get("to"));
          if (to != from) {
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
            const auto payload = std::make_shared<unsigned char[]>(size);

            memcpy(payload.get(), bytes.get(), size);

            this->dispatch([this, options, size, payload, from, to] () {
              Lock lock(this->mutex);
              auto recipient = this->clients[to];
              auto client = this->clients[from];
              if (client != nullptr && recipient != nullptr) {
                Lock lock(client->mutex);
                do {
                  Lock lock(recipient->mutex);
                  debug(">>>>>>>>>>>>>>>>>>>>> PROCESSED FRAME (PAYLOAD)");
                  recipient->send(options, payload, size);
                } while (0);
              }
            });
          }
        } catch (...) {
        }
      }

      // TODO(@jwerle,@heapwolf): handle this
      return;
    }

    const auto uri = URL::Builder()
      .setScheme("ipc")
      .setHostname(decoded.pluck("route"))
      .setSearchParam("id", std::to_string(client->id))
      .setSearchParams(decoded.map())
      .build()
      .str();

    size_t offset = 0;
    Vector<uint8_t> buffer;

    size_t size = 0;
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

    client->queue.clear();
    client->frameBuffer.resize(0);

    const auto bytes = vectorToSharedPointer(buffer);
    const auto message = ipc::Message(uri);

    auto window = client && client->client.id > 0
      ? this->context.getRuntime()->windowManager.getWindowForClient({ client->client.id })
      : nullptr;

    // prevent external usage of internal routes
    if (message.name.starts_with("internal.")) {
      const auto result = ipc::Result(ipc::Result::Err {
        message,
        JSON::Object::Entries {
          {"type", "NotFoundError"},
          {"message", "Not found"}
        }
      });
      const auto data = result.json().str();
      const auto size = data.size();
      const auto payload = std::make_shared<unsigned char[]>(size);
      memcpy(payload.get(), data.c_str(), size);
      client->send({}, payload, size);
      return;
    }

    debug(">>> FRAME WITH URI %s", uri.c_str());

    bool invoked = false;
    if (window != nullptr) {
      invoked = window->bridge->router.invoke(uri, bytes, size);
    } else {
      window = this->context.getRuntime()->windowManager.getWindow(0);
      invoked = window->bridge->router.invoke(
        uri,
        bytes,
        size,
        [client](const auto result) {
          auto token = result.token;
          if (result.queuedResponse.body != nullptr && result.queuedResponse.length > 0) {
            client->send({{"token", token}}, result.queuedResponse.body, result.queuedResponse.length);
          } else {
            const auto data = result.json().str();
            const auto size = data.size();
            const auto payload = std::make_shared<unsigned char[]>(size);
            memcpy(payload.get(), data.c_str(), size);
            client->send({{"token", token}}, payload, size);
          }
        }
      );
    }

    if (!invoked) {
      const auto result = ipc::Result(ipc::Result::Err {
        message,
        JSON::Object::Entries {
          {"type", "NotFoundError"},
          {"message", "Not found"}
        }
      });
      const auto data = result.json().str();
      const auto size = data.size();
      const auto payload = std::make_shared<unsigned char[]>(size);
      memcpy(payload.get(), data.c_str(), size);
      client->send({}, payload, size);
      return;
    }
  }

  struct ClientWriteContext {
    Conduit::Client* client = nullptr;
    const Function<void()> callback = nullptr;
  };

  bool Conduit::Client::send (
    const Conduit::Message::Options& options,
    SharedPointer<unsigned char[]> bytes,
    size_t length,
    int opcode,
    const Function<void()> callback
  ) {
    auto handle = reinterpret_cast<uv_handle_t*>(&this->handle);

    if (!this->conduit) {
      return false;
    }

    debug(">>> CLIENT SEND BYTES FROM UDP TO WEBSOCKET");

    Vector<uint8_t> payload(bytes.get(), bytes.get() + length);
    Vector<uint8_t> encodedMessage;

    try {
      encodedMessage = this->conduit->encodeMessage(options, payload);
    } catch (const std::exception& e) {
      debug("Conduit::Client: Error - Failed to encode message payload: %s", e.what());
      return false;
    }

    this->conduit->loop.dispatch([
      this,
      opcode,
      handle,
      callback = std::move(callback),
      encodedMessage = std::move(encodedMessage)
    ]() mutable {
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

          debug(">>> CLIENT DID SEND BYTES FROM UDP TO WEBSOCKET");
          delete req;

          if (context != nullptr) {
            context->client->conduit->loop.dispatch([=]() mutable {
              context->callback();
              delete context;
            });
          }
        }
      );
    });

    return true;
  }

  bool Conduit::Client::write (const bytes::Buffer& buffer, const WriteCallback callback) {
    auto buf = uv_buf_init(reinterpret_cast<char*>(const_cast<unsigned char*>(buffer.data())), buffer.size());
    auto req = new uv_write_t;
    auto handle = reinterpret_cast<uv_handle_t*>(req);
    auto stream = reinterpret_cast<uv_stream_t*>(&this->handle);

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

    uv_write(req, stream, &buf, 1, [](uv_write_t *req, int status) {
      const auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(req));
      const auto context = static_cast<ClientWriteContext*>(data);

      delete req;

      if (context != nullptr) {
        context->client->conduit->loop.dispatch([=]() mutable {
          context->callback();
          delete context;
        });
      }
    });

    return true;
  }

  struct ClientCloseContext {
    Conduit::Client* client = nullptr;
    Conduit::Client::CloseCallback callback = nullptr;
  };

  void Conduit::Client::close (const CloseCallback callback) {
    auto handle = reinterpret_cast<uv_handle_t*>(&this->handle);

    if (this->isClosing || this->isClosed || !uv_is_active(handle)) {
      if (callback != nullptr) {
        this->conduit->loop.dispatch(callback);
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
        this->conduit->loop.dispatch(callback);
      }
      return;
    }

    const auto closeHandle = [=, this]() {
      if (uv_is_closing(handle)) {
        this->isClosed = true;
        this->isClosing = false;

        if (callback != nullptr) {
          this->conduit->loop.dispatch(callback);
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
            client->conduit->loop.dispatch(callback);
          }
        });
      });
    };

    this->send({}, vectorToSharedPointer({ 0x00 }), 1, 0x08, closeHandle);
  }

  bool Conduit::start () {
    return this->start(nullptr);
  }

  bool Conduit::start (const StartCallback callback) {
    if (this->isActive()) {
      if (callback != nullptr) {
        this->loop.dispatch(callback);
      }
      return true;
    }

    bool expected = false;
    if (!this->isStarting.compare_exchange_strong(expected, true)) {
      if (callback != nullptr) {
        this->loop.dispatch(callback);
      }
      return true;
    }

    this->isStarting.store(true);

    this->hostname = runtime::env::get("SOCKET_RUNTIME_CONDUIT_HOSTNAME", this->hostname);

    auto port = this->port.load();

    if (runtime::env::has("SOCKET_RUNTIME_CONDUIT_PORT")) {
      try {
        port = std::stoi(runtime::env::get("SOCKET_RUNTIME_CONDUIT_PORT"));
      } catch (...) {}
    }

    uv_ip4_addr(this->hostname.c_str(), port, &this->addr);
    uv_tcp_init(loop.get(), &this->socket);
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

    debug(">>> STARTING CONDUIT (%i)", this->port.load());

    const auto result = uv_listen(reinterpret_cast<uv_stream_t*>(&this->socket), 128, [](uv_stream_t* stream, int status) {
      if (status < 0) {
        return;
      }

      auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(stream));
      auto conduit = static_cast<Conduit*>(data);
      auto client = new Conduit::Client(conduit);
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
          if (buf && size > 0) {
            buf->base = new char[size]{0};
            buf->len = size;
          } else if (buf) {
            buf->base = nullptr;
            buf->len = 0;
          }
        },
        [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
          auto data = uv_handle_get_data(reinterpret_cast<uv_handle_t*>(stream));
          auto client = static_cast<Conduit::Client*>(data);
          auto buffer = buf->base;

          if (client && !client->isClosing && !client->isClosed && nread > 0) {
            if (client->isHandshakeDone) {
              do {
                Lock lock(client->conduit->mutex);
                if (!client->conduit->clients.contains(client->id)) {
                  if (buffer) {
                    delete [] buffer;
                  }
                  client->close([client]() {
                    if (client->isClosed) {
                      delete client;
                    }
                  });
                  return;
                }
              } while (0);
              client->conduit->processFrame(client, buffer, nread);
            } else {
              client->conduit->handshake(client, buffer);
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

          if (buffer) {
            delete [] buffer;
          }
        }
      );
    });

    if (result) {
      debug("Conduit: Listen error %s\n", uv_strerror(result));
    }

    this->isStarting.store(false);

    if (callback != nullptr) {
      this->loop.dispatch(callback);
    }

    return result == 0;
  }

  bool Conduit::stop () {
    this->port = 0;
    this->isStarting.store(false);

    if (!this->isActive()) {
      return false;
    }

    return this->loop.dispatch([this]() {
      debug(">>> STOPPING CONDUIT");
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
              auto conduit = reinterpret_cast<Conduit*>(data);

              delete shutdown;

              conduit->loop.dispatch([=]() {
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

  bool Conduit::isActive () {
    Lock lock(this->mutex);
    return (
      this->port > 0 &&
      uv_is_active(reinterpret_cast<uv_handle_t*>(&this->socket)) &&
      !uv_is_closing(reinterpret_cast<uv_handle_t*>(&this->socket))
    );
  }
}
