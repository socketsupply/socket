#ifndef SSC_CORE_PEER_H
#define SSC_CORE_PEER_H
namespace SSC {
  typedef enum {
    PEER_TYPE_NONE = 0,
    PEER_TYPE_TCP = 1 << 1,
    PEER_TYPE_UDP = 1 << 2,
    PEER_TYPE_MAX = 0xF
  } peer_type_t;

  typedef enum {
    PEER_FLAG_NONE = 0,
    PEER_FLAG_EPHEMERAL = 1 << 1
  } peer_flag_t;

  typedef enum {
    PEER_STATE_NONE = 0,
    // general states
    PEER_STATE_CLOSED = 1 << 1,
    // udp states (10)
    PEER_STATE_UDP_BOUND = 1 << 10,
    PEER_STATE_UDP_CONNECTED = 1 << 11,
    PEER_STATE_UDP_RECV_STARTED = 1 << 12,
    PEER_STATE_UDP_PAUSED = 1 << 13,
    // tcp states (20)
    PEER_STATE_TCP_BOUND = 1 << 20,
    PEER_STATE_TCP_CONNECTED = 1 << 21,
    PEER_STATE_TCP_PAUSED = 1 << 13,
    PEER_STATE_MAX = 1 << 0xF
  } peer_state_t;

  struct LocalPeerInfo {
    struct sockaddr_storage addr;
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    int getsockname (uv_udp_t *socket, struct sockaddr *addr);
    int getsockname (uv_tcp_t *socket, struct sockaddr *addr);
    void init (uv_udp_t *socket);
    void init (uv_tcp_t *socket);
    void init (const struct sockaddr_storage *addr);
  };

  struct RemotePeerInfo {
    struct sockaddr_storage addr;
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    int getpeername (uv_udp_t *socket, struct sockaddr *addr);
    int getpeername (uv_tcp_t *socket, struct sockaddr *addr);
    void init (uv_udp_t *socket);
    void init (uv_tcp_t *socket);
    void init (const struct sockaddr_storage *addr);
  };

  /**
   * A generic structure for a bound or connected peer.
   */
  class Peer {
    public:
      struct RequestContext {
        using Callback = Function<void(int, Post)>;
        Callback cb;
        Peer *peer = nullptr;
        RequestContext (Callback cb) { this->cb = cb; }
      };

      using UDPReceiveCallback = Function<void(
        ssize_t,
        const uv_buf_t*,
        const struct sockaddr*
      )>;

      // uv handles
      union {
        uv_udp_t udp;
        uv_tcp_t tcp; // XXX: FIXME
      } handle;

      // sockaddr
      struct sockaddr_in addr;

      // callbacks
      UDPReceiveCallback receiveCallback;
      std::vector<Function<void()>> onclose;

      // instance state
      uint64_t id = 0;
      std::recursive_mutex mutex;
      Core *core;

      struct {
        struct {
          bool reuseAddr = false;
          bool ipv6Only = false; // @TODO
        } udp;
      } options;

      // peer state
      LocalPeerInfo local;
      RemotePeerInfo remote;
      peer_type_t type = PEER_TYPE_NONE;
      peer_flag_t flags = PEER_FLAG_NONE;
      peer_state_t state = PEER_STATE_NONE;

      /**
      * Private `Peer` class constructor
      */
      Peer (Core *core, peer_type_t peerType, uint64_t peerId, bool isEphemeral);
      ~Peer ();

      int init ();
      int initRemotePeerInfo ();
      int initLocalPeerInfo ();
      void addState (peer_state_t value);
      void removeState (peer_state_t value);
      bool hasState (peer_state_t value);
      const RemotePeerInfo* getRemotePeerInfo ();
      const LocalPeerInfo* getLocalPeerInfo ();
      bool isUDP ();
      bool isTCP ();
      bool isEphemeral ();
      bool isBound ();
      bool isActive ();
      bool isClosing ();
      bool isClosed ();
      bool isConnected ();
      bool isPaused ();
      int bind ();
      int bind (String address, int port);
      int bind (String address, int port, bool reuseAddr);
      int rebind ();
      int connect (String address, int port);
      int disconnect ();
      void send (
        char *buf,
        size_t size,
        int port,
        const String address,
        Peer::RequestContext::Callback cb
      );
      int recvstart ();
      int recvstart (UDPReceiveCallback onrecv);
      int recvstop ();
      int resume ();
      int pause ();
      void close ();
      void close (Function<void()> onclose);
  };
}
#endif
