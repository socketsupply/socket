#ifndef SSC_CORE_H
#define SSC_CORE_H

#include "common.hh"
#include "include/uv.h"

#ifndef _WIN32
#include <ifaddrs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#endif

#if defined(__linux__) && !defined(__ANDROID__)
#include <gtk/gtk.h>
#endif

#include <chrono>
#include <semaphore>
#include <thread>

#if defined(__APPLE__)
	#import <Webkit/Webkit.h>
  using Task = id<WKURLSchemeTask>;
#  define debug(format, ...) NSLog(@format, ##__VA_ARGS__)
#else
  class Task {};
#endif

#ifndef debug
#  define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif

namespace SSC {
  using String = std::string;

  struct Post {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char* body = nullptr;
    int length = 0;
    String headers = "";
    bool bodyNeedsFree = false;
  };

  using Cb = std::function<void(String, String, Post)>;
  using Tasks = std::map<String, Task>;
  using Posts = std::map<uint64_t, Post>;

  class Core {
    std::unique_ptr<Tasks> tasks;
    std::unique_ptr<Posts> posts;

    public:
      static std::map<std::string, std::string> getFSConstantsMap () {
        std::map<std::string, std::string> constants;

        #define SET_CONSTANT(c) constants[#c] = std::to_string(c);

        #ifdef UV_DIRENT_UNKNOWN
          SET_CONSTANT(UV_DIRENT_UNKNOWN)
        #endif

        #ifdef UV_DIRENT_FILE
          SET_CONSTANT(UV_DIRENT_FILE)
        #endif

        #ifdef UV_DIRENT_DIR
          SET_CONSTANT(UV_DIRENT_DIR)
        #endif

        #ifdef UV_DIRENT_LINK
          SET_CONSTANT(UV_DIRENT_LINK)
        #endif

        #ifdef UV_DIRENT_FIFO
          SET_CONSTANT(UV_DIRENT_FIFO)
        #endif

        #ifdef UV_DIRENT_SOCKET
          SET_CONSTANT(UV_DIRENT_SOCKET)
        #endif

        #ifdef UV_DIRENT_CHAR
          SET_CONSTANT(UV_DIRENT_CHAR)
        #endif

        #ifdef UV_DIRENT_BLOCK
          SET_CONSTANT(UV_DIRENT_BLOCK)
        #endif

        #ifdef O_RDONLY
          SET_CONSTANT(O_RDONLY);
        #endif

        #ifdef O_WRONLY
          SET_CONSTANT(O_WRONLY);
        #endif

        #ifdef O_RDWR
          SET_CONSTANT(O_RDWR);
        #endif

        #ifdef O_APPEND
          SET_CONSTANT(O_APPEND);
        #endif

        #ifdef O_ASYNC
          SET_CONSTANT(O_ASYNC);
        #endif

        #ifdef O_CLOEXEC
          SET_CONSTANT(O_CLOEXEC);
        #endif

        #ifdef O_CREAT
          SET_CONSTANT(O_CREAT);
        #endif

        #ifdef O_DIRECT
          SET_CONSTANT(O_DIRECT);
        #endif

        #ifdef O_DIRECTORY
          SET_CONSTANT(O_DIRECTORY);
        #endif

        #ifdef O_DSYNC
          SET_CONSTANT(O_DSYNC);
        #endif

        #ifdef O_EXCL
          SET_CONSTANT(O_EXCL);
        #endif

        #ifdef O_LARGEFILE
          SET_CONSTANT(O_LARGEFILE);
        #endif

        #ifdef O_NOATIME
          SET_CONSTANT(O_NOATIME);
        #endif

        #ifdef O_NOCTTY
          SET_CONSTANT(O_NOCTTY);
        #endif

        #ifdef O_NOFOLLOW
          SET_CONSTANT(O_NOFOLLOW);
        #endif

        #ifdef O_NONBLOCK
          SET_CONSTANT(O_NONBLOCK);
        #endif

        #ifdef O_NDELAY
          SET_CONSTANT(O_NDELAY);
        #endif

        #ifdef O_PATH
          SET_CONSTANT(O_PATH);
        #endif

        #ifdef O_SYNC
          SET_CONSTANT(O_SYNC);
        #endif

        #ifdef O_TMPFILE
          SET_CONSTANT(O_TMPFILE);
        #endif

        #ifdef O_TRUNC
          SET_CONSTANT(O_TRUNC);
        #endif

        #ifdef S_IFMT
          SET_CONSTANT(S_IFMT);
        #endif

        #ifdef S_IFREG
          SET_CONSTANT(S_IFREG);
        #endif

        #ifdef S_IFDIR
          SET_CONSTANT(S_IFDIR);
        #endif

        #ifdef S_IFCHR
          SET_CONSTANT(S_IFCHR);
        #endif

        #ifdef S_IFBLK
          SET_CONSTANT(S_IFBLK);
        #endif

        #ifdef S_IFIFO
          SET_CONSTANT(S_IFIFO);
        #endif

        #ifdef S_IFLNK
          SET_CONSTANT(S_IFLNK);
        #endif

        #ifdef S_IFSOCK
          SET_CONSTANT(S_IFSOCK);
        #endif

        #ifdef S_IRWXU
          SET_CONSTANT(S_IRWXU);
        #endif

        #ifdef S_IRUSR
          SET_CONSTANT(S_IRUSR);
        #endif

        #ifdef S_IWUSR
          SET_CONSTANT(S_IWUSR);
        #endif

        #ifdef S_IXUSR
          SET_CONSTANT(S_IXUSR);
        #endif

        #ifdef S_IRWXG
          SET_CONSTANT(S_IRWXG);
        #endif

        #ifdef S_IRGRP
          SET_CONSTANT(S_IRGRP);
        #endif

        #ifdef S_IWGRP
          SET_CONSTANT(S_IWGRP);
        #endif

        #ifdef S_IXGRP
          SET_CONSTANT(S_IXGRP);
        #endif

        #ifdef S_IRWXO
          SET_CONSTANT(S_IRWXO);
        #endif

        #ifdef S_IROTH
          SET_CONSTANT(S_IROTH);
        #endif

        #ifdef S_IWOTH
          SET_CONSTANT(S_IWOTH);
        #endif

        #ifdef S_IXOTH
          SET_CONSTANT(S_IXOTH);
        #endif

        #ifdef F_OK
          SET_CONSTANT(F_OK);
        #endif

        #ifdef R_OK
          SET_CONSTANT(R_OK);
        #endif

        #ifdef W_OK
          SET_CONSTANT(W_OK);
        #endif

        #ifdef X_OK
          SET_CONSTANT(X_OK);
        #endif

        #undef SET_CONSTANT

        return constants;
      }

      String getFSConstants () const;

      void handleEvent (String seq, String event, String data, Cb cb) const;

      void fsAccess (String seq, String path, int mode, Cb cb) const;
      void fsChmod (String seq, String path, int mode, Cb cb) const;
      void fsCopyFile (String seq, String src, String dst, int mode, Cb cb) const;
      void fsClose (String seq, uint64_t id, Cb cb) const;
      void fsClosedir (String seq, uint64_t id, Cb cb) const;
      void fsCloseOpenDescriptor (String seq, uint64_t id, Cb cb) const;
      void fsCloseOpenDescriptors (String seq, Cb cb) const;
      void fsCloseOpenDescriptors (String seq, bool preserveRetained, Cb cb) const;
      void fsFStat (String seq, uint64_t id, Cb cb) const;
      void fsGetOpenDescriptors (String seq, Cb cb) const;
      void fsMkdir (String seq, String path, int mode, Cb cb) const;
      void fsOpen (String seq, uint64_t id, String path, int flags, int mode, Cb cb) const;
      void fsOpendir (String seq, uint64_t id, String path, Cb cb) const;
      void fsRead (String seq, uint64_t id, int len, int offset, Cb cb) const;
      void fsReaddir (String seq, uint64_t id, size_t entries, Cb cb) const;
      void fsRetainOpenDescriptor (String seq, uint64_t id, Cb cb) const;
      void fsRename (String seq, String pathA, String pathB, Cb cb) const;
      void fsRmdir (String seq, String path, Cb cb) const;
      void fsStat (String seq, String path, Cb cb) const;
      void fsUnlink (String seq, String path, Cb cb) const;
      void fsWrite (String seq, uint64_t id, String data, int64_t offset, Cb cb) const;

      void udpBind (String seq, uint64_t peerId, String address, int port, bool reuseAddr, Cb cb) const;
      void udpConnect (String seq, uint64_t peerId, String address, int port, Cb cb) const;
      void udpGetPeerName (String seq, uint64_t peerId, Cb cb) const;
      void udpGetSockName (String seq, uint64_t peerId, Cb cb) const;
      void udpGetState (String seq, uint64_t peerId,  Cb cb) const;
      void udpReadStart (String seq, uint64_t peerId, Cb cb) const;
      void udpSend (String seq, uint64_t peerId, char* buf, int len, int port, String address, bool ephemeral, Cb cb) const;

      void sendBufferSize (String seq, uint64_t peerId, int size, Cb cb) const;
      void recvBufferSize (String seq, uint64_t peerId, int size, Cb cb) const;
      void close (String seq, uint64_t peerId, Cb cb) const;
      void shutdown (String seq, uint64_t peerId, Cb cb) const;
      void readStop (String seq, uint64_t peerId, Cb cb) const;

      void dnsLookup (String seq, String hostname, int family, Cb cb) const;
      String getNetworkInterfaces () const;

      Task getTask (String id);
      bool hasTask (String id);
      void removeTask (String id);
      void putTask (String id, Task t);

      Post getPost (uint64_t id);
      bool hasPost (uint64_t id);
      void removePost (uint64_t id);
      void removeAllPosts ();
      void expirePosts ();
      void putPost (uint64_t id, Post p);
      String createPost (String seq, String params, Post post);

      Core() {
        this->tasks = std::unique_ptr<Tasks>(new Tasks());
        this->posts = std::unique_ptr<Posts>(new Posts());
      }
  };

  // forward
  struct Peer;
  struct PeerRequestContext;
  struct Descriptor;
  struct DescriptorRequestContext;

  static std::map<uint64_t, Peer*> peers;
  static std::map<uint64_t, Descriptor*> descriptors;

  static std::recursive_mutex descriptorsMutex;
  static std::recursive_mutex loopMutex;
  static std::recursive_mutex peersMutex;
  static std::recursive_mutex postsMutex;
  static std::recursive_mutex tasksMutex;
  static std::recursive_mutex timersMutex;

  static std::atomic<bool> didLoopInit = false;
  static std::atomic<bool> didTimersInit = false;
  static std::atomic<bool> didTimersStart = false;

  static std::atomic<bool> isLoopRunning = false;

  static constexpr int EVENT_LOOP_POLL_TIMEOUT = 32; // in milliseconds
  static uv_loop_t eventLoop = {0};

#if defined(__APPLE__)
  static dispatch_queue_attr_t eventLoopQueueAttrs = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_SERIAL,
    QOS_CLASS_DEFAULT,
    -1
  );

  static dispatch_queue_t eventLoopQueue = dispatch_queue_create(
    "co.socketsupply.queue.event-loop",
    eventLoopQueueAttrs
  );
#else
  static std::thread *eventLoopThread = nullptr;
#endif

  struct Descriptor {
    uv_file fd = 0;
    uv_dir_t *dir = nullptr;
    String seq;
    Cb cb;
    uint64_t id;
    std::atomic<bool> retained = false;
    std::atomic<bool> stale = false;
    std::recursive_mutex mutex;
    void *data;

    static Descriptor * get (uint64_t id) {
      std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
      if (descriptors.find(id) != descriptors.end()) {
        return descriptors.at(id);
      }
      return nullptr;
    }

    static void remove (uint64_t id) {
      std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
      if (descriptors.find(id) != descriptors.end()) {
        descriptors.erase(id);
      }
    }

    static bool exists (uint64_t id) {
      std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
      return descriptors.find(id) != descriptors.end();
    }

    Descriptor (uint64_t id) {
      this->id = id;
    }
  };

  struct Timer {
    uv_timer_t handle;
    bool repeated = false;
    bool started = false;
    uint64_t timeout = 0;
    uint64_t interval = 0;
    uv_timer_cb invoke;
  };

  struct Timers {
    Core core; // isolate
    Timer releaseWeakDescriptors = {
      .timeout = 256, // in milliseconds
      .invoke = [](uv_timer_t *handle) {
        std::lock_guard<std::recursive_mutex> descriptorsGuard(descriptorsMutex);
        std::vector<uint64_t> ids;
        std::string msg = "";

        auto core = reinterpret_cast<Core *>(handle->data);

        for (auto const &tuple : descriptors) {
          ids.push_back(tuple.first);
        }

        for (auto const id : ids) {
          auto desc = descriptors.at(id);

          if (desc == nullptr) {
            descriptors.erase(id);
            continue;
          }

          if (desc->retained || !desc->stale) {
            continue;
          }

          if (desc->dir != nullptr) {
            core->fsClosedir("", desc->id, [](auto seq, auto msg, auto post) {
              // noop
            });
          } else if (desc->fd > 0) {
            core->fsClose("", desc->id, [](auto seq, auto msg, auto post) {
              // noop
            });
          } else {
            // free
            descriptors.erase(id);
            delete desc;
          }
        }
      }
    };
  };

  struct EventLoopDispatchContext {
    typedef std::function<void()> Callback;
    uv_async_t async;
    uv_handle_t *handle;
    Callback dispatch;

    EventLoopDispatchContext (Callback dispatch) {
      this->dispatch = dispatch;
      this->handle = (uv_handle_t *) &this->async;
      this->handle->data = this;
    }

    void close () {
      uv_close(this->handle, [](uv_handle_t *handle) {
        if (handle != nullptr) {
          delete (EventLoopDispatchContext *) handle->data;
        }
      });
    }
  };

  // timers
  static Timers timers;
  static void initTimers ();
  static void startTimers ();

  // loop
  static uv_loop_t* getEventLoop ();
  static int getEventLoopTimeout ();
  static bool isLoopAlive ();
  static int runEventLoop ();
  static void stopEventLoop ();
  static void dispatchEventLoop (EventLoopDispatchContext::Callback dispatch);

  static String addrToIPv4 (struct sockaddr_in* sin) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin->sin_addr, buf, INET_ADDRSTRLEN);
    return String(buf);
  }

  static String addrToIPv6 (struct sockaddr_in6* sin) {
    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sin->sin6_addr, buf, INET6_ADDRSTRLEN);
    return String(buf);
  }

  static void parseAddress (struct sockaddr *name, int* port, char* address) {
    struct sockaddr_in *name_in = (struct sockaddr_in *) name;
    *port = ntohs(name_in->sin_port);
    uv_ip4_name(name_in, address, 17);
  }

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

  struct PeerRequestContext {
    uint64_t id;
    String seq;
    Cb cb;
    Peer* peer;
    char *buf;
    int bufsize;
    String address;
    int port;

    PeerRequestContext () {
      this->id = SSC::rand64();
    }

    PeerRequestContext (String s, Cb c)
      : PeerRequestContext(s, c, nullptr)
    {
      // noop
    }

    PeerRequestContext (String s, Cb c, Peer *p) {
      this->id = SSC::rand64();
      this->cb = c;
      this->seq = s;
      this->peer = p;
    }

    void end (String seq, String msg, Post post) {
      auto cb = this->cb;
      delete this;
      if (cb != nullptr) {
        cb(seq, msg, post);
      }
    }

    void end (String seq, String msg) {
      this->end(seq, msg, Post{});
    }

    void end (String msg, Post post) {
      this->end(this->seq, msg, post);
    }

    void end (String msg) {
      this->end(this->seq, msg, Post{});
    }
  };

  struct LocalPeerInfo {
    struct sockaddr_storage addr;
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    int getsockname (uv_udp_t *socket, struct sockaddr *addr) {
      int namelen = sizeof(struct sockaddr_storage);
      return uv_udp_getsockname(socket, addr, &namelen);
    }

    int getsockname (uv_tcp_t *socket, struct sockaddr *addr) {
      int namelen = sizeof(struct sockaddr_storage);
      return uv_tcp_getsockname(socket, addr, &namelen);
    }

    void init (uv_udp_t *socket) {
      this->address = "";
      this->family = "";
      this->port = 0;

      if ((this->err = getsockname(socket, (struct sockaddr *) &this->addr))) {
        return;
      }

      this->init(&this->addr);
    }

    void init (uv_tcp_t *socket) {
      this->address = "";
      this->family = "";
      this->port = 0;

      if ((this->err = getsockname(socket, (struct sockaddr *) &this->addr))) {
        return;
      }

      this->init(&this->addr);
    }

    void init (const struct sockaddr_storage *addr) {
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
  };

  struct RemotePeerInfo {
    struct sockaddr_storage addr;
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    int getpeername (uv_udp_t *socket, struct sockaddr *addr) {
      int namelen = sizeof(struct sockaddr_storage);
      return uv_udp_getpeername(socket, addr, &namelen);
    }

    int getpeername (uv_tcp_t *socket, struct sockaddr *addr) {
      int namelen = sizeof(struct sockaddr_storage);
      return uv_tcp_getpeername(socket, addr, &namelen);
    }

    void init (uv_udp_t *socket) {
      this->address = "";
      this->family = "";
      this->port = 0;

      if ((this->err = getpeername(socket, (struct sockaddr *) &this->addr))) {
        return;
      }

      this->init(&this->addr);
    }

    void init (uv_tcp_t *socket) {
      this->address = "";
      this->family = "";
      this->port = 0;

      if ((this->err = getpeername(socket, (struct sockaddr *) &this->addr))) {
        return;
      }

      this->init(&this->addr);
    }

    void init (const struct sockaddr_storage *addr) {
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
  };

  /**
   * A generic structure for a bound or connected peer.
   */
  struct Peer {
    // uv handles
    union {
      uv_udp_t udp;
      uv_tcp_t tcp; // XXX: FIXME
    } handle;

    // sockaddr
    struct sockaddr_in addr;

    // callbacks
    Cb recv;

    // instance state
    uint64_t id = 0;
    std::recursive_mutex mutex;

    struct {
      struct {
        bool reuseAddr = false;
      } udp;
    } options;

    // peer state
    LocalPeerInfo local;
    RemotePeerInfo remote;
    peer_type_t type = PEER_TYPE_NONE;
    peer_flag_t flags = PEER_FLAG_NONE;
    peer_state_t state = PEER_STATE_NONE;

    static void resumeAll () {
      dispatchEventLoop([=]() {
        std::lock_guard<std::recursive_mutex> guard(peersMutex);
        for (auto const &tuple : peers) {
          auto peer = tuple.second;
          if (peer != nullptr && (peer->isBound() || peer->isConnected())) {
            peer->resume();
          }
        }
      });
    }

    static void pauseAll () {
      dispatchEventLoop([=]() {
        std::lock_guard<std::recursive_mutex> guard(peersMutex);

        for (auto const &tuple : peers) {
          auto peer = tuple.second;
          if (peer != nullptr && (peer->isBound() || peer->isConnected())) {
            peer->pause();
          }
        }
      });
    }

    /**
     * Checks if a `Peer` exists by `peerId`.
     */
    static bool exists (uint64_t peerId) {
      std::lock_guard<std::recursive_mutex> guard(peersMutex);
      return peers.find(peerId) != peers.end();
    }

    /**
     * Remove a `Peer` by `peerId` optionally auto closing it.
     */
    static void remove (uint64_t peerId) { return Peer::remove(peerId, false); }
    static void remove (uint64_t peerId, bool autoClose) {
      if (Peer::exists(peerId)) {
        if (autoClose) {
          auto peer = Peer::get(peerId);
          if (peer != nullptr) {
            peer->close();
          }
        }

        std::lock_guard<std::recursive_mutex> guard(peersMutex);
        peers.erase(peerId);
      }
    }

    /**
     * Get a peer by `peerId` returning `nullptr` if it doesn't exist.
     */
    static Peer* get (uint64_t peerId) {
      if (!Peer::exists(peerId)) return nullptr;
      std::lock_guard<std::recursive_mutex> guard(peersMutex);
      return peers.at(peerId);
    }

    /**
     * Factory for creating a new `Peer` of `peerType` and `peerId`
     */
    static Peer* create (peer_type_t peerType, uint64_t peerId) {
      return Peer::create(peerType, peerId, false);
    }

    static Peer* create (
      peer_type_t peerType,
      uint64_t peerId,
      bool isEphemeral
    ) {
      if (Peer::exists(peerId)) {
        auto peer = Peer::get(peerId);
        if (peer != nullptr) {
          std::lock_guard<std::recursive_mutex> guard(peer->mutex);
          if (isEphemeral) {
            peer->flags = (peer_flag_t) (peer->flags | PEER_FLAG_EPHEMERAL);
          }
        }

        return peer;
      }

      auto peer = new Peer(peerType, peerId, isEphemeral);
      std::lock_guard<std::recursive_mutex> guard(peersMutex);
      peers[peer->id] = peer;
      return peer;
    }

    /**
     * Private `Peer` class constructor
     */
    Peer (peer_type_t peerType, uint64_t peerId, bool isEphemeral) {
      this->id = peerId;
      this->type = peerType;

      if (isEphemeral) {
        this->flags = (peer_flag_t) (this->flags | PEER_FLAG_EPHEMERAL);
      }

      this->init();
    }

    ~Peer () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      Peer::remove(this->id, true); // auto close
    }

    int init () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      auto loop = getEventLoop();
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

    int initRemotePeerInfo () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      if (this->type == PEER_TYPE_UDP) {
        this->remote.init((uv_udp_t *) &this->handle);
      } else if (this->type == PEER_TYPE_TCP) {
        this->remote.init((uv_tcp_t *) &this->handle);
      }
      return this->remote.err;
    }

    int initLocalPeerInfo () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      if (this->type == PEER_TYPE_UDP) {
        this->local.init((uv_udp_t *) &this->handle);
      } else if (this->type == PEER_TYPE_TCP) {
        this->local.init((uv_tcp_t *) &this->handle);
      }
      return this->local.err;
    }

    void setState (peer_state_t value) {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      this->state = (peer_state_t) (this->state | value);
    }

    void removeState (peer_state_t value) {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      this->state = (peer_state_t) (this->state & ~value);
    }

    bool hasState (peer_state_t value) {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return (value & this->state) == value;
    }

    const RemotePeerInfo* getRemotePeerInfo () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return &this->remote;
    }

    const LocalPeerInfo* getLocalPeerInfo () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return &this->local;
    }

    bool isUDP () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return this->type == PEER_TYPE_UDP;
    }

    bool isTCP () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return this->type == PEER_TYPE_TCP;
    }

    bool isEphemeral () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return (PEER_FLAG_EPHEMERAL & this->flags) == PEER_FLAG_EPHEMERAL;
    }

    bool isBound () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return (
        (this->isUDP() && this->hasState(PEER_STATE_UDP_BOUND)) ||
        (this->isTCP() && this->hasState(PEER_STATE_TCP_BOUND))
      );
    }

    bool isActive () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return uv_is_active((const uv_handle_t *) &this->handle);
    }

    bool isClosing () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return uv_is_closing((const uv_handle_t *) &this->handle);
    }

    bool isClosed () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return this->hasState(PEER_STATE_CLOSED);
    }

    bool isConnected () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return (
        (this->isUDP() && this->hasState(PEER_STATE_UDP_CONNECTED)) ||
        (this->isTCP() && this->hasState(PEER_STATE_TCP_CONNECTED))
      );
    }

    bool isPaused () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      return (
        (this->isUDP() && this->hasState(PEER_STATE_UDP_PAUSED)) ||
        (this->isTCP() && this->hasState(PEER_STATE_TCP_PAUSED))
      );
    }

    int bind () {
      auto info = this->getLocalPeerInfo();

      if (info->err) {
        return info->err;
      }

      return this->bind(info->address, info->port, this->options.udp.reuseAddr);
    }

    int bind (std::string address, int port) {
      return this->bind(address, port, false);
    }

    int bind (std::string address, int port, bool reuseAddr) {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
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

        this->setState(PEER_STATE_UDP_BOUND);
      }

      if (this->isTCP()) {
        // @TODO: `bind()` + `listen()?`
      }

      return this->initLocalPeerInfo();
    }

    int rebind () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      int err = 0;

      if (this->isUDP()) {
        if ((err = this->recvstop())) {
          return err;
        }
      }

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

    int connect (std::string address, int port) {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      auto sockaddr = (struct sockaddr*) &this->addr;
      int err = 0;

      if ((err = uv_ip4_addr((char *) address.c_str(), port, &this->addr))) {
        return err;
      }

      if (this->isUDP()) {
        if ((err = uv_udp_connect((uv_udp_t *) &this->handle, sockaddr))) {
          return err;
        }

        this->setState(PEER_STATE_UDP_CONNECTED);
      }

      return this->initRemotePeerInfo();
    }

    void send (std::string seq, char *buf, int len, int port, String address, Cb cb) {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      auto loop = getEventLoop();
      auto ctx = new PeerRequestContext(seq, cb);
      int err = 0;

      struct sockaddr *sockaddr = nullptr;

      if (!this->isUDP()) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.send",
          "err": {
            "id": "$S",
            "message": "Invalid UDP state"
          }
        })MSG", std::to_string(this->id));

        return ctx->end(msg);
      }

      if (!this->isConnected()) {
        sockaddr = (struct sockaddr *) &this->addr;
        err = uv_ip4_addr((char *) address.c_str(), port, &this->addr);

        if (err) {
          auto msg = SSC::format(R"MSG({
            "source": "udp.send",
            "err": {
              "id": "$S",
              "message": "$S"
            }
          })MSG", std::to_string(id), std::string(uv_strerror(err)));

          return ctx->end(msg);
        }
      }

      auto buffer = uv_buf_init(buf, len);
      auto req = new uv_udp_send_t;

      req->data = (void *) ctx;
      ctx->peer = this;

      err = uv_udp_send(req, (uv_udp_t *) &this->handle, &buffer, 1, sockaddr, [](uv_udp_send_t *req, int status) {
        auto ctx = reinterpret_cast<PeerRequestContext*>(req->data);
        auto peer = ctx->peer;
        std::string msg = "";

        if (status < 0) {
          msg = SSC::format(R"MSG({
            "source": "udp.send",
            "err": {
              "id": "$S",
              "message": "$S"
            }
          })MSG", std::to_string(peer->id), std::string(uv_strerror(status)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "udp.send",
            "data": {
              "id": "$S",
              "status": "$i"
            }
          })MSG", std::to_string(peer->id), status);
        }

        delete req;

        if (peer->isEphemeral()) {
          peer->close();
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.send",
          "err": {
            "id": "$S",
            "message": "Write error: $S"
          }
        })MSG", std::to_string(id), std::string(uv_strerror(err)));

        delete req;

        if (isEphemeral()) {
          this->close();
        }

        ctx->end(msg);
      }
    }

    int recvstart () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);

      if (this->recv != nullptr) {
        return this->recvstart(this->recv);
      }

      return UV_EINVAL;
    }

    int recvstart (Cb onrecv) {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      if (this->hasState(PEER_STATE_UDP_RECV_STARTED)) {
        return UV_EALREADY;
      }

      this->recv = onrecv;
      this->setState(PEER_STATE_UDP_RECV_STARTED);

      auto allocate = [](uv_handle_t *handle, size_t size, uv_buf_t *buf) {
        if (size > 0) {
          buf->base = (char *) new char[size]{0};
          buf->len = size;
          memset(buf->base, 0, buf->len);
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

        if (nread <= 0) {
          if (buf && buf->base) {
            free(buf->base);
          }
        }

        if (nread == UV_ENOTCONN) {
          peer->recvstop();
          return;
        }

        if (nread == UV_EOF) {
          auto msg = SSC::format(R"MSG({
            "source": "udp.readStart",
            "data": {
              "id": "$S",
              "EOF": true
            }
          })MSG", std::to_string(peer->id));

          peer->recv("-1", msg, Post{});
        } else if (nread > 0) {
          int port;
          char ipbuf[17];
          parseAddress((struct sockaddr *) addr, &port, ipbuf);
          String ip(ipbuf);

          auto headers = SSC::format(R"MSG(
            content-type: application/octet-stream
            content-length: $i
          )MSG", (int) nread);

          Post post = {0};
          post.id = SSC::rand64();
          post.body = buf->base;
          post.length = (int) nread;
          post.headers = headers;
          post.bodyNeedsFree = true;

          auto msg = SSC::format(R"MSG({
            "source": "udp.readStart",
            "data": {
              "id": "$S",
              "bytes": $S,
              "port": $i,
              "address": "$S"
            }
          })MSG",
            std::to_string(peer->id),
            std::to_string(post.length),
            port,
            ip
          );

          peer->recv("-1", msg, post);
        }
      };

      std::lock_guard<std::recursive_mutex> lock(loopMutex);
      return uv_udp_recv_start((uv_udp_t *) &this->handle, allocate, receive);
    }

    int recvstop () {
      int rc = 0;

      if (this->hasState(PEER_STATE_UDP_RECV_STARTED)) {
        this->removeState(PEER_STATE_UDP_RECV_STARTED);
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        std::lock_guard<std::recursive_mutex> lock(loopMutex);
        rc = uv_udp_recv_stop((uv_udp_t *) &this->handle);
      }

      return rc;
    }

    int resume () {
      int rc = 0;

      if ((rc = this->init())) {
        return rc;
      }

      if ((rc = this->rebind())) {
        return rc;
      }

      this->removeState(PEER_STATE_UDP_PAUSED);

      return rc;
    }

    int pause () {
      int rc = 0;

      if ((rc = this->recvstop())) {
        return rc;
      }

      if (!this->isPaused() && !this->isClosing()) {
        this->setState(PEER_STATE_UDP_PAUSED);
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        uv_close((uv_handle_t *) &this->handle, nullptr);
      }

      return rc;
    }

    void close () {
      std::lock_guard<std::recursive_mutex> guard(this->mutex);
      if (!this->isActive()) {
        Peer::remove(this->id);
        return;
      }

      if (this->isClosed()) {
        Peer::remove(this->id);
        return;
      }

      if (this->isClosing()) {
        return;
      }

      if (this->type == PEER_TYPE_UDP) {
        // reset state and set to CLOSED
        uv_close((uv_handle_t*) &this->handle, [](uv_handle_t *handle) {
          auto peer = (Peer *) handle->data;
          if (peer != nullptr) {
            peer->removeState(PEER_STATE_UDP_BOUND);
            Peer::remove(peer->id);
            delete peer;
          }
        });
      }
    }
  };

  struct DescriptorRequestContext {
    uint64_t id;
    String seq = "";
    Descriptor *desc = nullptr;
    uv_fs_t req;
    uv_buf_t iov[16];
    // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
    uv_dirent_t dirents[256];
    uv_loop_t *loop;
    int offset = 0;
    int result = 0;
    Cb cb;

    DescriptorRequestContext () {}
    DescriptorRequestContext (Descriptor *desc)
      : DescriptorRequestContext(desc, "", nullptr) {}
    DescriptorRequestContext (String seq, Cb cb)
      : DescriptorRequestContext(nullptr, seq, cb) {}
    DescriptorRequestContext (Descriptor *desc, String seq, Cb cb) {
      this->id = SSC::rand64();
      this->cb = cb;
      this->seq = seq;
      this->desc = desc;
      this->loop = getEventLoop();
      this->req.data = (void *) this;
    }

    ~DescriptorRequestContext () {
      if (this->req.ptr) {
        delete [] (char *) this->req.ptr;
      }

      uv_fs_req_cleanup(&this->req);
    }

    void setBuffer (int index, int len, char *base) {
      this->iov[index].base = base;
      this->iov[index].len = len;
    }

    void freeBuffer (int index) {
      if (this->iov[index].base != nullptr) {
        delete [] (char *) this->iov[index].base;
        this->iov[index].base = nullptr;
      }

      this->iov[index].len = 0;
    }

    char* getBuffer (int index) {
      return this->iov[index].base;
    }

    size_t getBufferSize (int index) {
      return this->iov[index].len;
    }

    void end (String seq, String msg, Post post) {
      auto cb = this->cb;

      delete this;

      if (cb != nullptr) {
        cb(seq, msg, post);
      }
    }

    void end (String seq, String msg) {
      this->end(seq, msg, Post{});
    }

    void end (String msg, Post post) {
      this->end(this->seq, msg, post);
    }

    void end (String msg) {
      this->end(this->seq, msg, Post{});
    }
  };

#if defined(__linux__) && !defined(__ANDROID__)
  struct UVSource {
    GSource base; // should ALWAYS be first member
    gpointer tag;
  };

    // @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
  static GSourceFuncs loopSourceFunctions = {
    .prepare = [](GSource *source, gint *timeout) -> gboolean {
      if (!isLoopAlive() || !isLoopRunning) {
        return false;
      }

      *timeout = getEventLoopTimeout();
      return 0 == *timeout;
    },

    .dispatch = [](GSource *source, GSourceFunc callback, gpointer user_data) -> gboolean {
      std::lock_guard<std::recursive_mutex> lock(loopMutex);
      auto loop = getEventLoop();
      uv_run(loop, UV_RUN_NOWAIT);
      return G_SOURCE_CONTINUE;
    }
  };

#endif

  static void initEventLoop () {
    if (didLoopInit) {
      return;
    }

    didLoopInit = true;
    uv_loop_init(&eventLoop);

#if defined(__linux__) && !defined(__ANDROID__)
    GSource *source = g_source_new(&loopSourceFunctions, sizeof(UVSource));
    UVSource *uvSource = (UVSource *) source;
    uvSource->tag = g_source_add_unix_fd(
      source,
      uv_backend_fd(&eventLoop),
      (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
    );

    g_source_attach(source, nullptr);
#endif
  }

  static uv_loop_t* getEventLoop () {
    initEventLoop();
    return &eventLoop;
  }

  static int getEventLoopTimeout () {
    auto loop = getEventLoop();
    uv_update_time(loop);
    return uv_backend_timeout(loop);
  }

  static bool isLoopAlive () {
    return uv_loop_alive(getEventLoop());
  }

  static void stopEventLoop() {
    isLoopRunning = false;
    uv_stop(&eventLoop);
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
#if defined(__ANDROID__)
    if (eventLoopThread != nullptr) {
      if (eventLoopThread->joinable()) {
        eventLoopThread->join();
      }

      delete eventLoopThread;
      eventLoopThread = nullptr;
    }
#endif
  }

  static void sleepEventLoop (int64_t ms) {
    if (ms > 0) {
      auto timeout = getEventLoopTimeout();
      ms = timeout > ms ? timeout : ms;
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
  }

  static void sleepEventLoop () {
    sleepEventLoop(getEventLoopTimeout());
  }

  static void dispatchEventLoop (EventLoopDispatchContext::Callback dispatch) {
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    auto loop = getEventLoop();
    auto ctx = new EventLoopDispatchContext(dispatch);

    uv_async_init(loop, &ctx->async, [](uv_async_t *async) {
      auto self = (EventLoopDispatchContext *) async->data;
      auto loop = getEventLoop();
      self->dispatch();
      self->close();
    });

    uv_async_send(&ctx->async);
    runEventLoop();
  }

  static void pollEventLoop () {
    auto loop = getEventLoop();

    while (isLoopRunning) {
      sleepEventLoop(EVENT_LOOP_POLL_TIMEOUT);

      do {
        auto timeout = getEventLoopTimeout();
        if (timeout >= 0) {
          uv_run(loop, UV_RUN_DEFAULT);
        } else {
          do {
            std::lock_guard<std::recursive_mutex> lock(loopMutex);
            if (!isLoopRunning || !uv_run(loop, UV_RUN_NOWAIT)) {
              break;
            }
          } while (isLoopAlive() && getEventLoopTimeout() == -1);
        }
      } while (isLoopRunning && isLoopAlive());
    }

    isLoopRunning = false;
  }

  static int runEventLoop () {
    if (isLoopRunning) {
      return 0;
    }

    isLoopRunning = true;

    int rc = 0;

    dispatchEventLoop([]() {
      initTimers();
      startTimers();
    });

#if defined(__APPLE__)
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    dispatch_async(eventLoopQueue, ^{ pollEventLoop(); });
#elif defined(__linux__) && !defined(__ANDROID__) // linux desktop
    // the linux implementation uses glib sources for polled event sourcing
    // to actually execute our loop in tandem with glib's event loop
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    rc = uv_run(getEventLoop(), UV_RUN_NOWAIT);
#else
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    if (eventLoopThread != nullptr) {
      if (eventLoopThread->joinable()) {
        eventLoopThread->join();
      }

      delete eventLoopThread;
      eventLoopThread = nullptr;
    }

    eventLoopThread = new std::thread(&pollEventLoop);
#endif

    return rc;
  }

  static void initTimers () {
    if (didTimersInit) {
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(timersMutex);

    auto loop = getEventLoop();

    std::vector<Timer *> timersToInit = {
      &timers.releaseWeakDescriptors
    };

    for (const auto& timer : timersToInit) {
      uv_timer_init(loop, &timer->handle);
      timer->handle.data = (void *) &timers.core;
    }

    didTimersInit = true;
  }

  static void startTimers () {
    std::lock_guard<std::recursive_mutex> guard(timersMutex);

    std::vector<Timer *> timersToStart = {
      &timers.releaseWeakDescriptors
    };

    for (const auto &timer : timersToStart) {
      if (timer->started) {
        uv_timer_again(&timer->handle);
      } else {
        timer->started = 0 == uv_timer_start(
          &timer->handle,
          timer->invoke,
          timer->timeout,
          !timer->repeated
            ? 0
            : timer->interval > 0
              ? timer->interval
              : timer->timeout
        );
      }
    }

    didTimersStart = false;
  }

  static void stopTimers () {
    if (didTimersStart == false) {
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(timersMutex);

    std::vector<Timer *> timersToStop = {
      &timers.releaseWeakDescriptors
    };

    for (const auto& timer : timersToStop) {
      if (timer->started) {
        uv_timer_stop(&timer->handle);
      }
    }

    didTimersStart = false;
  }

  typedef struct {
    uv_write_t req;
    uv_buf_t buf;
  } write_req_t;

  void Core::fsRetainOpenDescriptor (String seq, uint64_t id, Cb cb) const {
    auto desc = Descriptor::get(id);
    auto ctx = new DescriptorRequestContext(seq, cb);
    std::string msg;

    if (desc == nullptr) {
      msg = SSC::format(R"MSG({
        "source": "fs.retainOpenDescriptor",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));
    } else {
      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      desc->retained = true;
      auto msg = SSC::format(R"MSG({
        "source": "fs.retainOpenDescriptor",
        "data": {
          "id": "$S"
        }
      })MSG", std::to_string(desc->id));
    }

    ctx->end(msg);
  }

  void Core::handleEvent (String seq, String event, String data, Cb cb) const {
    // init page
    if (event == "domcontentloaded") {
      std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);

      for (auto const &tuple : descriptors) {
        auto desc = tuple.second;
        if (desc != nullptr) {
          std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
          desc->stale = true;
        } else {
          descriptors.erase(tuple.first);
        }
      }
    }

    cb(seq, "{}", Post{});

    runEventLoop();
  }

  bool Core::hasTask (String id) {
    std::lock_guard<std::recursive_mutex> guard(tasksMutex);
    if (id.size() == 0) return false;
    return tasks->find(id) != tasks->end();
  }

  Task Core::getTask (String id) {
    std::lock_guard<std::recursive_mutex> guard(tasksMutex);
    if (tasks->find(id) == tasks->end()) return Task{};
    return tasks->at(id);
  }

  void Core::removeTask (String id) {
    std::lock_guard<std::recursive_mutex> guard(tasksMutex);
    if (tasks->find(id) == tasks->end()) return;
    tasks->erase(id);
  }

  void Core::putTask (String id, Task t) {
    std::lock_guard<std::recursive_mutex> guard(tasksMutex);
    tasks->insert_or_assign(id, t);
  }

  Post Core::getPost (uint64_t id) {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    if (posts->find(id) == posts->end()) return Post{};
    return posts->at(id);
  }

  bool Core::hasPost (uint64_t id) {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    return posts->find(id) != posts->end();
  }

  void Core::expirePosts () {
    std::vector<uint64_t> ids;
    auto now = std::chrono::system_clock::now()
      .time_since_epoch()
      .count();

    {
      std::lock_guard<std::recursive_mutex> guard(postsMutex);
      for (auto const &tuple : *posts) {
        auto id = tuple.first;
        auto post = tuple.second;

        if (post.ttl < now) {
          ids.push_back(id);
        }
      }
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }

  void Core::putPost (uint64_t id, Post p) {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    posts->insert_or_assign(id, p);
  }

  void Core::removePost (uint64_t id) {
    if (posts->find(id) == posts->end()) return;
    auto post = getPost(id);

    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    if (post.body && post.bodyNeedsFree) {
      delete [] post.body;
      post.bodyNeedsFree = false;
      post.body = nullptr;
    }

    posts->erase(id);
  }

  String Core::createPost (String seq, String params, Post post) {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);

    if (post.id == 0) {
      post.id = SSC::rand64();
    }

    post.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(32 * 1024)
    )
      .time_since_epoch()
      .count();

    String sid = std::to_string(post.id);

    String js(
      ";(() => {\n"
      "  const xhr = new XMLHttpRequest();\n"
      "  xhr.responseType = 'arraybuffer';\n"
      "  xhr.onload = e => {\n"
      "    let o = `" + params + "`;\n"
      "    let headers = `" + SSC::trim(post.headers) + "`\n"
      "      .trim().split(/[\\r\\n]+/).filter(Boolean);\n"
      "    try { o = JSON.parse(o) } catch (err) {\n"
      "      console.error(err.stack || err, o)\n"
      "    };\n"
      "    o.seq = `" + seq + "`;\n"
      "    const detail = {\n"
      "      data: xhr.response,\n"
      "      sid: '" + sid + "',\n"
      "      headers: Object.fromEntries(\n"
      "        headers.map(l => l.split(/\\s*:\\s*/))\n"
      "      ),\n"
      "      params: o\n"
      "    };\n"
      "    queueMicrotask(() => window._ipc.emit('data', detail));\n"
      "  };\n"
      "  xhr.open('GET', 'ipc://post?id=" + sid + "');\n"
      "  xhr.send();\n"
      "})();\n"
      "//# sourceURL=post.js"
    );

    posts->insert_or_assign(post.id, post);
    return js;
  }

  void Core::removeAllPosts () {
    std::vector<uint64_t> ids;
    auto now = std::chrono::system_clock::now()
      .time_since_epoch()
      .count();

    {
      std::lock_guard<std::recursive_mutex> guard(postsMutex);
      for (auto const &tuple : *posts) {
        auto id = tuple.first;
        ids.push_back(id);
      }
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }

  String Core::getFSConstants () const {
    auto constants = Core::getFSConstantsMap();
    auto count = constants.size();
    std::stringstream stream;

    stream << "{\"source\": \"fs.constants\",";
    stream << "\"data\": {";

    for (auto const &tuple : constants) {
      auto key = tuple.first;
      auto value = tuple.second;

      stream << "\"" << key << "\":" << value;

      if (--count > 0) {
        stream << ",";
      }
    }

    stream << "}";
    stream << "}";

    return stream.str();
  }

  void Core::fsAccess (String seq, String path, int mode, Cb cb) const {
    dispatchEventLoop([=]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_access(ctx->loop, &ctx->req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.access",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.access",
            "data": {
              "mode": $S
            }
          })MSG", std::to_string(req->flags));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.access",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG", std::to_string(err), String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsChmod (String seq, String path, int mode, Cb cb) const {
    dispatchEventLoop([=]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_chmod(ctx->loop, &ctx->req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.chmod",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.chmod",
            "data": {
              "mode": "$S"
            }
          })MSG", std::to_string(req->flags));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.chmod",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG", std::to_string(err), String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsOpen (String seq, uint64_t id, String path, int flags, int mode, Cb cb) const {
    dispatchEventLoop([=]() {
      auto filename = path.c_str();
      auto desc = new Descriptor(id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      auto err = uv_fs_open(ctx->loop, &ctx->req, filename, flags, mode, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.open",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));

          delete desc;
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.open",
            "data": {
              "id": "$S",
              "fd": $S
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result));

          // insert into `descriptors` map
          std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
          desc->fd = (int) req->result;
          descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.open",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        delete desc;
        ctx->end(msg);
      }
    });
  }

  void Core::fsOpendir(String seq, uint64_t id, String path, Cb cb) const {
    dispatchEventLoop([=]() {
      auto filename = path.c_str();
      auto desc =  new Descriptor(id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      auto err = uv_fs_opendir(ctx->loop, &ctx->req, filename, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.opendir",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));

          delete desc;
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.opendir",
            "data": {
              "id": "$S"
            }
          })MSG",
          std::to_string(desc->id));

          // insert into `descriptors` map
          std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
          desc->dir = (uv_dir_t *) req->ptr;
          descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.opendir",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        delete desc;
        ctx->end(msg);
      }
    });
  }

  void Core::fsReaddir (String seq, uint64_t id, size_t nentries, Cb cb) const {
    dispatchEventLoop([=]() {
      auto desc = Descriptor::get(id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      if (desc == nullptr) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.readdir",
          "err": {
            "id": "$S",
            "code": "ENOTOPEN",
            "type": "NotFoundError",
            "message": "No directory descriptor found with that id"
          }
        })MSG", std::to_string(id));

        ctx->end(msg);
        return;
      }

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);

      if (desc->dir == nullptr) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.readdir",
          "err": {
            "id": "$S",
            "code": "ENOTOPEN",
            "message": "Directory descriptor with that id is not open"
          }
        })MSG", std::to_string(id));

        ctx->end(msg);
        return;
      }

      desc->dir->dirents = ctx->dirents;
      desc->dir->nentries = nentries;

      auto err = uv_fs_readdir(ctx->loop, &ctx->req, desc->dir, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.readdir",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          Stringstream entries;
          entries << "[";

          for (int i = 0; i < req->result; ++i) {
            entries << "{";
            entries << "\"type\":" << std::to_string(desc->dir->dirents[i].type) << ",";
            entries << "\"name\":" << "\"" << desc->dir->dirents[i].name << "\"";
            entries << "}";

            if (i + 1 < req->result) {
              entries << ", ";
            }
          }

          entries << "]";

          msg = SSC::format(R"MSG({
            "source": "fs.readdir",
            "data": {
              "id": "$S",
              "entries": $S
            }
          })MSG",
          std::to_string(desc->id),
          entries.str());
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.readdir",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsClose (String seq, uint64_t id, Cb cb) const {
    dispatchEventLoop([=]() {
      auto desc = Descriptor::get(id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      if (desc == nullptr) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.close",
          "err": {
            "id": "$S",
            "code": "ENOTOPEN",
            "type": "NotFoundError",
            "message": "No file descriptor found with that id"
          }
        })MSG", std::to_string(id));

        ctx->end(msg);
        return;
      }

      auto err = uv_fs_close(ctx->loop, &ctx->req, desc->fd, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.close",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.close",
            "data": {
              "id": "$S",
              "fd": $S
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(desc->fd));

          Descriptor::remove(desc->id);
          delete desc;
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.close",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsClosedir (String seq, uint64_t id, Cb cb) const {
    dispatchEventLoop([=]() {
      auto desc = Descriptor::get(id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      if (desc == nullptr) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.closedir",
          "err": {
            "id": "$S",
            "code": "ENOTOPEN",
            "type": "NotFoundError",
            "message": "No directory descriptor found with that id"
          }
        })MSG", std::to_string(id));

        ctx->end(msg);
        return;
      }

      auto err = uv_fs_closedir(ctx->loop, &ctx->req, desc->dir, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.closedir",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.closedir",
            "data": {
              "id": "$S"
            }
          })MSG", std::to_string(desc->id));

          Descriptor::remove(desc->id);
          delete desc;
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.closedir",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsCloseOpenDescriptor (String seq, uint64_t id, Cb cb) const {
    auto desc = Descriptor::get(id);

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.closeOpenDescriptor",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No descriptor found with that id"
        }
      })MSG", std::to_string(id));

      cb(seq, msg, Post{});
      return;
    }

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);

    if (desc->dir != nullptr) {
      this->fsClosedir(seq, id, cb);
    } else if (desc->fd > 0){
      this->fsClose(seq, id, cb);
    }
  }

  void Core::fsCloseOpenDescriptors (String seq, Cb cb) const {
    return this->fsCloseOpenDescriptors(seq, false, cb);
  }

  void Core::fsCloseOpenDescriptors (String seq, bool preserveRetained, Cb cb) const {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);

    std::vector<uint64_t> ids;
    std::string msg = "";
    int pending = descriptors.size();
    int queued = 0;

    for (auto const &tuple : descriptors) {
      ids.push_back(tuple.first);
    }

    for (auto const id : ids) {
      auto desc = descriptors[id];
      pending--;

      if (desc == nullptr) {
        descriptors.erase(desc->id);
        continue;
      }

      if (preserveRetained  && desc->retained) {
        continue;
      }

      if (desc->dir != nullptr) {
        queued++;
        this->fsClosedir(seq, desc->id, [pending, cb](auto seq, auto msg, auto post) {
          if (pending == 0) {
            cb(seq, msg, post);
          }
        });
      } else if (desc->fd > 0) {
        queued++;
        this->fsClose(seq, desc->id, [pending, cb](auto seq, auto msg, auto post) {
          if (pending == 0) {
            cb(seq, msg, post);
          }
        });
      }
    }

    if (queued == 0) {
      cb(seq, msg, Post{});
    }
  }

  void Core::fsRead (String seq, uint64_t id, int len, int offset, Cb cb) const {
    dispatchEventLoop([=]() {
      auto desc = Descriptor::get(id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      if (desc == nullptr) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.read",
          "err": {
            "id": "$S",
            "code": "ENOTOPEN",
            "type": "NotFoundError",
            "message": "No file descriptor found with that id"
          }
        })MSG", std::to_string(id));

        ctx->end(msg);
        return;
      }

      auto buf = new char[len]{0};
      ctx->setBuffer(0, len, buf);

      auto err = uv_fs_read(ctx->loop, &ctx->req, desc->fd, ctx->iov, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<DescriptorRequestContext*>(req->data);
        auto desc = ctx->desc;
        std::string msg;
        Post post = {0};

        std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.read",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));

          auto buf = ctx->getBuffer(0);
          if (buf != nullptr) {
            delete [] buf;
          }
        } else {
          msg = "{}";
          post.id = SSC::rand64();
          post.body = ctx->getBuffer(0);
          post.length = (int) req->result;
          post.bodyNeedsFree = true;
        }

        ctx->end(msg, post);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.read",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(id), String(uv_strerror(err)));

        delete [] buf;
        ctx->end(msg);
      }
    });
  }

  void Core::fsWrite (String seq, uint64_t id, String data, int64_t offset, Cb cb) const {
    auto size = data.size();
    auto bytes = new char[size]{0};

    memcpy(bytes, data.data(), size);
    dispatchEventLoop([=]() {
      auto desc = Descriptor::get(id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      if (desc == nullptr) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.write",
          "err": {
            "id": "$S",
            "code": "ENOTOPEN",
            "type": "NotFoundError",
            "message": "No file descriptor found with that id"
          }
        })MSG", std::to_string(id));

        delete [] bytes;
        ctx->end(msg);
        return;
      }

      ctx->setBuffer(0, size, bytes);

      auto err = uv_fs_write(ctx->loop, &ctx->req, desc->fd, ctx->iov, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<DescriptorRequestContext*>(req->data);
        auto desc = ctx->desc;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.write",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.write",
            "data": {
              "id": "$S",
              "result": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result));
        }

        auto bytes = ctx->getBuffer(0);
        if (bytes != nullptr) {
          delete [] bytes;
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.write",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        delete [] bytes;
        ctx->end(msg);
      }
    });
  }

  void Core::fsStat (String seq, String path, Cb cb) const {
    dispatchEventLoop([=]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_stat(ctx->loop, &ctx->req, filename, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.stat",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
            auto stats = uv_fs_get_statbuf(req);
            msg = SSC::format(R"MSG({
              "source": "fs.stat",
              "data": {
                "st_dev": "$S",
                "st_mode": "$S",
                "st_nlink": "$S",
                "st_uid": "$S",
                "st_gid": "$S",
                "st_rdev": "$S",
                "st_ino": "$S",
                "st_size": "$S",
                "st_blksize": "$S",
                "st_blocks": "$S",
                "st_flags": "$S",
                "st_gen": "$S",
                "st_atim": { "tv_sec": "$S", "tv_nsec": "$S" },
                "st_mtim": { "tv_sec": "$S", "tv_nsec": "$S" },
                "st_ctim": { "tv_sec": "$S", "tv_nsec": "$S" },
                "st_birthtim": { "tv_sec": "$S", "tv_nsec": "$S" }
              }
            })MSG",
            std::to_string(stats->st_dev),
            std::to_string(stats->st_mode),
            std::to_string(stats->st_nlink),
            std::to_string(stats->st_uid),
            std::to_string(stats->st_gid),
            std::to_string(stats->st_rdev),
            std::to_string(stats->st_ino),
            std::to_string(stats->st_size),
            std::to_string(stats->st_blksize),
            std::to_string(stats->st_blocks),
            std::to_string(stats->st_flags),
            std::to_string(stats->st_gen),
            std::to_string(stats->st_atim.tv_sec),
            std::to_string(stats->st_atim.tv_nsec),
            std::to_string(stats->st_mtim.tv_sec),
            std::to_string(stats->st_mtim.tv_nsec),
            std::to_string(stats->st_ctim.tv_sec),
            std::to_string(stats->st_ctim.tv_nsec),
            std::to_string(stats->st_birthtim.tv_sec),
            std::to_string(stats->st_birthtim.tv_nsec)
          );
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.stat",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsFStat (String seq, uint64_t id, Cb cb) const {
    dispatchEventLoop([=]() {
      auto desc = Descriptor::get(id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      if (desc == nullptr) {
        auto msg = SSC::format(R"MSG({
          "err": {
          "source": "fs.read",
            "id": "$S",
            "code": "ENOTOPEN",
            "type": "NotFoundError",
            "message": "No file descriptor found with that id"
          }
        })MSG", std::to_string(id));

        ctx->end(msg);
        return;
      }

      auto err = uv_fs_fstat(ctx->loop, &ctx->req, desc->fd, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.fstat",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          auto stats = uv_fs_get_statbuf(req);
          msg = SSC::trim(SSC::format(R"MSG({
            "source": "fs.fstat",
            "data": {
              "id": "$S",
              "st_dev": "$S",
              "st_mode": "$S",
              "st_nlink": "$S",
              "st_uid": "$S",
              "st_gid": "$S",
              "st_rdev": "$S",
              "st_ino": "$S",
              "st_size": "$S",
              "st_blksize": "$S",
              "st_blocks": "$S",
              "st_flags": "$S",
              "st_gen": "$S",
              "st_atim": { "tv_sec": "$S", "tv_nsec": "$S" },
              "st_mtim": { "tv_sec": "$S", "tv_nsec": "$S" },
              "st_ctim": { "tv_sec": "$S", "tv_nsec": "$S" },
              "st_birthtim": { "tv_sec": "$S", "tv_nsec": "$S" }
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(stats->st_dev),
          std::to_string(stats->st_mode),
          std::to_string(stats->st_nlink),
          std::to_string(stats->st_uid),
          std::to_string(stats->st_gid),
          std::to_string(stats->st_rdev),
          std::to_string(stats->st_ino),
          std::to_string(stats->st_size),
          std::to_string(stats->st_blksize),
          std::to_string(stats->st_blocks),
          std::to_string(stats->st_flags),
          std::to_string(stats->st_gen),
          std::to_string(stats->st_atim.tv_sec),
          std::to_string(stats->st_atim.tv_nsec),
          std::to_string(stats->st_mtim.tv_sec),
          std::to_string(stats->st_mtim.tv_nsec),
          std::to_string(stats->st_ctim.tv_sec),
          std::to_string(stats->st_ctim.tv_nsec),
          std::to_string(stats->st_birthtim.tv_sec),
          std::to_string(stats->st_birthtim.tv_nsec)));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.fstat",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsGetOpenDescriptors (String seq, Cb cb) const {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);

    std::string msg = "";
    int pending = descriptors.size();

    msg += "{ \"source\": \"fs.getOpenDescriptors\", \"data\": [";
    for (auto const &tuple : descriptors) {
      auto desc = tuple.second;
      if (!desc) {
        continue;
      }

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);

      if (desc->stale || !desc->retained) {
        continue;
      }

      msg += SSC::format(
        R"MSG({ "id": "$S", "fd": "$S", "type": "$S" })MSG",
        std::to_string(desc->id),
        std::to_string(desc->dir ? desc->id : desc->fd),
        desc->dir ? "directory" : "file"
      );

      if (--pending > 0) {
        msg += ", ";
      }
    }
    msg += "] }";

    cb(seq, msg, Post{});
  }

  void Core::fsUnlink (String seq, String path, Cb cb) const {
    dispatchEventLoop([=]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_unlink(ctx->loop, &ctx->req, filename, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.unlink",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.unlink",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.unlink",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsRename (String seq, String pathA, String pathB, Cb cb) const {
    dispatchEventLoop([=]() {
      auto ctx = new DescriptorRequestContext(seq, cb);
      auto src = pathA.c_str();
      auto dst = pathB.c_str();

      auto err = uv_fs_rename(ctx->loop, &ctx->req, src, dst, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.rename",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror((int)req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.rename",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.rename",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsCopyFile (String seq, String pathA, String pathB, int flags, Cb cb) const {
    dispatchEventLoop([=]() {
      auto ctx = new DescriptorRequestContext(seq, cb);
      auto src = pathA.c_str();
      auto dst = pathB.c_str();

      auto err = uv_fs_copyfile(ctx->loop, &ctx->req, src, dst, flags, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.copyFile",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror((int)req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.copyFile",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.copyFile",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsRmdir (String seq, String path, Cb cb) const {
    dispatchEventLoop([=]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_rmdir(ctx->loop, &ctx->req, filename, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.rmdir",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.rmdir",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.rmdir",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsMkdir (String seq, String path, int mode, Cb cb) const {
    dispatchEventLoop([=]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_mkdir(ctx->loop, &ctx->req, filename, mode, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        std::string msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.mkdir",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.mkdir",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.mkdir",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::sendBufferSize (String seq, uint64_t peerId, int size, Cb cb) const {
    auto peer = Peer::get(peerId);

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.sendBufferSize",
        "err": {
          "id": "$S",
          "method": "sendBufferSize",
          "message": "No handle with that id"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(peer->mutex);
    uv_handle_t* handle;

    if (peer->type == PEER_TYPE_UDP) {
      handle = (uv_handle_t*) &peer->handle;
    } else {
      auto msg = SSC::format(R"MSG({
        "source": "fs.sendBufferSize",
        "err": {
          "message": "Handle is not valid"
        }
      })MSG");

      cb(seq, msg, Post{});
      return;
    }

    int sz = size;
    int rSize = uv_send_buffer_size(handle, &sz);

    auto msg = SSC::format(R"MSG({
      "source": "fs.sendBufferSize",
      "data": {
        "id": "$S",
        "method": "Cb",
        "size": $i
      }
    })MSG", std::to_string(peerId), rSize);

    cb(seq, msg, Post{});
    return;
  }

  void Core::recvBufferSize (String seq, uint64_t peerId, int size, Cb cb) const {
    auto peer = Peer::get(peerId);

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.recvBufferSize",
        "err": {
          "id": "$S",
          "method": "Cb",
          "message": "Not connected"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(peer->mutex);
    auto handle = (uv_handle_t*) &peer->handle;

    auto sz = size;
    auto rSize = uv_recv_buffer_size(handle, &sz);

    auto msg = SSC::format(R"MSG({
      "source": "fs.recvBufferSize",
      "data": {
        "id": "$S",
        "method": "Cb",
        "size": $i
      }
    })MSG", std::to_string(peerId), rSize);

    cb(seq, msg, Post{});
    return;
  }

  void Core::readStop (String seq, uint64_t peerId, Cb cb) const {
    auto peer = Peer::get(peerId);

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.readStop",
        "err": {
          "id": "$S",
          "message": "No connection with specified id"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(peer->mutex);
    auto stream = (uv_stream_t *) &peer->handle;
    auto result = uv_read_stop(stream);
    auto msg = SSC::format(R"MSG({
      "source": "fs.readStop",
      "data": $i
    })MSG", result);

    cb(seq, msg, Post{});
  }

  void Core::close (String seq, uint64_t peerId, Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "close",
        "err": {
          "id": "$S",
          "code": "NOT_FOUND_ERR",
          "type": "NotFoundError",
          "message": "No connection with specified id"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = Peer::get(peerId);

    if (peer->isClosed() || peer->isClosing()) {
      auto msg = SSC::format(R"MSG({
        "source": "close",
        "err": {
          "id": "$S",
          "code": "ERR_SOCKET_DGRAM_NOT_RUNNING",
          "type": "InternalError",
          "message": "The socket has already been closed"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    dispatchEventLoop([=]() {
      peer->close();
      cb(seq, "{}", Post{});
    });
  }

  void Core::shutdown (String seq, uint64_t peerId, Cb cb) const {
    auto ctx = new PeerRequestContext(seq, cb);

    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(
        R"MSG({
          "source": "shutdown",
          "err": {
            "id": "$S",
            "message": "No connection with specified id"
          }
        })MSG",
        std::to_string(peerId)
      );

      ctx->end(msg);
      return;
    }

    auto req = new uv_shutdown_t;
    auto peer = Peer::get(peerId);
    auto handle = (uv_handle_t *) &peer->handle;

    ctx->peer = peer;
    req->data = ctx;

    auto err = uv_shutdown(req, (uv_stream_t*) handle, [](uv_shutdown_t *req, int status) {
      auto ctx = reinterpret_cast<PeerRequestContext *>(req->data);
      auto peer = ctx->peer;

      std::string msg = "";

      if (status < 0) {
        msg = SSC::format(R"MSG({
          "source": "shutdown",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peer->id), std::string(uv_strerror(status)));
      } else {
        msg = SSC::format(R"MSG({
          "source": "shutdown",
          "data": {
            "id": "$S",
            "status": "$i"
          }
        })MSG", std::to_string(peer->id), status);

        Peer::remove(peer->id, true);
      }

      ctx->end(msg);

      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "source": "shutdown",
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      ctx->end(msg);
      return;
    }

    runEventLoop();
  }

  void Core::udpBind (String seq, uint64_t peerId, String address, int port, bool reuseAddr, Cb cb) const {
    if (Peer::exists(peerId) && Peer::get(peerId)->isBound()) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.bind",
        "err": {
          "id": "$S",
          "code": "ERR_SOCKET_ALREADY_BOUND",
          "message": "Socket is already bound"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    dispatchEventLoop([=]() {
      auto peer = Peer::create(PEER_TYPE_UDP, peerId);
      auto err = peer->bind(address, port, reuseAddr);

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.bind",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
        cb(seq, msg, Post{});
        return;
      }

      auto info = peer->getLocalPeerInfo();

      if (info->err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.bind",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peerId), std::string(uv_strerror(info->err)));
        cb(seq, msg, Post{});
        return;
      }

      auto msg = SSC::format(R"MSG({
        "source": "udp.bind",
        "data": {
          "event": "listening",
          "id": "$S",
          "address": "$S",
          "port": $i,
          "family": "$S"
        }
      })MSG", std::to_string(peerId), info->address, info->port, info->family);

      cb(seq, msg, Post{});
    });
  }

  void Core::udpConnect (String seq, uint64_t peerId, String address, int port, Cb cb) const {
    dispatchEventLoop([=]() {
      auto peer = Peer::create(PEER_TYPE_UDP, peerId);
      auto err = peer->connect(address, port);

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "udp.connect",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
        cb(seq, msg, Post{});
        return;
      }

      auto msg = SSC::format(R"MSG({
        "source": "udp.connect",
        "data": {
          "address": "$S",
          "port": $i,
          "id": "$S"
        }
      })MSG", std::string(address), port, std::to_string(peerId));

      cb(seq, msg, Post{});
    });
  }

  void Core::udpGetPeerName (String seq, uint64_t peerId, Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getPeerName",
        "err": {
          "id": "$S",
          "message": "no such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = Peer::get(peerId);
    auto info = peer->getRemotePeerInfo();

    if (info->err < 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getPeerName",
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(info->err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "source": "udp.getPeerName",
      "data": {
        "id": "$S",
        "address": "$S",
        "port": $i,
        "family": "$S"
      }
    })MSG", std::to_string(peerId), info->address, info->port, info->family);

    cb(seq, msg, Post{});
  }

  void Core::udpGetSockName (String seq, uint64_t peerId, Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getSockName",
        "err": {
          "id": "$S",
          "message": "no such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = Peer::get(peerId);
    auto info = peer->getLocalPeerInfo();

    if (info->err < 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getSockName",
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(info->err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(
      R"MSG({
        "source": "udp.getSockName",
        "data": {
          "id": "$S",
          "address": "$S",
          "port": $i,
          "family": "$S"
        }
      })MSG",
      std::to_string(peerId),
      info->address,
      info->port,
      info->family
    );

    cb(seq, msg, Post{});
  }

  void Core::udpGetState (String seq, uint64_t peerId,  Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getState",
        "err": {
          "id": "$S",
          "code": "NOT_FOUND_ERR",
          "message": "no such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = Peer::get(peerId);

    if (!peer->isUDP()) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.getState",
        "err": {
          "id": "$S",
          "code": "NOT_FOUND_ERR",
          "message": "no such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(
      R"MSG({
        "source": "udp.getState",
        "data": {
          "id": "$S",
          "type": "udp",
          "ephemeral": $S,
          "bound": $S,
          "active": $S,
          "closing": $S,
          "closed": $S,
          "connected": $S
        }
      })MSG",
      std::to_string(peerId),
      std::string(peer->isEphemeral() ? "true" : "false"),
      std::string(peer->isBound() ? "true" : "false"),
      std::string(peer->isActive() ? "true" : "false"),
      std::string(peer->isClosing() ? "true" : "false"),
      std::string(peer->isClosed() ? "true" : "false"),
      std::string(peer->isConnected() ? "true" : "false")
    );

    cb(seq, msg, Post{});
  }

  void Core::udpSend (String seq, uint64_t peerId, char* buf, int len, int port, String address, bool ephemeral, Cb cb) const {
    auto buffer = new char[len]{0};
    memcpy(buffer, buf, len);
    dispatchEventLoop([=]() {
      auto peer = Peer::create(PEER_TYPE_UDP, peerId, ephemeral);
      auto ctx = new PeerRequestContext(seq, cb);

      ctx->buf = buffer;
      ctx->bufsize = len;
      ctx->port = port;
      ctx->address = address;
      ctx->peer = peer;

      ctx->peer->send(ctx->seq, ctx->buf, ctx->bufsize, ctx->port, ctx->address, [ctx](auto seq, auto msg, auto post) {
        delete [] ctx->buf;
        ctx->cb(seq, msg, post);
      });
    });
  }

  void Core::udpReadStart (String seq, uint64_t peerId, Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.readStart",
        "err": {
          "id": "$S",
          "message": "no such handle"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    auto peer = Peer::get(peerId);

    if (peer->isActive()) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.readStart",
        "data": {
          "id": "$S"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    if (peer->isClosing()) {
      auto msg = SSC::format(R"MSG({
        "source": "udp.readStart",
        "err": {
          "id": "$S",
          "message": "handle is closing"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    if (peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      auto msg = SSC::format(R"MSG({
       "source": "udp.readStart",
        "err": {
          "id": "$S",
          "message": "handle is already listening"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    auto err = peer->recvstart(cb);

    // `UV_EALREADY || UV_EBUSY` means there is active IO on the underlying handle
    if (err < 0 && err != UV_EALREADY && err != UV_EBUSY) {
      auto msg = SSC::format(
        R"MSG({
          "source": "udp.readStart",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(peerId),
        std::string(uv_strerror(err))
      );

      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {
        "id": "$S"
      }
    })MSG", std::to_string(peerId));
    cb(seq, msg, Post{});
  }

  void Core::dnsLookup (String seq, String hostname, int family, Cb cb) const {
    dispatchEventLoop([=]() {
      auto ctx = new PeerRequestContext(seq, cb);
      auto loop = getEventLoop();

      struct addrinfo hints = {0};

      if (family == 6) {
        hints.ai_family = AF_INET6;
      } else if (family == 4) {
        hints.ai_family = AF_INET;
      } else {
        hints.ai_family = AF_UNSPEC;
      }

      hints.ai_socktype = 0; // `0` for any
      hints.ai_protocol = 0; // `0` for any

      uv_getaddrinfo_t* resolver = new uv_getaddrinfo_t;
      resolver->data = ctx;

      auto err = uv_getaddrinfo(loop, resolver, [](uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
        auto ctx = (PeerRequestContext*) resolver->data;

        if (status < 0) {
          auto msg = SSC::format(
            R"MSG({
              "source": "dns.lookup",
              "err": {
                "code": $S,
                "message": "$S"
              }
            })MSG",
            std::to_string(status),
            String(uv_strerror(status))
          );

          ctx->end(msg);
          return;
        }

        String address = "";

        if (res->ai_family == AF_INET) {
          char addr[17] = {'\0'};
          uv_ip4_name((struct sockaddr_in*)(res->ai_addr), addr, 16);
          address = String(addr, 17);
        } else if (res->ai_family == AF_INET6) {
          char addr[40] = {'\0'};
          uv_ip6_name((struct sockaddr_in6*)(res->ai_addr), addr, 39);
          address = String(addr, 40);
        }

        address = address.erase(address.find('\0'));

        auto family = res->ai_family == AF_INET
          ? 4
          : res->ai_family == AF_INET6
            ? 6
            : 0;

        auto msg = SSC::format(
          R"MSG({
            "source": "dns.lookup",
            "data": {
              "address": "$S",
              "family": $i
            }
          })MSG",
          address,
          family
        );

        uv_freeaddrinfo(res);
        ctx->end(msg);

      }, hostname.c_str(), nullptr, &hints);

      if (err < 0) {
        auto msg = SSC::format(
          R"MSG({
            "source": "dns.lookup",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(err),
          std::string(uv_strerror(err))
        );

        ctx->end(msg);
      }
    });
  }

  String Core::getNetworkInterfaces () const {
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *interface = nullptr;
    int success = getifaddrs(&interfaces);
    Stringstream value;
    Stringstream v4;
    Stringstream v6;

    if (success != 0) {
      return "{\"err\": {\"message\":\"unable to get interfaces\"}}";
    }

    interface = interfaces;
    v4 << "\"ipv4\":{";
    v6 << "\"ipv6\":{";

    while (interface != nullptr) {
      String address = "";
      const struct sockaddr_in *addr = (const struct sockaddr_in*)interface->ifa_addr;

      if (addr->sin_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in*)interface->ifa_addr;
        v4 << "\"" << interface->ifa_name << "\":\"" << SSC::addrToIPv4(addr) << "\",";
      }

      if (addr->sin_family == AF_INET6) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6*)interface->ifa_addr;
        v6 << "\"" << interface->ifa_name << "\":\"" << SSC::addrToIPv6(addr) << "\",";
      }

      interface = interface->ifa_next;
    }

    v4 << "\"local\":\"0.0.0.0\"}";
    v6 << "\"local\":\"::1\"}";

    getifaddrs(&interfaces);
    freeifaddrs(interfaces);

    value << "{\"data\":{" << v4.str() << "," << v6.str() << "}}";
    return value.str();
  }
} // SSC
#endif
