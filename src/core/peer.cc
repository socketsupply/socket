#include "core.hh"

namespace SSC {
  void Core::resumeAllPeers () {
    dispatchEventLoop([=, this]() {
      Lock lock(this->peersMutex);
      for (auto const &tuple : this->peers) {
        auto peer = tuple.second;
        if (peer != nullptr && (peer->isBound() || peer->isConnected())) {
          peer->resume();
        }
      }
    });
  }

  void Core::pauseAllPeers () {
    dispatchEventLoop([=, this]() {
      Lock lock(this->peersMutex);
      for (auto const &tuple : this->peers) {
        auto peer = tuple.second;
        if (peer != nullptr && (peer->isBound() || peer->isConnected())) {
          peer->pause();
        }
      }
    });
  }

  bool Core::hasPeer (uint64_t peerId) {
    Lock lock(this->peersMutex);
    return this->peers.find(peerId) != this->peers.end();
  }

  void Core::removePeer (uint64_t peerId) {
    return this->removePeer(peerId, false);
  }

  void Core::removePeer (uint64_t peerId, bool autoClose) {
    if (this->hasPeer(peerId)) {
      if (autoClose) {
        auto peer = this->getPeer(peerId);
        if (peer != nullptr) {
          peer->close();
        }
      }

      Lock lock(this->peersMutex);
      this->peers.erase(peerId);
    }
  }

  Peer* Core::getPeer (uint64_t peerId) {
    if (!this->hasPeer(peerId)) return nullptr;
    Lock lock(this->peersMutex);
    return this->peers.at(peerId);
  }

  Peer* Core::createPeer (peer_type_t peerType, uint64_t peerId) {
    return this->createPeer(peerType, peerId, false);
  }

  Peer* Core::createPeer (
    peer_type_t peerType,
    uint64_t peerId,
    bool isEphemeral
  ) {
    if (this->hasPeer(peerId)) {
      auto peer = this->getPeer(peerId);
      if (peer != nullptr) {
        if (isEphemeral) {
          Lock lock(peer->mutex);
          peer->flags = (peer_flag_t) (peer->flags | PEER_FLAG_EPHEMERAL);
        }
      }

      return peer;
    }

    auto peer = new Peer(this, peerType, peerId, isEphemeral);
    Lock lock(this->peersMutex);
    this->peers[peer->id] = peer;
    return peer;
  }

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
      this->address = addrToIPv4((struct sockaddr_in*) addr);
      this->port = (int) htons(((struct sockaddr_in*) addr)->sin_port);
    } else if (addr->ss_family == AF_INET6) {
      this->family = "IPv6";
      this->address = addrToIPv6((struct sockaddr_in6*) addr);
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
      this->address = addrToIPv4((struct sockaddr_in*) addr);
      this->port = (int) htons(((struct sockaddr_in*) addr)->sin_port);
    } else if (addr->ss_family == AF_INET6) {
      this->family = "IPv6";
      this->address = addrToIPv6((struct sockaddr_in6*) addr);
      this->port = (int) htons(((struct sockaddr_in6*) addr)->sin6_port);
    }
  }

  Peer::Peer (
    Core *core,
    peer_type_t peerType,
    uint64_t peerId,
    bool isEphemeral
  ) {
    this->id = peerId;
    this->type = peerType;
    this->core = core;

    if (isEphemeral) {
      this->flags = (peer_flag_t) (this->flags | PEER_FLAG_EPHEMERAL);
    }

    this->init();
  }

  Peer::~Peer () {
    this->core->removePeer(this->id, true); // auto close
  }

  int Peer::init () {
    Lock lock(this->mutex);
    auto loop = this->core->getEventLoop();
    int err = 0;

    memset(&this->handle, 0, sizeof(this->handle));

    if (this->type == PEER_TYPE_UDP) {
      if ((err = uv_udp_init(loop, (uv_udp_t *) &this->handle))) {
        return err;
      }
      this->handle.udp.data = (void *) this;
    } else if (this->type == PEER_TYPE_TCP) {
      if ((err = uv_tcp_init(loop, (uv_tcp_t *) &this->handle))) {
        return err;
      }
      this->handle.tcp.data = (void *) this;
    }

    return err;
  }

  int Peer::initRemotePeerInfo () {
    Lock lock(this->mutex);
    if (this->type == PEER_TYPE_UDP) {
      this->remote.init((uv_udp_t *) &this->handle);
    } else if (this->type == PEER_TYPE_TCP) {
      this->remote.init((uv_tcp_t *) &this->handle);
    }
    return this->remote.err;
  }

  int Peer::initLocalPeerInfo () {
    Lock lock(this->mutex);
    if (this->type == PEER_TYPE_UDP) {
      this->local.init((uv_udp_t *) &this->handle);
    } else if (this->type == PEER_TYPE_TCP) {
      this->local.init((uv_tcp_t *) &this->handle);
    }
    return this->local.err;
  }

  void Peer::addState (peer_state_t value) {
    Lock lock(this->mutex);
    this->state = (peer_state_t) (this->state | value);
  }

  void Peer::removeState (peer_state_t value) {
    Lock lock(this->mutex);
    this->state = (peer_state_t) (this->state & ~value);
  }

  bool Peer::hasState (peer_state_t value) {
    Lock lock(this->mutex);
    return (value & this->state) == value;
  }

  const RemotePeerInfo* Peer::getRemotePeerInfo () {
    Lock lock(this->mutex);
    return &this->remote;
  }

  const LocalPeerInfo* Peer::getLocalPeerInfo () {
    Lock lock(this->mutex);
    return &this->local;
  }

  bool Peer::isUDP () {
    Lock lock(this->mutex);
    return this->type == PEER_TYPE_UDP;
  }

  bool Peer::isTCP () {
    Lock lock(this->mutex);
    return this->type == PEER_TYPE_TCP;
  }

  bool Peer::isEphemeral () {
    Lock lock(this->mutex);
    return (PEER_FLAG_EPHEMERAL & this->flags) == PEER_FLAG_EPHEMERAL;
  }

  bool Peer::isBound () {
    return (
      (this->isUDP() && this->hasState(PEER_STATE_UDP_BOUND)) ||
      (this->isTCP() && this->hasState(PEER_STATE_TCP_BOUND))
    );
  }

  bool Peer::isActive () {
    Lock lock(this->mutex);
    return uv_is_active((const uv_handle_t *) &this->handle);
  }

  bool Peer::isClosing () {
    Lock lock(this->mutex);
    return uv_is_closing((const uv_handle_t *) &this->handle);
  }

  bool Peer::isClosed () {
    return this->hasState(PEER_STATE_CLOSED);
  }

  bool Peer::isConnected () {
    return (
      (this->isUDP() && this->hasState(PEER_STATE_UDP_CONNECTED)) ||
      (this->isTCP() && this->hasState(PEER_STATE_TCP_CONNECTED))
    );
  }

  bool Peer::isPaused () {
    return (
      (this->isUDP() && this->hasState(PEER_STATE_UDP_PAUSED)) ||
      (this->isTCP() && this->hasState(PEER_STATE_TCP_PAUSED))
    );
  }

  int Peer::bind () {
    auto info = this->getLocalPeerInfo();

    if (info->err) {
      return info->err;
    }

    return this->bind(info->address, info->port, this->options.udp.reuseAddr);
  }

  int Peer::bind (const String address, int port) {
    return this->bind(address, port, false);
  }

  int Peer::bind (const String address, int port, bool reuseAddr) {
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

      this->addState(PEER_STATE_UDP_BOUND);
    }

    if (this->isTCP()) {
      // @TODO: `bind()` + `listen()?`
    }

    return this->initLocalPeerInfo();
  }

  int Peer::rebind () {
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

  int Peer::connect (const String address, int port) {
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

      this->addState(PEER_STATE_UDP_CONNECTED);
    }

    return this->initRemotePeerInfo();
  }

  int Peer::disconnect () {
    int err = 0;

    if (this->isUDP()) {
      if (this->isConnected()) {
        // calling `uv_udp_connect()` with `nullptr` for `addr`
        // should disconnect the connected socket
        Lock lock(this->mutex);
        if ((err = uv_udp_connect((uv_udp_t *) &this->handle, nullptr))) {
          return err;
        }

        this->removeState(PEER_STATE_UDP_CONNECTED);
      }
    }

    return err;
  }

  void Peer::send (
    char *buf,
    size_t size,
    int port,
    const String address,
    Peer::RequestContext::Callback cb
  ) {
    Lock lock(this->mutex);
    int err = 0;

    struct sockaddr *sockaddr = nullptr;

    if (!this->isConnected()) {
      sockaddr = (struct sockaddr *) &this->addr;
      err = uv_ip4_addr((char *) address.c_str(), port, &this->addr);

      if (err) {
        return cb(err, Post{});
      }
    }

    auto buffer = uv_buf_init(buf, (int) size);
    auto ctx = new Peer::RequestContext(cb);
    auto req = new uv_udp_send_t;

    req->data = (void *) ctx;
    ctx->peer = this;

    err = uv_udp_send(req, (uv_udp_t *) &this->handle, &buffer, 1, sockaddr, [](uv_udp_send_t *req, int status) {
      auto ctx = reinterpret_cast<Peer::RequestContext*>(req->data);
      auto peer = ctx->peer;

      ctx->cb(status, Post{});

      if (peer->isEphemeral()) {
        peer->close();
      }

      delete ctx;
      delete req;
    });

    if (err < 0) {
      ctx->cb(err, Post{});

      if (this->isEphemeral()) {
        this->close();
      }

      delete ctx;
      delete req;
    }
  }

  int Peer::recvstart () {
    if (this->receiveCallback != nullptr) {
      return this->recvstart(this->receiveCallback);
    }

    return UV_EINVAL;
  }

  int Peer::recvstart (Peer::UDPReceiveCallback receiveCallback) {
    Lock lock(this->mutex);

    if (this->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      return UV_EALREADY;
    }

    this->addState(PEER_STATE_UDP_RECV_STARTED);
    this->receiveCallback = receiveCallback;

    auto allocate = [](uv_handle_t *handle, size_t size, uv_buf_t *buf) {
      if (size > 0) {
        buf->base = (char *) new char[size]{0};
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
      auto peer = (Peer *) handle->data;

      if (nread == UV_ENOTCONN) {
        peer->recvstop();
        return;
      }

      peer->receiveCallback(nread, buf, addr);
    };

    return uv_udp_recv_start((uv_udp_t *) &this->handle, allocate, receive);
  }

  int Peer::recvstop () {
    int err = 0;

    if (this->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      this->removeState(PEER_STATE_UDP_RECV_STARTED);
      Lock lock(this->core->loopMutex);
      err = uv_udp_recv_stop((uv_udp_t *) &this->handle);
    }

    return err;
  }

  int Peer::resume () {
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

      this->removeState(PEER_STATE_UDP_PAUSED);
    }

    return err;
  }

  int Peer::pause () {
    int err = 0;

    if ((err = this->recvstop())) {
      return err;
    }

    if (!this->isPaused() && !this->isClosing()) {
      this->addState(PEER_STATE_UDP_PAUSED);
      if (this->isBound()) {
        Lock lock(this->mutex);
        uv_close((uv_handle_t *) &this->handle, nullptr);
      } else if (this->isConnected()) {
        // TODO
      }
    }

    return err;
  }

  void Peer::close () {
    return this->close(nullptr);
  }

  void Peer::close (std::function<void()> onclose) {
    if (this->isClosed()) {
      this->core->removePeer(this->id);
      onclose();
      return;
    }

    if (onclose != nullptr) {
      Lock lock(this->mutex);
      this->onclose.push_back(onclose);
    }

    if (this->isClosing()) {
      return;
    }

    if (this->type == PEER_TYPE_UDP) {
      Lock lock(this->mutex);
      // reset state and set to CLOSED
      uv_close((uv_handle_t*) &this->handle, [](uv_handle_t *handle) {
        auto peer = (Peer *) handle->data;
        if (peer != nullptr) {
          peer->removeState((peer_state_t) (
            PEER_STATE_UDP_BOUND |
            PEER_STATE_UDP_CONNECTED |
            PEER_STATE_UDP_RECV_STARTED
          ));

          for (const auto &onclose : peer->onclose) {
            onclose();
          }

          delete peer;
        }
      });
    }
  }
}
