#ifndef SSC_CORE_H
#define SSC_CORE_H

#include "common.hh"
#include "include/uv.h"

#ifndef _WIN32
#include <ifaddrs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#endif

#ifdef __linux__
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

      void udpBind (String seq, uint64_t peerId, String ip, int port, Cb cb) const;
      void udpConnect (String seq, uint64_t peerId, const char* ip, int port, Cb cb) const;
      void udpSend (String seq, uint64_t peerId, char* buf, int offset, int len, int port, const char* ip, bool ephemeral, Cb cb) const;
      void udpGetPeerName (String seq, uint64_t peerId, Cb cb) const;
      void udpReadStart (String seq, uint64_t peerId, Cb cb) const;
      void udpGetSockName (String seq, uint64_t peerId, Cb cb) const;

      void sendBufferSize (String seq, uint64_t peerId, int size, Cb cb) const;
      void recvBufferSize (String seq, uint64_t peerId, int size, Cb cb) const;
      void close (String seq, uint64_t peerId, Cb cb) const;
      void shutdown (String seq, uint64_t peerId, Cb cb) const;
      void readStop (String seq, uint64_t peerId, Cb cb) const;

      void dnsLookup (String seq, String hostname, Cb cb) const;
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

  struct GenericContext {
    Cb cb;
    uint64_t id;
    String seq;
  };

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

  typedef enum {
    PEER_TYPE_NONE = 0,
    PEER_TYPE_TCP = 1,
    PEER_TYPE_UDP = 2,
    PEER_TYPE_MAX = 0xf
  } peer_type_t;

  struct Peer {
    Cb cb;
    String seq;
    uint64_t id;

    uv_udp_t udp;
    uv_stream_t* stream;
    peer_type_t type; // can be bit or'd
    bool ephemeral = false;
    bool connected = false;
    struct sockaddr_in addr;
  };

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

  struct PeerInfo {
    String ip = "";
    String family = "";
    int port = 0;
    int error = 0;
    void init(uv_udp_t* socket);
  };

  void PeerInfo::init (uv_udp_t* socket) {
    int namelen;
    struct sockaddr_storage addr;
    namelen = sizeof(addr);

    error = uv_udp_getpeername(socket, (struct sockaddr*) &addr, &namelen);

    if (error) {
      return;
    }

    if (addr.ss_family == AF_INET) {
      family = "IPv4";
      ip = addrToIPv4((struct sockaddr_in*) &addr);
      port = (int)htons(((struct sockaddr_in*) &addr)->sin_port);
    } else {
      family = "IPv6";
      ip = addrToIPv6((struct sockaddr_in6*) &addr);
      port = (int)htons(((struct sockaddr_in6*) &addr)->sin6_port);
    }
  }

  static void parseAddress (struct sockaddr *name, int* port, char* ip) {
    struct sockaddr_in *name_in = (struct sockaddr_in *) name;
    *port = ntohs(name_in->sin_port);
    uv_ip4_name(name_in, ip, 17);
  }

  std::map<uint64_t, Peer*> peers;
  std::map<uint64_t, GenericContext*> contexts;
  std::map<uint64_t, DescriptorContext*> descriptors;

  std::recursive_mutex peersMutex;
  std::recursive_mutex contextsMutex;
  std::recursive_mutex descriptorsMutex;

  std::atomic<bool> didTimersInit = false;
  std::atomic<bool> didTimersStart = false;
  std::recursive_mutex timersMutex;
  std::recursive_mutex timersInitMutex;
  std::recursive_mutex timersStartMutex;

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
  static uv_loop_t defaultLoop = {0};

  static std::atomic<bool> didLoopInit = false;
  static std::atomic<bool> isLoopRunning = false;
  static std::recursive_mutex loopMutex;

  static void initTimers ();
  static void startTimers ();
  static uv_loop_t* getDefaultLoop ();

#ifdef __linux__
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

#ifdef __linux__
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
#ifdef __linux__
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
        std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
        desc->stale = true;
      }
    }

    cb(seq, "", Post{});

    runDefaultLoop();
  }

  bool Core::hasTask (String id) {
    if (id.size() == 0) return false;
    return tasks->find(id) != tasks->end();
  }

  Task Core::getTask (String id) {
    if (tasks->find(id) == tasks->end()) return Task{};
    return tasks->at(id);
  }

  void Core::removeTask (String id) {
    if (tasks->find(id) == tasks->end()) return;
    tasks->erase(id);
  }

  void Core::putTask (String id, Task t) {
    tasks->insert_or_assign(id, t);
  }

  Post Core::getPost (uint64_t id) {
    if (posts->find(id) == posts->end()) return Post{};
    return posts->at(id);
  }

  bool Core::hasPost (uint64_t id) {
    return posts->find(id) != posts->end();
  }

  void Core::expirePosts () {
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
    posts->insert_or_assign(id, p);
  }

  void Core::removePost (uint64_t id) {
    if (posts->find(id) == posts->end()) return;
    auto post = getPost(id);

    if (post.body && post.bodyNeedsFree) {
      delete post.body;
      post.body = nullptr;
    }

    posts->erase(id);
  }

  String Core::createPost (String seq, String params, Post post) {
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
        "peerId": "$S",
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
          "peerId": "$S",
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
        "peerId": "$S",
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
          "peerId": "$S",
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
    Peer* peer = peers[peerId];

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "peerId": "$S",
          "message": "No connection with specified id"
        }
      })MSG", std::to_string(peerId));
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

    uv_close(handle, [](uv_handle_t* handle) {
      auto peer = reinterpret_cast<Peer*>(handle->data);

      auto msg = SSC::format(R"MSG({ "data": {} })MSG");
      peer->cb(peer->seq, msg, Post{});
      free(handle);
    });

    runDefaultLoop();
  }

  void Core::shutdown (String seq, uint64_t peerId, Cb cb) const {
    Peer* peer = peers[peerId];

    if (peer == nullptr) {
      auto msg = SSC::format(
        R"MSG({
          "err": {
            "peerId": "$S",
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

  void Core::udpBind (String seq, uint64_t peerId, String ip, int port, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(peersMutex);
    Peer* peer = peers[peerId] = new Peer();
    peer->cb = cb;
    peer->seq = seq;
    peer->type = PEER_TYPE_UDP;
    peer->id = peerId;
    peer->udp.data = peer;

    int err;
    struct sockaddr_in addr;

    err = uv_ip4_addr((char*) ip.c_str(), port, &peer->addr);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "uv_ip4_addr: $S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      guard.unlock();
      cb(seq, msg, Post{});
      return;
    }

    uv_udp_init(getDefaultLoop(), &peer->udp);
    err = uv_udp_bind(&peer->udp, (const struct sockaddr*)&peer->addr, 0);

    guard.unlock();

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "uv_udp_bind: $S"
        }
      })MSG", std::to_string(peer->id), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {
        "source": "udp",
        "id": "$S",
        "event": "listening"
      }
    })MSG", std::to_string(peer->id));

    cb(seq, msg, Post{});
    runDefaultLoop();
  }

  void Core::udpConnect (String seq, uint64_t peerId, const char* ip, int port, Cb cb) const {
    std::lock_guard<std::recursive_mutex> guard(peersMutex);
    Peer* peer = nullptr;
    auto loop = getDefaultLoop();

    if (peers[peerId] == nullptr) {
      peer = new Peer();
      peers[peerId] = peer;
      uv_udp_init(loop, &peer->udp);
    } else {
      peer = peers[peerId];
    }

    peer->id = peerId;
    peer->cb = cb;
    peer->seq = seq;
    peer->type = PEER_TYPE_UDP;

    int err;
    err = uv_ip4_addr(ip, port, &peer->addr);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "uv_udp_connect: $S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    err = uv_udp_connect(&peer->udp, (const struct sockaddr*)&peer->addr);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "uv_udp_connect: $S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {
        "source": "udp",
        "ip": "$S",
        "port": $i,
        "id": "$S"
      }
    })MSG", std::string(ip), port, std::to_string(peerId));

    peer->connected = true;

    cb(seq, msg, Post{});
    runDefaultLoop();
  }

  void Core::udpGetPeerName (String seq, uint64_t peerId, Cb cb) const {
    struct sockaddr sockname;
    int len = sizeof(sockname);
    int err = 0;

    std::lock_guard<std::recursive_mutex> guard(peersMutex);
    Peer* peer = peers[peerId];

    if (peer == nullptr) {
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

    PeerInfo info;
    info.init(&peer->udp);

    auto msg = SSC::format(R"MSG({
      "data": {
        "source": "udp",
        "id": "$S",
        "ip": "$S",
        "port": $i,
        "family": "$S"
      }
    })MSG", std::to_string(peerId), info.ip, info.port, info.family);
    cb(seq, msg, Post{});
  }

  void Core::udpGetSockName (String seq, uint64_t peerId, Cb cb) const {
    struct sockaddr sockname;
    int len = sizeof(sockname);
    int err = 0;

    std::lock_guard<std::recursive_mutex> guard(peersMutex);
    Peer* peer = peers[peerId];

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "no handle found for id provided"
        }
      })MSG", std::to_string(peerId));
      cb(seq, msg, Post{});
      return;
    }

    err = uv_udp_getsockname(&peer->udp, &sockname, &len);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "uv_udp_getsockname: $S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    auto name = ((struct sockaddr_in*)&sockname);
    int port = htons(name->sin_port);
    std::string ip = std::string((char*)inet_ntoa(name->sin_addr));

    auto msg = SSC::format(R"MSG({
      "data": {
        "source": "udp",
        "id": "$S",
        "ip": "$S",
        "port": $i
      }
    })MSG", std::to_string(peerId), ip, port);
    cb(seq, msg, Post{});
  }

  void Core::udpSend (String seq, uint64_t peerId, char* buf, int offset, int len, int port, const char* ip, bool ephemeral, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(peersMutex);
    Peer* peer = nullptr;
    auto loop = getDefaultLoop();

    if (peers[peerId] != nullptr) {
      peer = peers[peerId];
    } else {
      peer = new Peer();
      peer->id = peerId;
      peer->type = PEER_TYPE_UDP;

      uv_udp_init(loop, &peer->udp);

      if (ephemeral) {
        peer->ephemeral = true;
      } else {
        peers[peerId] = peer;
      }
    }

    peer->cb = cb;
    peer->seq = seq;

    int err;
    err = uv_ip4_addr((char*) ip, port, &peer->addr);

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));

      guard.unlock();
      cb(seq, msg, Post{});
      return;
    }

    uv_buf_t buffer = uv_buf_init(buf + offset, len);
    uv_udp_send_t* req = new uv_udp_send_t;

    req->data = peer;

    struct sockaddr* addr = NULL;

    if (!peer->connected) {
      addr = (struct sockaddr*)&peer->addr;
    }

    err = uv_udp_send(req, &peer->udp, &buffer, 1, addr, [](uv_udp_send_t *req, int status) {
      std::lock_guard<std::recursive_mutex> guard(peersMutex);
      auto peer = reinterpret_cast<Peer*>(req->data);
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

      peer->cb(peer->seq, msg, Post{});

      if (peer->ephemeral) {
        uv_close((uv_handle_t *) &peer->udp, 0);
        delete peer;
      }

      delete req;
    });

    guard.unlock();

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "message": "Write error: $S"
        }
      })MSG", std::to_string(peer->id), std::string(uv_strerror(err)));

      cb(seq, msg, Post{});

      if (ephemeral) {
        uv_close((uv_handle_t *) &peer->udp, 0);
        delete peer;
      }

      delete req;
      return;
    }

    runDefaultLoop();
  }

  void Core::udpReadStart (String seq, uint64_t peerId, Cb cb) const {
    std::unique_lock<std::recursive_mutex> guard(peersMutex);
    Peer* peer = peers[peerId];

    if (peer == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "no such handle"
        }
      })MSG", std::to_string(peerId));

      cb(seq, msg, Post{});
      guard.unlock();
      return;
    }

    peer->cb = cb;
    peer->seq = seq;

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
            "peerId": "$S",
            "EOF": true
          }
        })MSG", std::to_string(peer->id));
        peer->cb("-1", msg, Post{});
        return;
      }

      if (nread > 0) {
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
              "ip": "$S"
            }
          })MSG",
          std::to_string(peer->id),
          std::to_string(post.length),
          port,
          ip
        );
        peer->cb("-1", msg, post);
      }
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "source": "udp",
          "id": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(peerId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      guard.unlock();
      return;
    }

    guard.unlock();
    auto msg = SSC::format(R"MSG({ "data": {} })MSG");
    cb(seq, msg, Post{});
    runDefaultLoop();
  }

  void Core::dnsLookup (String seq, String hostname, Cb cb) const {
    auto ctxId = SSC::rand64();
    GenericContext* ctx = contexts[ctxId] = new GenericContext;
    ctx->id = ctxId;
    ctx->cb = cb;
    ctx->seq = seq;

    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC; // `AF_INET` or `AF_INET6`
    hints.ai_socktype = 0; // `0` for any
    hints.ai_protocol = 0; // `0` for any

    uv_getaddrinfo_t* resolver = new uv_getaddrinfo_t;
    resolver->data = ctx;

    uv_getaddrinfo(getDefaultLoop(), resolver, [](uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
      auto ctx = (GenericContext*) resolver->data;

      if (status < 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "code": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(ctx->id), String(uv_err_name((int)status)), String(uv_strerror(status)));
        ctx->cb(ctx->seq, msg, Post{});
        contexts.erase(ctx->id);
        delete ctx;
        return;
      }

      String ip = "";

      if (res->ai_family == AF_INET) {
        char addr[17] = {'\0'};
        uv_ip4_name((struct sockaddr_in*)(res->ai_addr), addr, 16);
        ip = String(addr, 17);
      } else if (res->ai_family == AF_INET6) {
        char addr[40] = {'\0'};
        uv_ip6_name((struct sockaddr_in6*)(res->ai_addr), addr, 39);
        ip = String(addr, 40);
      }

      ip = ip.erase(ip.find('\0'));

      auto msg = SSC::format(R"MSG({
        "data": {
          "address": "$S",
          "family": $i
        }
      })MSG", ip, res->ai_family == AF_INET ? 4 : res->ai_family == AF_INET6 ? 6 : 0);

      ctx->cb(ctx->seq, msg, Post{});
      contexts.erase(ctx->id);
      delete ctx;

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
      String ip = "";
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
