#include "../udp.hh"

namespace ssc::runtime::udp {
  int LocalPeerInfo::getsockname (uv_udp_t *socket, struct sockaddr *addr) {
    int namelen = sizeof(struct sockaddr_storage);
    return uv_udp_getsockname(socket, addr, &namelen);
  }

  int LocalPeerInfo::getsockname (uv_tcp_t *socket, struct sockaddr *addr) {
    int namelen = sizeof(struct sockaddr_storage);
    return uv_tcp_getsockname(socket, addr, &namelen);
  }

  void LocalPeerInfo::init (uv_udp_t *socket) {
    this->address = "";
    this->family = "";
    this->port = 0;

    if ((this->err = getsockname(socket, (struct sockaddr *) &this->addr))) {
      return;
    }

    this->init(&this->addr);
  }

  void LocalPeerInfo::init (uv_tcp_t *socket) {
    this->address = "";
    this->family = "";
    this->port = 0;

    if ((this->err = getsockname(socket, (struct sockaddr *) &this->addr))) {
      return;
    }

    this->init(&this->addr);
  }

  void LocalPeerInfo::init (const struct sockaddr_storage *addr) {
    if (addr->ss_family == AF_INET) {
      this->family = "IPv4";
      this->address = udp::ip::addrToIPv4((struct sockaddr_in*) addr);
      this->port = (int) htons(((struct sockaddr_in*) addr)->sin_port);
    } else if (addr->ss_family == AF_INET6) {
      this->family = "IPv6";
      this->address = udp::ip::addrToIPv6((struct sockaddr_in6*) addr);
      this->port = (int) htons(((struct sockaddr_in6*) addr)->sin6_port);
    }
  }

  int RemotePeerInfo::getpeername (uv_udp_t *socket, struct sockaddr *addr) {
    int namelen = sizeof(struct sockaddr_storage);
    return uv_udp_getpeername(socket, addr, &namelen);
  }

  int RemotePeerInfo::getpeername (uv_tcp_t *socket, struct sockaddr *addr) {
    int namelen = sizeof(struct sockaddr_storage);
    return uv_tcp_getpeername(socket, addr, &namelen);
  }

  void RemotePeerInfo::init (uv_udp_t *socket) {
    this->address = "";
    this->family = "";
    this->port = 0;

    if ((this->err = getpeername(socket, (struct sockaddr *) &this->addr))) {
      return;
    }

    this->init(&this->addr);
  }

  void RemotePeerInfo::init (uv_tcp_t *socket) {
    this->address = "";
    this->family = "";
    this->port = 0;

    if ((this->err = getpeername(socket, (struct sockaddr *) &this->addr))) {
      return;
    }

    this->init(&this->addr);
  }

  void RemotePeerInfo::init (const struct sockaddr_storage *addr) {
    if (addr->ss_family == AF_INET) {
      this->family = "IPv4";
      this->address = udp::ip::addrToIPv4((struct sockaddr_in*) addr);
      this->port = (int) htons(((struct sockaddr_in*) addr)->sin_port);
    } else if (addr->ss_family == AF_INET6) {
      this->family = "IPv6";
      this->address = udp::ip::addrToIPv6((struct sockaddr_in6*) addr);
      this->port = (int) htons(((struct sockaddr_in6*) addr)->sin6_port);
    }
  }

  Socket::Socket (
    SocketManager* manager,
    socket_type_t peerType,
    uint64_t peerId,
    bool isEphemeral
  ) : manager(manager),
      type(peerType),
      id(peerId)
  {
    if (isEphemeral) {
      this->flags = (socket_flag_t) (this->flags | SOCKET_FLAG_EPHEMERAL);
    }

    this->init();
  }

  Socket::~Socket () {}

  int Socket::init () {
    Lock lock(this->mutex);
    auto loop = this->manager->loop.get();
    int err = 0;

    memset(&this->handle, 0, sizeof(this->handle));

    if (this->type == SOCKET_TYPE_UDP) {
      if ((err = uv_udp_init(loop, (uv_udp_t *) &this->handle))) {
        return err;
      }
      this->handle.udp.data = (void *) this;
    } else if (this->type == SOCKET_TYPE_TCP) {
      if ((err = uv_tcp_init(loop, (uv_tcp_t *) &this->handle))) {
        return err;
      }
      this->handle.tcp.data = (void *) this;
    }

    return err;
  }

  int Socket::initRemotePeerInfo () {
    Lock lock(this->mutex);
    if (this->type == SOCKET_TYPE_UDP) {
      this->remote.init((uv_udp_t *) &this->handle);
    } else if (this->type == SOCKET_TYPE_TCP) {
      this->remote.init((uv_tcp_t *) &this->handle);
    }
    return this->remote.err;
  }

  int Socket::initLocalPeerInfo () {
    Lock lock(this->mutex);
    if (this->type == SOCKET_TYPE_UDP) {
      this->local.init((uv_udp_t *) &this->handle);
    } else if (this->type == SOCKET_TYPE_TCP) {
      this->local.init((uv_tcp_t *) &this->handle);
    }
    return this->local.err;
  }

  void Socket::addState (socket_state_t value) {
    Lock lock(this->mutex);
    this->state = (socket_state_t) (this->state | value);
  }

  void Socket::removeState (socket_state_t value) {
    Lock lock(this->mutex);
    this->state = (socket_state_t) (this->state & ~value);
  }

  bool Socket::hasState (socket_state_t value) {
    Lock lock(this->mutex);
    return (value & this->state) == value;
  }

  const RemotePeerInfo* Socket::getRemotePeerInfo () {
    Lock lock(this->mutex);
    return &this->remote;
  }

  const LocalPeerInfo* Socket::getLocalPeerInfo () {
    Lock lock(this->mutex);
    return &this->local;
  }

  bool Socket::isUDP () {
    Lock lock(this->mutex);
    return this->type == SOCKET_TYPE_UDP;
  }

  bool Socket::isTCP () {
    Lock lock(this->mutex);
    return this->type == SOCKET_TYPE_TCP;
  }

  bool Socket::isEphemeral () {
    Lock lock(this->mutex);
    return (SOCKET_FLAG_EPHEMERAL & this->flags) == SOCKET_FLAG_EPHEMERAL;
  }

  bool Socket::isBound () {
    return (
      (this->isUDP() && this->hasState(SOCKET_STATE_UDP_BOUND)) ||
      (this->isTCP() && this->hasState(SOCKET_STATE_TCP_BOUND))
    );
  }

  bool Socket::isActive () {
    Lock lock(this->mutex);
    return uv_is_active((const uv_handle_t *) &this->handle);
  }

  bool Socket::isClosing () {
    Lock lock(this->mutex);
    return uv_is_closing((const uv_handle_t *) &this->handle);
  }

  bool Socket::isClosed () {
    return this->hasState(SOCKET_STATE_CLOSED);
  }

  bool Socket::isConnected () {
    return (
      (this->isUDP() && this->hasState(SOCKET_STATE_UDP_CONNECTED)) ||
      (this->isTCP() && this->hasState(SOCKET_STATE_TCP_CONNECTED))
    );
  }

  bool Socket::isPaused () {
    return (
      (this->isUDP() && this->hasState(SOCKET_STATE_UDP_PAUSED)) ||
      (this->isTCP() && this->hasState(SOCKET_STATE_TCP_PAUSED))
    );
  }

  int Socket::bind () {
    auto info = this->getLocalPeerInfo();

    if (info->err) {
      return info->err;
    }

    return this->bind(info->address, info->port, this->options.udp.reuseAddr);
  }

  int Socket::bind (const String& address, int port) {
    return this->bind(address, port, false);
  }

  int Socket::bind (const String& address, int port, bool reuseAddr) {
    Lock lock(this->mutex);
    auto sockaddr = (struct sockaddr*) &this->addr;
    int flags = 0;
    int err = 0;

    this->options.udp.reuseAddr = reuseAddr;

    if (reuseAddr) {
      flags |= UV_UDP_REUSEADDR;
    }

    if (this->isUDP()) {
      if ((err = uv_ip4_addr((char *) address.c_str(), port, &this->addr))) {
        return err;
      }

      // @TODO(jwerle): support flags in `bind()`
      if ((err = uv_udp_bind((uv_udp_t *) &this->handle, sockaddr, flags))) {
        return err;
      }

      this->addState(SOCKET_STATE_UDP_BOUND);
    }

    if (this->isTCP()) {
      // @TODO: `bind()` + `listen()?`
    }

    return this->initLocalPeerInfo();
  }

  int Socket::rebind () {
    int err = 0;

    if (this->isUDP()) {
      if ((err = this->recvstop())) {
        return err;
      }
    }

    Lock lock(this->mutex);
    memset((void *) &this->addr, 0, sizeof(struct sockaddr_in));

    if ((err = this->bind())) {
      return err;
    }

    if (this->isUDP()) {
      if ((err = this->recvstart())) {
        return err;
      }
    }

    return err;
  }

  int Socket::connect (const String& address, int port) {
    Lock lock(this->mutex);
    auto sockaddr = (struct sockaddr*) &this->addr;
    int err = 0;

    if ((err = uv_ip4_addr((char *) address.c_str(), port, &this->addr))) {
      return err;
    }

    if (this->isUDP()) {
      if ((err = uv_udp_connect((uv_udp_t *) &this->handle, sockaddr))) {
        return err;
      }

      this->addState(SOCKET_STATE_UDP_CONNECTED);
    }

    return this->initRemotePeerInfo();
  }

  int Socket::disconnect () {
    int err = 0;

    if (this->isUDP()) {
      if (this->isConnected()) {
        // calling `uv_udp_connect()` with `nullptr` for `addr`
        // should disconnect the connected socket
        Lock lock(this->mutex);
        if ((err = uv_udp_connect((uv_udp_t *) &this->handle, nullptr))) {
          return err;
        }

        this->removeState(SOCKET_STATE_UDP_CONNECTED);
      }
    }

    return err;
  }

  void Socket::send (
    SharedPointer<unsigned char[]> bytes,
    size_t size,
    int port,
    const String& address,
    const Socket::RequestContext::Callback callback
  ) {
    Lock lock(this->mutex);
    int err = 0;

    struct sockaddr *sockaddr = nullptr;

    if (!this->isConnected()) {
      sockaddr = (struct sockaddr *) &this->addr;
      err = uv_ip4_addr((char *) address.c_str(), port, &this->addr);

      if (err) {
        return callback(err, QueuedResponse{});
      }
    }

    auto ctx = new Socket::RequestContext(size, bytes, callback);
    auto req = new uv_udp_send_t;

    req->data = (void *) ctx;
    ctx->socket = this;

    err = uv_udp_send(req, (uv_udp_t *) &this->handle, &ctx->buffer, 1, sockaddr, [](uv_udp_send_t *req, int status) {
      auto ctx = reinterpret_cast<Socket::RequestContext*>(req->data);
      auto socket = ctx->socket;

      ctx->callback(status, QueuedResponse{});

      if (socket->isEphemeral()) {
        socket->close();
      }

      delete ctx;
      delete req;
    });

    if (err < 0) {
      ctx->callback(err, QueuedResponse{});

      if (this->isEphemeral()) {
        this->close();
      }

      delete ctx;
      delete req;
    }
  }

  int Socket::recvstart () {
    if (this->receiveCallback != nullptr) {
      return this->recvstart(this->receiveCallback);
    }

    return UV_EINVAL;
  }

  int Socket::recvstart (Socket::UDPReceiveCallback receiveCallback) {
    Lock lock(this->mutex);

    if (this->hasState(SOCKET_STATE_UDP_RECV_STARTED)) {
      return UV_EALREADY;
    }

    this->addState(SOCKET_STATE_UDP_RECV_STARTED);
    this->receiveCallback = receiveCallback;

    auto allocate = [](uv_handle_t *handle, size_t size, uv_buf_t *buf) {
      if (size > 0) {
        buf->base = (char *) new unsigned char[size]{0};
        buf->len = size;
      }
    };

    auto receive = [](
      uv_udp_t *handle,
      ssize_t nread,
      const uv_buf_t *buf,
      const struct sockaddr *addr,
      unsigned flags
    ) {
      auto socket = (Socket *) handle->data;

      if (nread == UV_ENOTCONN) {
        socket->recvstop();
        return;
      }

      socket->receiveCallback(nread, buf, addr);
    };

    return uv_udp_recv_start((uv_udp_t *) &this->handle, allocate, receive);
  }

  int Socket::recvstop () {
    if (this->hasState(SOCKET_STATE_UDP_RECV_STARTED)) {
      this->removeState(SOCKET_STATE_UDP_RECV_STARTED);
      return uv_udp_recv_stop((uv_udp_t *) &this->handle);
    }

    return 0;
  }

  int Socket::resume () {
    int err = 0;

    if (this->isPaused()) {
      if ((err = this->init())) {
        return err;
      }

      if (this->isBound()) {
        if ((err = this->rebind())) {
          return err;
        }
      } else if (this->isConnected()) {
        // @TODO
      }

      this->removeState(SOCKET_STATE_UDP_PAUSED);
    }

    return err;
  }

  int Socket::pause () {
    int err = 0;

    if ((err = this->recvstop())) {
      return err;
    }

    if (!this->isPaused() && !this->isClosing()) {
      this->addState(SOCKET_STATE_UDP_PAUSED);
      if (this->isBound()) {
        Lock lock(this->mutex);
        if (
          !uv_is_closing(reinterpret_cast<uv_handle_t*>(&this->handle))
        ) {
          uv_close((uv_handle_t *) &this->handle, nullptr);
        }
      } else if (this->isConnected()) {
        // TODO
      }
    }

    return err;
  }

  void Socket::close () {
    return this->close(nullptr);
  }

  void Socket::close (Function<void()> onclose) {
    Lock lock(this->mutex);

    if (this->isClosed()) {
      this->manager->remove(this->id);
      if (onclose != nullptr) {
        onclose();
      }
      return;
    }

    if (this->isClosing()) {
      if (onclose != nullptr) {
        onclose();
      }
      return;
    }

    if (onclose != nullptr) {
      this->onclose.push_back(onclose);
    }

    if (this->type == SOCKET_TYPE_UDP) {
      // reset state and set to CLOSED
      uv_close((uv_handle_t*) &this->handle, [](uv_handle_t *handle) {
        auto socket = (Socket *) handle->data;
        if (socket != nullptr) {
          socket->removeState((socket_state_t) (
            SOCKET_STATE_UDP_BOUND |
            SOCKET_STATE_UDP_CONNECTED |
            SOCKET_STATE_UDP_RECV_STARTED
          ));

          for (const auto &onclose : socket->onclose) {
            onclose();
          }

          socket->manager->remove(socket->id);
          socket = nullptr;
        }
      });
    }
  }
}
