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

#if defined(__APPLE__)
	#import <Webkit/Webkit.h>
  using Task = id<WKURLSchemeTask>;
#else
  class Task {};
#endif

#define DEFAULT_BACKLOG 128

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

      void udpBind (String seq, uint64_t peerId, String address, int port, Cb cb) const;
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
  struct PeerRequest;
  struct DescriptorContext;

  static uv_loop_t defaultLoop = {0};

  static std::map<uint64_t, Peer*> peers;
  static std::map<uint64_t, PeerRequest*> requests;
  static std::map<uint64_t, DescriptorContext*> descriptors;

  static std::recursive_mutex peersMutex;
  static std::recursive_mutex tasksMutex;
  static std::recursive_mutex postsMutex;
  static std::recursive_mutex requestsMutex;
  static std::recursive_mutex descriptorsMutex;

  static std::recursive_mutex timersMutex;
  static std::recursive_mutex timersInitMutex;
  static std::recursive_mutex timersStartMutex;

  static std::atomic<bool> didLoopInit = false;
  static std::atomic<bool> didTimersInit = false;
  static std::atomic<bool> didTimersStart = false;

  static std::atomic<bool> isLoopRunning = false;
  static std::recursive_mutex loopMutex;

  struct DescriptorContext {
    uv_file fd = 0;
    uv_dir_t *dir = nullptr;
    // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
    uv_dirent_t dirents[256];
    String seq;
    Cb cb;
    uint64_t id;
    bool retained = false;
    bool stale = false;
    std::recursive_mutex mutex;
    void *data;
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
      .timeout = 256,
      .invoke = [](uv_timer_t *handle) {
        std::lock_guard<std::recursive_mutex> descriptorsGuard(descriptorsMutex);
        std::lock_guard<std::recursive_mutex> timersGuard(timersMutex);

        std::vector<uint64_t> ids;
        std::string msg = "";

        auto core = reinterpret_cast<Core *>(handle->data);

        for (auto const &tuple : SSC::descriptors) {
          ids.push_back(tuple.first);
        }

        for (auto const id : ids) {
          auto desc = SSC::descriptors[id];

          if (desc == nullptr) {
            SSC::descriptors.erase(id);
            continue;
          }

          if (desc->retained || !desc->stale) {
            continue;
          }

          // printf("Releasing weak descriptor %llu\n", desc->id);

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
            SSC::descriptors.erase(id);
            delete desc;
          }
        }
      }
    };
  };

  static Timers timers;

  static void initTimers ();
  static void startTimers ();
  static uv_loop_t* getDefaultLoop ();

#if defined(__linux__) && !defined(__ANDROID__)
  struct UVSource {
    GSource base; // should ALWAYS be first member
    gpointer tag;
  };

    // @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
  static GSourceFuncs loopSourceFunctions = {
    .prepare = [](GSource *source, gint *timeout) -> gboolean {
      auto loop = getDefaultLoop();
        uv_update_time(loop);

        if (!uv_loop_alive(loop)) {
          return false;
        }

        *timeout = uv_backend_timeout(loop);
        return 0 == *timeout;
      },

      .dispatch = [](GSource *source, GSourceFunc callback, gpointer user_data) -> gboolean {
        auto loop = getDefaultLoop();
        uv_run(loop, UV_RUN_NOWAIT);
        return G_SOURCE_CONTINUE;
      }
    };

#endif

  static void initLoop () {
    if (didLoopInit) {
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(loopMutex);

    if (didLoopInit) {
      return;
    }

    uv_loop_init(&defaultLoop);

#if defined(__linux__) && !defined(__ANDROID__)
    GSource *source = g_source_new(&loopSourceFunctions, sizeof(UVSource));
    UVSource *uvSource = (UVSource *) source;
    uvSource->tag = g_source_add_unix_fd(
      source,
      uv_backend_fd(&defaultLoop),
      (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
    );

    g_source_attach(source, nullptr);
#endif

    didLoopInit = true;
  }

  static uv_loop_t* getDefaultLoop () {
    initLoop();
    return &defaultLoop;
  }

  static void runDefaultLoop () {
    if (isLoopRunning) {
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(loopMutex);

    initTimers();
    startTimers();

    auto loop = getDefaultLoop();

    isLoopRunning = true;
#if defined(__linux__) && !defined(__ANDROID__)
    uv_run(loop, UV_RUN_NOWAIT);
#else
    while (uv_run(loop, UV_RUN_NOWAIT));
#endif
    isLoopRunning = false;
  }

  static void initTimers () {
    if (didTimersInit) {
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(timersInitMutex);

    auto loop = getDefaultLoop();

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
    std::lock_guard<std::recursive_mutex> guard(timersStartMutex);

    std::vector<Timer *> timersToStart = {
      &timers.releaseWeakDescriptors
    };

    for (const auto& timer : timersToStart) {
      if (!timer->started) {
        uv_timer_start(
          &timer->handle,
          timer->invoke,
          timer->timeout,
          timer->repeated
            ? timer->interval > 0 ? timer->interval : timer->timeout
            : 0
        );

        // set once
        timer->started = true;
      } else {
        uv_timer_again(&timer->handle);
      }
    }

    didTimersStart = false;
  }

  static void stopTimers () {
    if (didTimersStart == false) {
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(timersStartMutex);


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
    PEER_STATE_XXX = 1 << 1,
    PEER_STATE_CLOSED = 1 << 2,
    PEER_STATE_BOUND = 1 << 3,
    PEER_STATE_CONNECTED = 1 << 4
  } peer_state_t;

  struct PeerRequest {
    uint64_t id;
    String seq;
    Cb cb;
    Peer* peer;

    PeerRequest () {
      id = SSC::rand64();
    }

    PeerRequest (String s, Cb c)
      : PeerRequest(s, c, nullptr)
    {
      // noop
    }

    PeerRequest (String s, Cb c, Peer *p) {
      id = SSC::rand64();
      cb = c;
      seq = s;
      peer = p;
    }

    void end (String seq, String msg, Post post) const {
      cb(seq, msg, post);
      delete this;
    }

    void end (String seq, String msg) const {
      cb(seq, msg, Post{});
      delete this;
    }

    void end (String msg, Post post) const {
      cb(seq, msg, post);
      delete this;
    }

    void end (String msg) const {
      cb(seq, msg, Post{});
      delete this;
    }
  };

  struct LocalPeerInfo {
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    void init (uv_udp_t* socket) {
      struct sockaddr_storage addr;
      int namelen = sizeof(addr);

      address = "";
      port = 0;
      family = "";

      err = uv_udp_getsockname(socket, (struct sockaddr*) &addr, &namelen);

      if (err) {
        return;
      }

      if (addr.ss_family == AF_INET) {
        family = "IPv4";
        address = addrToIPv4((struct sockaddr_in*) &addr);
        port = (int)htons(((struct sockaddr_in*) &addr)->sin_port);
      } else {
        family = "IPv6";
        address = addrToIPv6((struct sockaddr_in6*) &addr);
        port = (int)htons(((struct sockaddr_in6*) &addr)->sin6_port);
      }
    }
  };

  struct RemotePeerInfo {
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    void init (uv_udp_t* socket) {
      struct sockaddr_storage addr;
      int namelen = sizeof(addr);

      address = "";
      port = 0;
      family = "";

      err = uv_udp_getpeername(socket, (struct sockaddr*) &addr, &namelen);

      if (err) {
        return;
      }

      if (addr.ss_family == AF_INET) {
        family = "IPv4";
        address = addrToIPv4((struct sockaddr_in*) &addr);
        port = (int)htons(((struct sockaddr_in*) &addr)->sin_port);
      } else {
        family = "IPv6";
        address = addrToIPv6((struct sockaddr_in6*) &addr);
        port = (int)htons(((struct sockaddr_in6*) &addr)->sin6_port);
      }
    }
  };

  /**
   * A generic structure for a bound or connected peer.
   */
  struct Peer {
    Cb cb;
    Cb onudpread;
    std::function<void(Peer *)> onclose;

    String seq = "";
    uint64_t id = 0;

    // peer state
    RemotePeerInfo remote;
    LocalPeerInfo local;
    peer_type_t type = PEER_TYPE_NONE;
    peer_flag_t flags = PEER_FLAG_NONE;
    peer_state_t state = PEER_STATE_NONE;

    // uv
    uv_udp_t udp;
    uv_stream_t* stream;

    struct sockaddr_in addr;
    std::recursive_mutex mutex;

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
    static void remove (uint64_t peerId) { return remove(peerId, false); }
    static void remove (uint64_t peerId, bool autoClose) {
      std::lock_guard<std::recursive_mutex> guard(peersMutex);
      if (exists(peerId)) {
        if (autoClose) {
          auto peer = get(peerId);
          // will call `peers.erase()`
          peer->close();
        } else {
          peers.erase(peerId);
        }
      }
    }

    /**
     * Get a peer by `peerId` returning `nullptr` if it doesn't exist.
     */
    static Peer* get (uint64_t peerId) {
      std::lock_guard<std::recursive_mutex> guard(peersMutex);
      if (!exists(peerId)) return nullptr;
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
      std::lock_guard<std::recursive_mutex> guard(peersMutex);

      if (exists(peerId)) {
        auto peer = get(peerId);
        if (isEphemeral) {
          peer->flags = (peer_flag_t) (peer->flags | PEER_FLAG_EPHEMERAL);
        }

        return peer;
      }

      auto peer = new Peer(peerType, peerId, isEphemeral);
      peers[peer->id] = peer;
      return peer;
    }

    /**
     * Private `Peer` class constructor
     */
    Peer (peer_type_t peerType, uint64_t peerId, bool isEphemeral) {
      std::lock_guard<std::recursive_mutex> guard(peersMutex);
      auto loop = getDefaultLoop();

      id = peerId;
      type = peerType;

      udp.data = (void *) this;

      if ((PEER_TYPE_UDP & type) == PEER_TYPE_UDP) {
        uv_udp_init(loop, &udp);
      }

      if (isEphemeral) {
        flags = (peer_flag_t) (flags | PEER_FLAG_EPHEMERAL);
      }
    }

    ~Peer () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      // will call `close()`
      Peer::remove(id, true);
    }

    void initRemotePeerInfo () {
      std::lock_guard<std::recursive_mutex> guard(mutex);

      if ((PEER_TYPE_UDP & type) == PEER_TYPE_UDP) {
        remote.init(&udp);
      }

      if (isConnected()) {
        setState(PEER_STATE_CONNECTED);
      }
    }

    void initLocalPeerInfo () {
      std::lock_guard<std::recursive_mutex> guard(mutex);

      if ((PEER_TYPE_UDP & type) == PEER_TYPE_UDP) {
        remote.init(&udp);
      }

      if (isConnected()) {
        setState(PEER_STATE_CONNECTED);
      }
    }

    void setState (peer_state_t value) {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      state = (peer_state_t) (state | value);
    }

    void removeState (peer_state_t value) {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      state = (peer_state_t) (state & ~value);
    }

    bool hasState (peer_state_t value) {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      return (value & state) == value;
    }

    const struct sockaddr* getSockAddr () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      return (const struct sockaddr *) &addr;
    }

    const RemotePeerInfo* getRemotePeerInfo () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      if ((PEER_TYPE_UDP & type) == PEER_TYPE_UDP) {
        remote.init(&udp);
      }
      return &remote;
    }

    const LocalPeerInfo* getLocalPeerInfo () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      if ((PEER_TYPE_UDP & type) == PEER_TYPE_UDP) {
        local.init(&udp);
      }
      return &local;
    }

    bool isEphemeral () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      return (PEER_FLAG_EPHEMERAL & flags) == PEER_FLAG_EPHEMERAL;
    }

    bool isBound () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      return hasState(PEER_STATE_BOUND);
    }

    bool isActive () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      // can eventually test for a `tcp` handle
      return uv_is_active((const uv_handle_t *) &udp);
    }

    bool isClosing () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      // can eventually test for a `tcp` handle
      return uv_is_closing((const uv_handle_t *) &udp);
    }

    bool isClosed () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      return hasState(PEER_STATE_CLOSED);
    }

    bool isConnected () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      if ((PEER_TYPE_UDP & type) == PEER_TYPE_UDP) {
        auto info = getRemotePeerInfo();
        return info->err == 0;
      }
      return false;
    }

    int bind (std::string address, int port) {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      int err = 0;

      err = uv_ip4_addr((char *) address.c_str(), port, &addr);
      if (err < 0) {
        return err;
      }

      err = uv_udp_bind(&udp, getSockAddr(), 0);
      if (err < 0) {
        return err;
      }

      setState(PEER_STATE_BOUND);
      initLocalPeerInfo();
      return err;
    }

    int connect (std::string address, int port) {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      int err = 0;

      err = uv_ip4_addr((char *) address.c_str(), port, &addr);
      if (err < 0) {
        return err;
      }

      err = uv_udp_connect(&udp, getSockAddr());
      if (err < 0) {
        return err;
      }

      setState(PEER_STATE_CONNECTED);
      initRemotePeerInfo();
      return err;
    }

    void send (PeerRequest *ctx, char *buf, int len, int port, String address) {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      auto loop = getDefaultLoop();
      int err = 0;

      err = uv_ip4_addr((char *) address.c_str(), port, &addr);

      if (err) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(id), std::string(uv_strerror(err)));

        return ctx->end(msg);
      }

      struct sockaddr* sockaddr = NULL;
      if (!isConnected()) {
        sockaddr = (struct sockaddr *) getSockAddr();
      }

      auto buffer = uv_buf_init(buf, len);
      auto req = new uv_udp_send_t;

      req->data = (void *) ctx;
      ctx->peer = this;

      err = uv_udp_send(req, &udp, &buffer, 1, sockaddr, [](uv_udp_send_t *req, int status) {
        auto ctx = reinterpret_cast<PeerRequest*>(req->data);
        auto peer = ctx->peer;
        std::string msg = "";

        if (status < 0) {
          msg = SSC::format(R"MSG({
            "err": {
              "id": "$S",
              "message": "$S"
            }
          })MSG", std::to_string(peer->id), std::string(uv_strerror(status)));
        } else {
          msg = SSC::format(R"MSG({
            "data": {
              "id": "$S",
              "status": "$i"
            }
          })MSG", std::to_string(peer->id), status);
        }

        ctx->end(msg);

        delete req;

        if (peer->isEphemeral()) {
          peer->close([] (Peer *peer) {
            delete peer;
          });
        }
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "Write error: $S"
          }
        })MSG", std::to_string(id), std::string(uv_strerror(err)));

        ctx->end(msg);

        if (isEphemeral()) {
          close([] (Peer *peer) {
            delete peer;
          });
        }

        delete req;
        return;
      }

      runDefaultLoop();
    }

    void close () {
      std::lock_guard<std::recursive_mutex> guard(mutex);
      static auto noop = [](Peer *){};
      close(noop);
    }

    void close (std::function<void(Peer *)> onclose) {
      std::lock_guard<std::recursive_mutex> guard(mutex);

      if (!isActive()) {
        return onclose(this);
      }

      if (isClosed() || isClosing()) {
        return onclose(this);
      }

      if ((PEER_TYPE_UDP & type) == PEER_TYPE_UDP) {
        this->onclose = onclose;
        uv_close((uv_handle_t*) &udp, [](uv_handle_t *handle) {
          auto peer = (Peer *) handle->data;

          if (peer != nullptr) {
            std::lock_guard<std::recursive_mutex> guard(peer->mutex);

            // reset state and set to CLOSED
            peer->state = PEER_STATE_CLOSED;
            Peer::remove(peer->id);
            peer->onclose(peer);
          }
        });

        runDefaultLoop();
      }
    }
  };

  typedef struct {
    uv_write_t req;
    uv_buf_t buf;
  } write_req_t;

  void Core::fsRetainOpenDescriptor (String seq, uint64_t id, Cb cb) const {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);

    auto desc = descriptors[id];

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));

      cb(seq, msg, Post{});
      return;
    }

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
    desc->retained = true;
    auto msg = SSC::format(
      R"MSG({ "data": { "id": "$S" } })MSG",
      std::to_string(desc->id)
    );

    cb(seq, msg, Post{});
  }

  void Core::handleEvent (String seq, String event, String data, Cb cb) const {
    // init page
    if (event == "domcontentloaded") {
      std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);

      for (auto const &tuple : SSC::descriptors) {
        auto desc = tuple.second;
        if (desc != nullptr) {
          std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
          desc->stale = true;
        } else {
          SSC::descriptors.erase(tuple.first);
        }
      }
    }

    cb(seq, "{}", Post{});

    runDefaultLoop();
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
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    auto now = std::chrono::system_clock::now()
      .time_since_epoch()
      .count();

    for (auto const &tuple : *posts) {
      auto id = tuple.first;
      auto post = tuple.second;

      if (post.ttl < now) {
        removePost(id);
      }
    }
  }

  void Core::putPost (uint64_t id, Post p) {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    posts->insert_or_assign(id, p);
  }

  void Core::removePost (uint64_t id) {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    if (posts->find(id) == posts->end()) return;
    auto post = getPost(id);

    if (post.body && post.bodyNeedsFree) {
      delete post.body;
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
      ";(() => {"
      "const xhr = new XMLHttpRequest();"
      "xhr.responseType = 'arraybuffer';"
      "xhr.onload = e => {"
      "  let o = `" + params + "`;"
      "  let headers = `" + SSC::trim(post.headers) + "`"
      "    .trim().split(/[\\r\\n]+/).filter(Boolean);"
      "  try { o = JSON.parse(o) } catch (err) {"
      "    console.error(err, `string<${o}>`)"
      "  };"
      "  o.seq = `" + seq + "`;"
      "  const detail = {"
      "    data: xhr.response,"
      "    sid: '" + sid + "',"
      "    headers: Object.fromEntries("
      "      headers.map(l => l.split(/\\s*:\\s*/))"
      "    ),"
      "    params: o"
      "  };"
      "  window._ipc.emit('data', detail);"
      "};"
      "xhr.open('GET', 'ipc://post?id=" + sid + "');"
      "xhr.send();"
      "})();"
    );

    posts->insert_or_assign(post.id, post);
    return js;
  }

  void Core::removeAllPosts () {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    for (auto const &tuple : *posts) {
      removePost(tuple.first);
    }
  }

  String Core::getFSConstants () const {
    auto constants = Core::getFSConstantsMap();
    auto count = constants.size();
    std::stringstream stream;

    stream << "{";
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
    auto filename = path.c_str();
    auto loop = getDefaultLoop();
    auto desc = new DescriptorContext;
    auto req = new uv_fs_t;

    desc->seq = seq;
    desc->cb = cb;

    req->data = desc;

    auto err = uv_fs_access(loop, req, filename, mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto seq = desc->seq;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(
          R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
          std::to_string(req->result), String(uv_strerror(req->result))
        );
      } else {
        msg = SSC::format(
          R"MSG({ "data": { "mode": $S } })MSG",
          std::to_string(req->flags)
        );
      }

      uv_fs_req_cleanup(req);
      delete desc;
      delete req;

      desc->cb(seq, msg, Post{});
    });

    if (err < 0) {
      auto msg = SSC::format(
        R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
        std::to_string(err), String(uv_strerror(err))
      );

      delete desc;
      delete req;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsChmod (String seq, String path, int mode, Cb cb) const {
    auto filename = path.c_str();
    auto loop = getDefaultLoop();
    auto desc = new DescriptorContext;
    auto req = new uv_fs_t;

    desc->seq = seq;
    desc->cb = cb;

    req->data = desc;

    auto err = uv_fs_chmod(loop, req, filename, mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto seq = desc->seq;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(
          R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
          std::to_string(req->result), String(uv_strerror(req->result))
        );
      } else {
        msg = SSC::format(
          R"MSG({ "data": { "mode": "$S" } })MSG",
          std::to_string(req->flags)
        );
      }

      uv_fs_req_cleanup(req);
      delete desc;
      delete req;

      desc->cb(seq, msg, Post{});
    });

    if (err < 0) {
      auto msg = SSC::format(
        R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
        std::to_string(err), String(uv_strerror(err))
      );

      delete desc;
      delete req;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsOpen (String seq, uint64_t id, String path, int flags, int mode, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(descriptorsMutex);

    auto desc = descriptors[id] = new DescriptorContext;
    guard.unlock();

    auto filename = path.c_str();
    auto loop = getDefaultLoop();
    auto req = new uv_fs_t;

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
    desc->id = id;
    desc->cb = cb;
    desc->seq = seq;

    req->data = desc;

    auto err = uv_fs_open(loop, req, filename, flags, mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);

      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(
          R"MSG({ "err": { "id": "$S", "message": "$S" } })MSG",
          std::to_string(desc->id),
          String(uv_strerror((int)req->result))
        );

        std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
        SSC::descriptors.erase(desc->id);
        delete desc;
      } else {
        desc->fd = (int) req->result;
        msg = SSC::format(
          R"MSG({ "data": { "id": "$S", "fd": $S } })MSG",
          std::to_string(desc->id),
          std::to_string(desc->fd)
        );
      }

      uv_fs_req_cleanup(req);
      delete req;

      cb(seq, msg, Post{});
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(id), String(uv_strerror(err)));

      SSC::descriptors.erase(desc->id);
      delete desc;
      delete req;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsOpendir(String seq, uint64_t id, String path, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(descriptorsMutex);
    auto desc = descriptors[id] = new DescriptorContext;
    guard.unlock();

    auto filename = path.c_str();
    auto loop = getDefaultLoop();
    auto req = new uv_fs_t;

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
    desc->id = id;
    desc->cb = cb;
    desc->seq = seq;
    desc->dir = nullptr;

    req->data = desc;

    auto err = uv_fs_opendir(loop, req, filename, [](uv_fs_t *req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(
          R"MSG({ "err": { "id": "$S", "message": "$S" } })MSG",
          std::to_string(desc->id),
          String(uv_strerror((int)req->result))
        );

        std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
        SSC::descriptors.erase(desc->id);
        delete desc;
      } else {
        msg = SSC::format(
          R"MSG({ "data": { "id": "$S" } })MSG",
          std::to_string(desc->id)
        );

        desc->dir = (uv_dir_t *) req->ptr;
      }

      uv_fs_req_cleanup(req);
      delete req;

      cb(seq, msg, Post{});
    });

    if (err < 0) {
      auto msg = SSC::format(
        R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
        std::to_string(err), String(uv_strerror(err))
      );

      SSC::descriptors.erase(desc->id);
      delete desc;
      delete req;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsReaddir(String seq, uint64_t id, size_t nentries, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(descriptorsMutex);
    auto desc = descriptors[id];
    guard.unlock();

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No directory descriptor found with that id"
        }
      })MSG", std::to_string(id));


      cb(seq, msg, Post{});
      return;
    }

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);

    if (desc->dir == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "message": "Directory descriptor with that id is not open"
        }
      })MSG", std::to_string(id));

      cb(seq, msg, Post{});
      return;
    }

    auto loop = getDefaultLoop();
    auto req = new uv_fs_t;

    desc->seq = seq;
    desc->cb = cb;
    desc->dir->dirents = desc->dirents;
    desc->dir->nentries = nentries;

    req->data = desc;

    auto err = uv_fs_readdir(loop, req, desc->dir, [](uv_fs_t *req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(
          R"MSG({ "err": { "id": "$S", "message": "$S" } })MSG",
          std::to_string(desc->id),
          String(uv_strerror((int)req->result))
        );
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

        msg = SSC::format(
          R"MSG({ "data": { "id": "$S", "entries": $S } })MSG",
          std::to_string(desc->id),
          entries.str()
        );
      }

      uv_fs_req_cleanup(req);
      delete req;

      cb(seq, msg, Post{});
    });

    if (err < 0) {
      auto msg = SSC::format(
        R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
        std::to_string(err), String(uv_strerror(err))
      );

      cb(seq, msg, Post{});
      delete req;
      return;
    }

    runDefaultLoop();
  }

  void Core::fsClose (String seq, uint64_t id, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(descriptorsMutex);
    auto desc = descriptors[id];
    guard.unlock();

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));


      cb(seq, msg, Post{});
      return;
    }

    auto req = new uv_fs_t;

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
    desc->seq = seq;
    desc->cb = cb;

    req->data = desc;

    auto err = uv_fs_close(getDefaultLoop(), req, desc->fd, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      std::string msg;
      auto seq = desc->seq;
      auto cb = desc->cb;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(desc->id), String(uv_strerror((int)req->result)));
      } else {
        msg = SSC::format(R"MSG({
          "data": {
            "id": "$S",
            "fd": $S
          }
        })MSG", std::to_string(desc->id), std::to_string(desc->fd));

        desc->fd = 0;
        std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
        SSC::descriptors.erase(desc->id);
        delete desc;
      }

      uv_fs_req_cleanup(req);
      delete req;

      cb(seq, msg, Post{});
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(id), String(uv_strerror(err)));

      delete req;
      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsClosedir (String seq, uint64_t id, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(descriptorsMutex);
    auto desc = descriptors[id];
    guard.unlock();

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No directory descriptor found with that id"
        }
      })MSG", std::to_string(id));


      cb(seq, msg, Post{});
      return;
    }


    auto req = new uv_fs_t;

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
    desc->seq = seq;
    desc->cb = cb;

    req->data = desc;

    auto err = uv_fs_closedir(getDefaultLoop(), req, desc->dir, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      std::string msg;
      auto seq = desc->seq;
      auto cb = desc->cb;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(desc->id), String(uv_strerror((int)req->result)));
      } else {
        msg = SSC::format(R"MSG({
          "data": {
            "id": "$S"
          }
        })MSG", std::to_string(desc->id));

        std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
        desc->dir = nullptr;
        SSC::descriptors.erase(desc->id);
        delete desc;
      }

      cb(seq, msg, Post{});

      uv_fs_req_cleanup(req);
      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(id), String(uv_strerror(err)));

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsCloseOpenDescriptor (String seq, uint64_t id, Cb cb) const {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);

    auto desc = descriptors[id];

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
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
    int pending = SSC::descriptors.size();
    int queued = 0;

    for (auto const &tuple : SSC::descriptors) {
      ids.push_back(tuple.first);
    }

    for (auto const id : ids) {
      auto desc = SSC::descriptors[id];
      pending--;

      if (desc == nullptr) {
        SSC::descriptors.erase(desc->id);
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
    std::unique_lock<std::recursive_mutex> guard(descriptorsMutex);
    auto desc = descriptors[id];
    guard.unlock();

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG");

      cb(seq, msg, Post{});
      return;
    }

    auto buf = new char[len];
    auto req = new uv_fs_t;
    const uv_buf_t iov = uv_buf_init(buf, len * sizeof(char));

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
    desc->data = buf;
    desc->seq = seq;
    desc->cb = cb;

    req->data = desc;

    auto err = uv_fs_read(getDefaultLoop(), req, desc->fd, &iov, 1, offset, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      std::string msg = "{}";
      auto seq = desc->seq;
      auto cb = desc->cb;
      Post post = {0};

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(desc->id), String(uv_strerror((int)req->result)));
      } else {
        post.body = (char *) desc->data;
        post.length = (int) req->result;
        post.bodyNeedsFree = true;
      }

      desc->data = 0;
      cb(seq, msg, post);

      uv_fs_req_cleanup(req);

      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(id), String(uv_strerror(err)));

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsWrite (String seq, uint64_t id, String data, int64_t offset, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(descriptorsMutex);
    auto desc = descriptors[id];
    guard.unlock();

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "code": "ENOTOPEN",
          "message": "No file descriptor found with that id"
        }
      })MSG");


      cb(seq, msg, Post{});
      return;
    }


    auto bytes = new char[data.size()];
    memcpy(bytes, data.data(), data.size());

    const uv_buf_t buf = uv_buf_init((char*) bytes, (int) data.size());
    auto req = new uv_fs_t;

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
    desc->seq = seq;
    desc->cb = cb;

    req->data = desc;
    req->ptr = bytes;

    auto err = uv_fs_write(getDefaultLoop(), req, desc->fd, &buf, 1, offset, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      std::string msg;
      auto seq = desc->seq;
      auto cb = desc->cb;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(desc->id), String(uv_strerror((int)req->result)));
      } else {
        msg = SSC::format(R"MSG({
          "data": {
            "id": "$S",
            "result": "$i"
          }
        })MSG", std::to_string(desc->id), (int)req->result);
      }

      cb(seq, msg, Post{});
      uv_fs_req_cleanup(req);
      if (req->ptr) {
        delete (char *) req->ptr;
      }
      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(id), String(uv_strerror(err)));

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsStat (String seq, String path, Cb cb) const {
    DescriptorContext* desc = new DescriptorContext;
    auto req = new uv_fs_t;

    desc->seq = seq;
    desc->cb = cb;

    req->data = desc;

    auto err = uv_fs_stat(getDefaultLoop(), req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", String(uv_strerror((int)req->result)));
      } else {
        auto stats = uv_fs_get_statbuf(req);
        msg = SSC::format(
          R"MSG({
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

      uv_fs_req_cleanup(req);

      delete desc;
      delete req;

      cb(seq, msg, Post{});
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));


      delete desc;
      delete req;

      cb(seq, msg, Post{});

      return;
    }

    runDefaultLoop();
  }

  void Core::fsFStat (String seq, uint64_t id, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(descriptorsMutex);
    auto desc = descriptors[id];
    guard.unlock();

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "code": "ENOTOPEN",
          "message": "No file descriptor found with that id"
        }
      })MSG");


      cb(seq, msg, Post{});
      return;
    }

    std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_fstat(getDefaultLoop(), req, desc->fd, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(desc->id),
        String(uv_strerror((int)req->result)));
      } else {
        auto stats = uv_fs_get_statbuf(req);
        msg = SSC::trim(SSC::format(R"MSG({
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
          std::to_string(stats->st_birthtim.tv_nsec)
        ));
      }

      uv_fs_req_cleanup(req);
      delete req;

      cb(seq, msg, Post{});
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(id), String(uv_strerror(err)));

      delete req;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsGetOpenDescriptors (String seq, Cb cb) const {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);

    std::string msg = "";
    int pending = SSC::descriptors.size();

    msg += "{ \"data\": [";
    for (auto const &tuple : SSC::descriptors) {
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
    auto filename = path.c_str();
    auto loop = getDefaultLoop();

    auto desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_unlink(loop, req, filename, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(desc->id),
        String(uv_strerror((int)req->result)));
      } else {
        msg = SSC::format(R"MSG({
          "data": {
            "result": "$i"
          }
        })MSG", (int)req->result);
      }

      uv_fs_req_cleanup(req);

      delete req;
      delete desc;

      cb(seq, msg, Post{});
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      delete req;
      delete desc;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsRename (String seq, String pathA, String pathB, Cb cb) const {
    auto loop = getDefaultLoop();
    auto src = pathA.c_str();
    auto dst = pathB.c_str();

    auto desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_rename(loop, req, src, dst, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(desc->id),
        String(uv_strerror((int)req->result)));
      } else {
        msg = SSC::format(R"MSG({
          "data": {
            "result": "$i"
          }
        })MSG", (int)req->result);
      }

      uv_fs_req_cleanup(req);

      delete req;
      delete desc;

      cb(seq, msg, Post{});
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      delete req;
      delete desc;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsCopyFile (String seq, String pathA, String pathB, int flags, Cb cb) const {
    auto loop = getDefaultLoop();
    auto src = pathA.c_str();
    auto dst = pathB.c_str();

    auto desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_copyfile(loop, req, src, dst, flags, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(desc->id),
        String(uv_strerror((int)req->result)));
      } else {
        msg = SSC::format(R"MSG({
          "data": {
            "result": "$i"
          }
        })MSG", (int)req->result);
      }

      uv_fs_req_cleanup(req);

      delete req;
      delete desc;

      cb(seq, msg, Post{});
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      delete req;
      delete desc;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsRmdir (String seq, String path, Cb cb) const {
    auto filename = path.c_str();
    auto loop = getDefaultLoop();

    auto desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_rmdir(loop, req, filename, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(desc->id),
        String(uv_strerror((int)req->result)));
      } else {
        msg = SSC::format(R"MSG({
          "data": {
            "result": "$i"
          }
        })MSG", (int)req->result);
      }

      uv_fs_req_cleanup(req);

      delete req;
      delete desc;

      cb(seq, msg, Post{});
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      delete req;
      delete desc;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsMkdir (String seq, String path, int mode, Cb cb) const {
    auto filename = path.c_str();
    auto loop = getDefaultLoop();

    auto desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_mkdir(loop, req, filename, mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto seq = desc->seq;
      auto cb = desc->cb;
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(desc->id),
        String(uv_strerror((int)req->result)));
      } else {
        msg = SSC::format(R"MSG({
          "data": {
            "result": "$i"
          }
        })MSG", (int)req->result);
      }

      uv_fs_req_cleanup(req);

      delete req;
      delete desc;

      cb(seq, msg, Post{});
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      delete req;
      delete desc;

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::sendBufferSize (String seq, uint64_t peerId, int size, Cb cb) const {
    std::lock_guard<std::recursive_mutex> guard(peersMutex);

    Peer* peer = peers[peerId];

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "method": "sendBufferSize",
          "message": "No handle with that id"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    uv_handle_t* handle;

    if ((PEER_TYPE_UDP & peer->type) == PEER_TYPE_UDP) {
      handle = (uv_handle_t*) &peer->udp;
    } else {
      auto msg = SSC::format(R"MSG({
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
    std::lock_guard<std::recursive_mutex> guard(peersMutex);
    Peer* peer = peers[peerId];

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "method": "Cb",
          "message": "Not connected"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    uv_handle_t* handle;

    if ((PEER_TYPE_UDP & peer->type) == PEER_TYPE_UDP) {
      handle = (uv_handle_t*) &peer->udp;
    } else {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "Handle is not valid"
        }
      })MSG");

      cb(seq, msg, Post{});
      return;
    }

    int sz = size;
    int rSize = uv_recv_buffer_size(handle, &sz);

    auto msg = SSC::format(R"MSG({
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
    Peer* peer = peers[peerId];

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "No connection with specified id"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    int r;

    if ((PEER_TYPE_UDP & peer->type) == PEER_TYPE_UDP) {
      r = uv_read_stop((uv_stream_t*) &peer->udp);
    } else {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "Handle is not valid"
        }
      })MSG");

      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({ "data": $i })MSG", r);
    cb(seq, msg, Post{});
  }

  void Core::close (String seq, uint64_t peerId, Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
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

    peer->close([cb, seq] (Peer *) {
      cb(seq, "{}", Post{});
    });
  }

  void Core::shutdown (String seq, uint64_t peerId, Cb cb) const {
    Peer* peer = peers[peerId];

    if (peer == nullptr) {
      auto msg = SSC::format(
        R"MSG({
          "err": {
            "id": "$S",
            "message": "No connection with specified id"
          }
        })MSG",
        std::to_string(peerId)
      );

      cb(seq, msg, Post{});
      return;
    }

    peer->seq = seq;
    peer->cb = cb;
    peer->id = peerId;

    uv_handle_t* handle;

    if ((PEER_TYPE_UDP & peer->type) == PEER_TYPE_UDP) {
      handle = (uv_handle_t*) &peer->udp;
    } else {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "Handle is not valid"
        }
      })MSG");

      cb(seq, msg, Post{});
      return;
    }

    handle->data = peer;

    uv_shutdown_t *req = new uv_shutdown_t;
    req->data = handle;

    uv_shutdown(req, (uv_stream_t*) handle, [](uv_shutdown_t *req, int status) {
      auto peer = reinterpret_cast<Peer*>(req->handle->data);

      auto msg = SSC::format(R"MSG({
        "data": {
          "status": "$i"
        }
      })MSG", status);

     peer->cb(peer->seq, msg, Post{});

     free(req);
     free(req->handle);
    });

    runDefaultLoop();
  }

  void Core::udpBind (String seq, uint64_t peerId, String address, int port, Cb cb) const {
    if (Peer::exists(peerId) && Peer::get(peerId)->isBound()) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ERR_SOCKET_ALREADY_BOUND",
          "source": "udp",
          "message": ": Socket is already bound"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = Peer::create(PEER_TYPE_UDP, peerId);
    auto err = peer->bind(address, port);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "source": "udp",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    auto info = peer->getLocalPeerInfo();

    if (info->err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "source": "udp",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(info->err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {
        "source": "udp",
        "event": "listening",
        "id": "$S",
        "address": "$S",
        "port": $i,
        "family": "$S"
      }
    })MSG", std::to_string(peerId), info->address, info->port, info->family);

    cb(seq, msg, Post{});

    runDefaultLoop();
  }

  void Core::udpConnect (String seq, uint64_t peerId, String address, int port, Cb cb) const {
    auto peer = Peer::create(PEER_TYPE_UDP, peerId);
    auto err = peer->connect(address, port);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "source": "udp",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {
        "source": "udp",
        "address": "$S",
        "port": $i,
        "id": "$S"
      }
    })MSG", std::string(address), port, std::to_string(peerId));

    cb(seq, msg, Post{});

    runDefaultLoop();
  }

  void Core::udpGetPeerName (String seq, uint64_t peerId, Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
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
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(info->err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {
        "source": "udp",
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
        "err": {
          "source": "udp",
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
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(info->err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {
        "source": "udp",
        "id": "$S",
        "address": "$S",
        "port": $i,
        "family": "$S"
      }
    })MSG", std::to_string(peerId), info->address, info->port, info->family);

    cb(seq, msg, Post{});
  }

  void Core::udpGetState (String seq, uint64_t peerId,  Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "code": "NOT_FOUND_ERR",
          "message": "no such peer"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    auto peer = Peer::get(peerId);
    auto msg = SSC::format(R"MSG({
        "data": {
          "source": "udp",
          "id": "$S",
          "type": "$S",
          "ephemeral": $S,
          "bound": $S,
          "active": $S,
          "closing": $S,
          "closed": $S,
          "connected": $S
        }
      })MSG",
      std::to_string(peerId),
      std::string(((PEER_TYPE_UDP & peer->type) == PEER_TYPE_UDP) ? "udp" : ""),
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
    auto peer = Peer::create(PEER_TYPE_UDP, peerId, ephemeral);
    auto ctx = new PeerRequest(seq, cb);

    peer->send(ctx, buf, len, port, address);
  }

  void Core::udpReadStart (String seq, uint64_t peerId, Cb cb) const {
    if (!Peer::exists(peerId)) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "no such handle"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    auto peer = Peer::get(peerId);

    if (peer->isActive()) {
      auto msg = SSC::format(R"MSG({ "data": {} })MSG");
      cb(seq, msg, Post{});
      return;
    }

    if (peer->isClosing()) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "handle is closing"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      return;
    }

    peer->onudpread = cb;

    auto allocate = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      buf->len = suggested_size;
      memset(buf->base, 0, buf->len);
    };

    int err = uv_udp_recv_start(&peer->udp, allocate, [](uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
      std::lock_guard<std::recursive_mutex> guard(peersMutex);
      Peer *peer = (Peer*)handle->data;

      if (nread == UV_EOF) {
        auto msg = SSC::format(R"MSG({
          "data": {
            "id": "$S",
            "EOF": true
          }
        })MSG", std::to_string(peer->id));
        peer->onudpread("-1", msg, Post{});
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
        post.body = buf->base;
        post.length = (int) nread;
        post.headers = headers;
        post.bodyNeedsFree = true;

        auto msg = SSC::format(R"MSG({
            "data": {
              "source": "udpReadStart",
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
        peer->onudpread("-1", msg, post);
      }
    });

    // `UV_EALREADY || UV_EBUSY` means there is active IO on the underlying handle
    if (err < 0 && err != UV_EALREADY && err != UV_EBUSY) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({ "data": {} })MSG");
    cb(seq, msg, Post{});
    runDefaultLoop();
  }

  void Core::dnsLookup (String seq, String hostname, int family, Cb cb) const {
    auto* rctx = new PeerRequest();
    requests[rctx->id] = rctx;
    rctx->cb = cb;
    rctx->seq = seq;

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
    resolver->data = rctx;

    uv_getaddrinfo(getDefaultLoop(), resolver, [](uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
      auto rctx = (PeerRequest*) resolver->data;

      if (status < 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "code": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(rctx->id), String(uv_err_name((int)status)), String(uv_strerror(status)));
        rctx->cb(rctx->seq, msg, Post{});
        requests.erase(rctx->id);
        delete rctx;
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

      auto msg = SSC::format(R"MSG({
        "data": {
          "address": "$S",
          "family": $i
        }
      })MSG", address, res->ai_family == AF_INET ? 4 : res->ai_family == AF_INET6 ? 6 : 0);

      rctx->cb(rctx->seq, msg, Post{});
      requests.erase(rctx->id);
      delete rctx;

      uv_freeaddrinfo(res);
    }, hostname.c_str(), nullptr, &hints);

    runDefaultLoop();
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
