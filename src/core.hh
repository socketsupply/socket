#ifndef SSC_CORE_H
#define SSC_CORE_H
//
// File and Network IO for all operating systems.
//
#include "common.hh"
#include "include/uv.h"

#ifndef _WIN32
#include <ifaddrs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#endif

#if defined(__APPLE__)
	#import <Webkit/Webkit.h>
  using Task = id<WKURLSchemeTask>;
#elif defined(__linux__)
  class Task {
  };
  // TODO @jwerle
#elif defined(_WIN32)
  // TODO
#endif

#define DEFAULT_BACKLOG 128

namespace SSC {
  using String = std::string;

  struct Post {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char* body = nullptr;
    int length = 0;
    String headers;
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

      void fsAccess (String seq, String path, int mode, Cb cb) const;
      void fsChmod (String seq, String path, int mode, Cb cb) const;
      void fsCopyFile (String seq, String src, String dst, int mode, Cb cb) const;
      void fsClose (String seq, uint64_t id, Cb cb) const;
      void fsClosedir (String seq, uint64_t id, Cb cb) const;
      void fsFStat (String seq, uint64_t id, Cb cb) const;
      void fsMkDir (String seq, String path, int mode, Cb cb) const;
      void fsOpen (String seq, uint64_t id, String path, int flags, int mode, Cb cb) const;
      void fsOpendir (String seq, uint64_t id, String path, Cb cb) const;
      void fsRead (String seq, uint64_t id, int len, int offset, Cb cb) const;
      void fsReaddir (String seq, uint64_t id, size_t entries, Cb cb) const;
      void fsRename (String seq, String pathA, String pathB, Cb cb) const;
      void fsRmDir (String seq, String path, Cb cb) const;
      void fsStat (String seq, String path, Cb cb) const;
      void fsUnlink (String seq, String path, Cb cb) const;
      void fsWrite (String seq, uint64_t id, String data, int64_t offset, Cb cb) const;

      void tcpBind (String seq, uint64_t serverId, String ip, int port, Cb cb) const;
      void tcpConnect (String seq, uint64_t clientId, int port, String ip, Cb cb) const;
      void tcpSetTimeout (String seq, uint64_t clientId, int timeout, Cb cb) const;
      void tcpSetKeepAlive (String seq, uint64_t clientId, int timeout, Cb cb) const;
      void tcpSend (uint64_t clientId, String message, Cb cb) const;
      void tcpReadStart (String seq, uint64_t clientId, Cb cb) const;

      void udpBind (String seq, uint64_t serverId, String ip, int port, Cb cb) const;
      void udpSend (String seq, uint64_t clientId, String message, int offset, int len, int port, const char* ip, Cb cb) const;
      void udpReadStart (String seq, uint64_t serverId, Cb cb) const;

      void sendBufferSize (String seq, uint64_t clientId, int size, Cb cb) const;
      void recvBufferSize (String seq, uint64_t clientId, int size, Cb cb) const;
      void close (String seq, uint64_t clientId, Cb cb) const;
      void shutdown (String seq, uint64_t clientId, Cb cb) const;
      void readStop (String seq, uint64_t clientId, Cb cb) const;

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
      String createPost (String params, Post post);

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
    uv_file fd;
    uv_dir_t *dir = nullptr;
    // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
    uv_dirent_t dirents[256];
    String seq;
    Cb cb;
    uint64_t id;
    void *data;
  };

  struct Peer {
    Cb cb;
    String seq;

    uv_tcp_t* tcp;
    uv_udp_t* udp;
    uv_stream_t* stream;

    ~Peer () {
      delete this->tcp;
      delete this->udp;
    };
  };

  struct Server : public Peer {
    uint64_t serverId;
  };

  struct Client : public Peer {
    Server* server;
    uint64_t clientId;
  };

  static uv_loop_t* getDefaultLoop () {
    return uv_default_loop();
  }

  static void runDefaultLoop () {
    auto loop = getDefaultLoop();
    uv_run(loop, UV_RUN_DEFAULT);
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

  struct PeerInfo {
    String ip = "";
    String family = "";
    int port = 0;
    int error = 0;
    void init(uv_tcp_t* connection);
    void init(uv_udp_t* socket);
  };

  void PeerInfo::init (uv_tcp_t* connection) {
    int namelen;
    struct sockaddr_storage addr;
    namelen = sizeof(addr);

    error = uv_tcp_getpeername(connection, (struct sockaddr*) &addr, &namelen);

    if (error) {
      return;
    }

    if (addr.ss_family == AF_INET) {
      family = "ipv4";
      ip = addrToIPv4((struct sockaddr_in*) &addr);
      port = (int)htons(((struct sockaddr_in*) &addr)->sin_port);
    } else {
      family = "ipv6";
      ip = addrToIPv6((struct sockaddr_in6*) &addr);
      port = (int)htons(((struct sockaddr_in6*) &addr)->sin6_port);
    }
  }

  void PeerInfo::init (uv_udp_t* socket) {
    int namelen;
    struct sockaddr_storage addr;
    namelen = sizeof(addr);

    error = uv_udp_getpeername(socket, (struct sockaddr*) &addr, &namelen);

    if (error) {
      return;
    }

    if (addr.ss_family == AF_INET) {
      family = "ipv4";
      ip = addrToIPv4((struct sockaddr_in*) &addr);
      port = (int)htons(((struct sockaddr_in*) &addr)->sin_port);
    } else {
      family = "ipv6";
      ip = addrToIPv6((struct sockaddr_in6*) &addr);
      port = (int)htons(((struct sockaddr_in6*) &addr)->sin6_port);
    }
  }

  static void parseAddress (struct sockaddr *name, int* port, char* ip) {
    struct sockaddr_in *name_in = (struct sockaddr_in *) name;
    *port = ntohs(name_in->sin_port);
    uv_ip4_name(name_in, ip, 17);
  }

  std::map<uint64_t, Client*> clients;
  std::map<uint64_t, Server*> servers;
  std::map<uint64_t, GenericContext*> contexts;
  std::map<uint64_t, DescriptorContext*> descriptors;

  struct sockaddr_in addr;

  typedef struct {
    uv_write_t req;
    uv_buf_t buf;
  } write_req_t;

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

  String Core::createPost (String params, Post post) {
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
      "  let headers = `" + post.headers + "`"
      "    .trim().split(/[\\r\\n]+/);"
      "  try { o = JSON.parse(o) } catch (err) {"
      "    console.error(err, `string<${o}>`)"
      "  };"
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

    req->data = desc;
    desc->seq = seq;
    desc->cb = cb;

    auto err = uv_fs_access(loop, req, filename, mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
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

      desc->cb(desc->seq, msg, Post{});
      uv_fs_req_cleanup(req);
      delete desc;
      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(
        R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
        std::to_string(err), String(uv_strerror(err))
      );

      cb(seq, msg, Post{});
      delete desc;
      delete req;
      return;
    }

    runDefaultLoop();
  }

  void Core::fsChmod (String seq, String path, int mode, Cb cb) const {
    auto filename = path.c_str();
    auto loop = getDefaultLoop();
    auto desc = new DescriptorContext;
    auto req = new uv_fs_t;

    req->data = desc;
    desc->seq = seq;
    desc->cb = cb;

    auto err = uv_fs_chmod(loop, req, filename, mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
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

      desc->cb(desc->seq, msg, Post{});
      uv_fs_req_cleanup(req);
      delete desc;
      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(
        R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
        std::to_string(err), String(uv_strerror(err))
      );

      cb(seq, msg, Post{});
      delete desc;
      delete req;
      return;
    }

    runDefaultLoop();
  }

  void Core::fsOpen (String seq, uint64_t id, String path, int flags, int mode, Cb cb) const {
    auto filename = path.c_str();
    auto loop = getDefaultLoop();
    auto desc = descriptors[id] = new DescriptorContext;
    auto req = new uv_fs_t;

    req->data = desc;

    desc->id = id;
    desc->seq = seq;
    desc->cb = cb;

    auto err = uv_fs_open(loop, req, filename, flags, mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(
          R"MSG({ "err": { "id": "$S", "message": "$S" } })MSG",
          std::to_string(desc->id),
          String(uv_strerror((int)req->result))
        );
      } else {
        desc->fd = (int) req->result;
        msg = SSC::format(
          R"MSG({ "data": { "id": "$S", "fd": $S } })MSG",
          std::to_string(desc->id),
          std::to_string(desc->fd)
        );
      }

      desc->cb(desc->seq, msg, Post{});
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
      delete req;
      return;
    }

    runDefaultLoop();
  }

  void Core::fsOpendir(String seq, uint64_t id, String path, Cb cb) const {
    auto filename = path.c_str();
    auto loop = getDefaultLoop();
    auto desc = descriptors[id] = new DescriptorContext;
    auto req = new uv_fs_t;

    req->data = desc;

    desc->id = id;
    desc->cb = cb;
    desc->dir = nullptr;
    desc->seq = seq;

    auto err = uv_fs_opendir(loop, req, filename, [](uv_fs_t *req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      std::string msg;

      if (req->result < 0) {
        msg = SSC::format(
          R"MSG({ "err": { "id": "$S", "message": "$S" } })MSG",
          std::to_string(desc->id),
          String(uv_strerror((int)req->result))
        );
      } else {
        msg = SSC::format(
          R"MSG({ "data": { "id": "$S" } })MSG",
          std::to_string(desc->id)
        );
      }

      desc->dir = (uv_dir_t *) req->ptr;
      desc->cb(desc->seq, msg, Post{});
      uv_fs_req_cleanup(req);
      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(
        R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
        std::to_string(err), String(uv_strerror(err))
      );

      cb(seq, msg, Post{});
      delete desc;
      delete req;
      return;
    }

    runDefaultLoop();
  }

  void Core::fsReaddir(String seq, uint64_t id, size_t nentries, Cb cb) const {
    auto desc = descriptors[id];

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "message": "No directory descriptor found with that id"
        }
      })MSG", std::to_string(id));

      cb(seq, msg, Post{});
      return;
    }

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
    req->data = desc;

    desc->seq = seq;
    desc->cb = cb;
    desc->dir->dirents = desc->dirents;
    desc->dir->nentries = nentries;

    auto err = uv_fs_readdir(loop, req, desc->dir, [](uv_fs_t *req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
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

      desc->cb(desc->seq, msg, Post{});
      uv_fs_req_cleanup(req);
      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(
        R"MSG({ "err": { "code": $S, "message": "$S" } })MSG",
        std::to_string(err), String(uv_strerror(err))
      );

      cb(seq, msg, Post{});
      delete desc;
      delete req;
      return;
    }

    runDefaultLoop();
  }

  void Core::fsClose (String seq, uint64_t id, Cb cb) const {
    auto desc = descriptors[id];

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));

      cb(seq, msg, Post{});
      return;
    }

    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_close(getDefaultLoop(), req, desc->fd, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      std::string msg;

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
      }

      desc->cb(desc->seq, msg, Post{});

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

    descriptors.erase(id);
  }

  void Core::fsClosedir (String seq, uint64_t id, Cb cb) const {
    auto desc = descriptors[id];

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "message": "No directory descriptor found with that id"
        }
      })MSG", std::to_string(id));

      cb(seq, msg, Post{});
      return;
    }

    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_closedir(getDefaultLoop(), req, desc->dir, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      std::string msg;

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
      }

      desc->cb(desc->seq, msg, Post{});

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

    descriptors.erase(id);
  }

  void Core::fsRead (String seq, uint64_t id, int len, int offset, Cb cb) const {
    auto desc = descriptors[id];

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

    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto buf = new char[len];
    const uv_buf_t iov = uv_buf_init(buf, len * sizeof(char));
    desc->data = buf;

    auto err = uv_fs_read(getDefaultLoop(), req, desc->fd, &iov, 1, offset, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      std::string msg = "{}";
      Post post = {0};

      if (req->result < 0) {
        msg = SSC::format(R"MSG({
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(desc->id), String(uv_strerror((int)req->result)));
      } else {
        auto headers = SSC::format(R"MSG(
          content-type: application/octet-stream
          content-length: $S
          event: fsRead
          id: $S
        )MSG", std::to_string(req->result), std::to_string(desc->id));

        post.body = (char *) desc->data;
        post.length = (int) req->result;
        post.headers = headers;
        post.bodyNeedsFree = true;
      }

      desc->data = 0;
      desc->cb(desc->seq, msg, post);

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
    auto desc = descriptors[id];

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

    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    const uv_buf_t buf = uv_buf_init((char*) data.data(), (int) data.size());

    auto err = uv_fs_write(uv_default_loop(), req, desc->fd, &buf, 1, offset, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      std::string msg;

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

      desc->cb(desc->seq, msg, Post{});
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

  void Core::fsStat (String seq, String path, Cb cb) const {
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_stat(getDefaultLoop(), req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
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

      desc->cb(desc->seq, msg, Post{});

      uv_fs_req_cleanup(req);
      delete desc;
      delete req;
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      cb(seq, msg, Post{});

      delete desc;
      delete req;

      return;
    }

    runDefaultLoop();
  }

  void Core::fsFStat (String seq, uint64_t id, Cb cb) const {
    auto desc = descriptors[id];

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

    desc->seq = seq;
    desc->cb = cb;

    auto req = new uv_fs_t;
    req->data = desc;

    auto err = uv_fs_fstat(getDefaultLoop(), req, desc->fd, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
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

      desc->cb(desc->seq, msg, Post{});
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
      delete req;
      return;
    }

    runDefaultLoop();
  }

  void Core::fsUnlink (String seq, String path, Cb cb) const {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_unlink(getDefaultLoop(), &req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto msg = SSC::format(R"MSG({
        "data": {
          "id": "$S",
          "result": "$i"
        }
      })MSG", std::to_string(desc->id), (int)req->result);

      desc->cb(desc->seq, msg, Post{});

      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      cb(seq, msg, Post{});
      return;
    }
    runDefaultLoop();
  }

  void Core::fsRename (String seq, String pathA, String pathB, Cb cb) const {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_rename(getDefaultLoop(), &req, (const char*) pathA.c_str(), (const char*) pathB.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto msg = SSC::format(R"MSG({
        "data": {
          "id": "$S",
          "result": "$i"
        }
      })MSG", std::to_string(desc->id), (int)req->result);

      desc->cb(desc->seq, msg, Post{});

      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsCopyFile (String seq, String pathA, String pathB, int flags, Cb cb) const {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_copyfile(getDefaultLoop(), &req, (const char*) pathA.c_str(), (const char*) pathB.c_str(), flags, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      auto msg = SSC::format(R"MSG({
        "data": {
          "id": "$S",
          "result": "$i"
        }
      })MSG", std::to_string(desc->id), (int)req->result);

      desc->cb(desc->seq, msg, Post{});
      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));

      cb(seq, msg, Post{});
      return;
    }
    runDefaultLoop();
  }

  void Core::fsRmDir (String seq, String path, Cb cb) const {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_rmdir(getDefaultLoop(), &req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      auto msg = SSC::format(R"MSG({
        "data": {
          "id": "$S",
          "result": "$i"
        }
      })MSG", std::to_string(desc->id), (int)req->result);

      desc->cb(desc->seq, msg, Post{});
      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::fsMkDir (String seq, String path, int mode, Cb cb) const {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_mkdir(getDefaultLoop(), &req, (const char*) path.c_str(), mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto msg = SSC::format(R"MSG({
        "data": {
          "id": "$S",
          "result": "$i"
        }
      })MSG", std::to_string(desc->id), (int)req->result);

      desc->cb(desc->seq, msg, Post{});

      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "message": "$S"
        }
      })MSG", String(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::sendBufferSize (String seq, uint64_t clientId, int size, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "method": "Cb",
          "message": "Not connected"
        }
      })MSG", std::to_string(clientId));

      cb(seq, msg, Post{});
      return;
    }

    uv_handle_t* handle;

    if (client->tcp != nullptr) {
      handle = (uv_handle_t*) client->tcp;
    } else {
      handle = (uv_handle_t*) client->udp;
    }

    int sz = size;
    int rSize = uv_send_buffer_size(handle, &sz);

    auto msg = SSC::format(R"MSG({
      "data": {
        "clientId": "$S",
        "method": "Cb",
        "size": $i
      }
    })MSG", std::to_string(clientId), rSize);

    cb(seq, msg, Post{});
    return;
  }

  void Core::recvBufferSize (String seq, uint64_t clientId, int size, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "client": "$S",
          "method": "Cb",
          "message": "Not connected"
        }
      })MSG", std::to_string(clientId));
      cb(seq, msg, Post{});
      return;
    }

    uv_handle_t* handle;

    if (client->tcp != nullptr) {
      handle = (uv_handle_t*) client->tcp;
    } else {
      handle = (uv_handle_t*) client->udp;
    }

    int sz = size;
    int rSize = uv_recv_buffer_size(handle, &sz);

    auto msg = SSC::format(R"MSG({
      "data": {
        "clientId": "$S",
        "method": "Cb",
        "size": $i
      }
    })MSG", std::to_string(clientId), rSize);

    cb(seq, msg, Post{});
    return;
  }

  void Core::tcpSend (uint64_t clientId, String message, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "method": "emit",
          "message": "Not connected"
        }
      })MSG", std::to_string(clientId));

      cb("-1", msg, Post{});
      return;
    }

    GenericContext* ctx = contexts[clientId] = new GenericContext;
    ctx->id = clientId;
    ctx->cb = cb;

    write_req_t *wr = (write_req_t*) malloc(sizeof(write_req_t));
    memset(wr, 0, sizeof(write_req_t));
    wr->req.data = ctx;
    wr->buf = uv_buf_init((char* const) message.c_str(), (int)message.size());

    auto onWrite = [](uv_write_t *req, int status) {
      auto ctx = reinterpret_cast<GenericContext*>(req->data);

      if (status) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "clientId": "$S",
            "method": "emit",
            "message": "Write error $S"
          }
        })MSG", std::to_string(ctx->id), uv_strerror(status));

        ctx->cb("-1", msg, Post{});
        return;
      }

      write_req_t *wr = (write_req_t*) req;
      free(wr->buf.base);
      free(wr);
    };

    uv_write((uv_write_t*) wr, (uv_stream_t*) client->tcp, &wr->buf, 1, onWrite);
    runDefaultLoop();
  }

  void Core::tcpConnect (String seq, uint64_t clientId, int port, String ip, Cb cb) const {
    uv_connect_t connect;

    Client* client = clients[clientId] = new Client();
    client->cb = cb;
    client->clientId = clientId;
    client->tcp = new uv_tcp_t;

    uv_tcp_init(getDefaultLoop(), client->tcp);

    client->tcp->data = client;

    uv_tcp_nodelay(client->tcp, 0);
    uv_tcp_keepalive(client->tcp, 1, 60);

    struct sockaddr_in dest4;
    struct sockaddr_in6 dest6;

    // check to validate the ip is actually an ipv6 address with a regex
    if (ip.find(":") != String::npos) {
      uv_ip6_addr(ip.c_str(), port, &dest6);
    } else {
      uv_ip4_addr(ip.c_str(), port, &dest4);
    }

    // uv_ip4_addr("172.217.16.46", 80, &dest);
    // NSLog(@"connect? %s:%i", ip.c_str(), port);

    auto onConnect = [](uv_connect_t* connect, int status) {
      auto* client = reinterpret_cast<Client*>(connect->handle->data);

      // NSLog(@"client connection?");

      if (status < 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "clientId": "$S",
            "method": "emit",
            "message": "$S"
          }
        })MSG", std::to_string(client->clientId), String(uv_strerror(status)));
        client->cb("-1", msg, Post{});
        return;
      }

      auto msg = SSC::format(R"MSG({
        "data": {
          "clientId": "$S",
          "method": "emit",
          "message": "connection"
        }
      })MSG", std::to_string(client->clientId));

      client->cb("-1", msg, Post{});

      auto onRead = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
        auto client = reinterpret_cast<Client*>(handle->data);
        auto clientId = std::to_string(client->clientId);

        auto headers = SSC::format(R"MSG(
          content-type: application/octet-stream
          content-length: $i
          clientId: $S
          event: tcpConnect
        )MSG", (int)buf->len, clientId);

        Post post;
        post.body = buf->base;
        post.length = (int)buf->len;
        post.headers = headers;

        client->cb("-1", "{}", post);
      };

      auto allocate = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = (char*) malloc(suggested_size);
        buf->len = suggested_size;
        memset(buf->base, 0, buf->len);
      };

      uv_read_start((uv_stream_t*) connect->handle, allocate, onRead);
    };

    int r = 0;

    if (ip.find(":") != String::npos) {
      r = uv_tcp_connect(&connect, client->tcp, (const struct sockaddr*) &dest6, onConnect);
    } else {
      r = uv_tcp_connect(&connect, client->tcp, (const struct sockaddr*) &dest4, onConnect);
    }

    if (r) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "method": "Cb",
          "message": "$S"
        }
      })MSG", std::to_string(clientId), String(uv_strerror(r)));
      cb(seq, msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::tcpSetTimeout (String seq, uint64_t clientId, int timeout, Cb cb) const {
    // TODO impl
  }

  void Core::tcpBind (String seq, uint64_t serverId, String ip, int port, Cb cb) const {
    Server* server = servers[serverId] = new Server();
    server->tcp = new uv_tcp_t;
    server->cb = cb;
    server->serverId = serverId;
    server->tcp->data = &server;

    uv_tcp_init(getDefaultLoop(), server->tcp);
    struct sockaddr_in addr;

    // addr.sin_port = htons(port);
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // NSLog(@"LISTENING %i", addr.sin_addr.s_addr);
    // NSLog(@"LISTENING %s:%i", ip.c_str(), port);

    uv_ip4_addr(ip.c_str(), port, &addr);
    uv_tcp_simultaneous_accepts(server->tcp, 0);
    uv_tcp_bind(server->tcp, (const struct sockaddr*) &addr, 0);

    int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, [](uv_stream_t* handle, int status) {
      auto* server = reinterpret_cast<Server*>(handle->data);

      if (status < 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "serverId": "$S",
            "method": "emit",
            "message": "connection error $S"
          }
        })MSG", std::to_string(server->serverId), uv_strerror(status));
        server->cb("-1", msg, Post{});
        return;
      }

      auto clientId = SSC::rand64();
      Client* client = clients[clientId] = new Client();
      client->clientId = clientId;
      client->server = server;
      client->stream = handle;
      client->tcp = new uv_tcp_t;

      client->tcp->data = client;

      uv_tcp_init(getDefaultLoop(), client->tcp);

      if (uv_accept(handle, (uv_stream_t*) handle) == 0) {
        PeerInfo info;
        info.init(client->tcp);

        auto msg = SSC::format(
          R"MSG({
            "data": {
              "serverId": "$S",
              "clientId": "$S",
              "ip": "$S",
              "family": "$S",
              "port": "$i"
            }
          })MSG",
          std::to_string(server->serverId),
          std::to_string(clientId),
          info.ip,
          info.family,
          info.port
        );

        server->cb("-1", msg, Post{});
      } else {
        uv_close((uv_handle_t*) handle, [](uv_handle_t* handle) {
          free(handle);
        });
      }
    });

    if (r) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "serverId": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(server->serverId), String(uv_strerror(r)));
      cb(seq, msg, Post{});

      // NSLog(@"Listener failed: %s", uv_strerror(r));
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {
        "serverId": "$S",
        "port": $i,
        "ip": "$S"
      }
    })MSG", std::to_string(server->serverId), port, ip);

    cb(seq, msg, Post{});
    // NSLog(@"Listener started");
    runDefaultLoop();
  }

  void Core::tcpSetKeepAlive (String seq, uint64_t clientId, int timeout, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "message": "No connection found with the specified id"
        }
      })MSG", std::to_string(clientId));

      cb(seq, msg, Post{});
      return;
    }

    client->seq = seq;
    client->cb = cb;
    client->clientId = clientId;

    uv_tcp_keepalive((uv_tcp_t*) client->tcp, 1, timeout);

    auto msg = SSC::format(R"MSG({
      "data": {}
    })MSG");

    client->cb(client->seq, msg, Post{});
  }

  void Core::tcpReadStart (String seq, uint64_t clientId, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "message": "No connection found with the specified id"
        }
      })MSG", std::to_string(clientId));
      cb(seq, msg, Post{});
      return;
    }

    client->seq = seq;
    client->cb = cb;

    auto onRead = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf) {
      auto client = reinterpret_cast<Client*>(handle->data);

      if (nread == UV_EOF) {
        auto msg = SSC::format(R"MSG({
          "serverId": "$S",
          "EOF": true
        })MSG", std::to_string(client->server->serverId));

        client->server->cb("-1", msg, Post{});
        return;
      }

      if (nread > 0) {
        write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, (int)nread);

        auto serverId = std::to_string(client->server->serverId);
        auto clientId = std::to_string(client->clientId);

        auto headers = SSC::format(R"MSG(
          content-type: application/octet-stream
          serverId: $S
          clientId: $S
          read: $i
          event: tcpReadStart
        )MSG", serverId, clientId, (int)nread);

        Post post;
        post.body = buf->base;
        post.length = (int)buf->len;
        post.headers = headers;

        client->server->cb("-1", "{}", post);
        return;
      }

      if (nread < 0) {
        uv_close((uv_handle_t*) client->tcp, [](uv_handle_t* handle) {
          free(handle);
        });
      }

      free(buf->base);
    };

    auto allocateBuffer = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      buf->len = suggested_size;
      memset(buf->base, 0, buf->len);
    };

    int err = uv_read_start((uv_stream_t*) client->stream, allocateBuffer, onRead);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "serverId": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(client->server->serverId), uv_strerror(err));

      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({ "data": {} })MSG");
    client->server->cb(client->server->seq, msg, Post{});

    runDefaultLoop();
  }

  void Core::readStop (String seq, uint64_t clientId, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "message": "No connection with specified id"
        }
      })MSG", std::to_string(clientId));
      cb(seq, msg, Post{});
      return;
    }

    int r;

    if (client->tcp) {
      r = uv_read_stop((uv_stream_t*) client->tcp);
    } else {
      r = uv_read_stop((uv_stream_t*) client->udp);
    }

    auto msg = SSC::format(R"MSG({ "data": $i })MSG", r);
    cb(seq, msg, Post{});
  }

 void Core::close (String seq, uint64_t clientId, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "message": "No connection with specified id"
        }
      })MSG", std::to_string(clientId));
      cb(seq, msg, Post{});
      return;
    }

    client->seq = seq;
    client->cb = cb;
    client->clientId = clientId;

    uv_handle_t* handle;

    if (client->tcp != nullptr) {
      handle = (uv_handle_t*) client->tcp;
    } else {
      handle = (uv_handle_t*) client->udp;
    }

    handle->data = client;

    uv_close(handle, [](uv_handle_t* handle) {
      auto client = reinterpret_cast<Client*>(handle->data);

      auto msg = SSC::format(R"MSG({ "data": {} })MSG");
      client->cb(client->seq, msg, Post{});
      free(handle);
    });

    runDefaultLoop();
  }

  void Core::shutdown (String seq, uint64_t clientId, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(
        R"MSG({
          "err": {
            "clientId": "$S",
            "message": "No connection with specified id"
          }
        })MSG",
        std::to_string(clientId)
      );

      cb(seq, msg, Post{});
      return;
    }

    client->seq = seq;
    client->cb = cb;
    client->clientId = clientId;

    uv_handle_t* handle;

    if (client->tcp != nullptr) {
     handle = (uv_handle_t*) client->tcp;
    } else {
     handle = (uv_handle_t*) client->udp;
    }

    handle->data = client;

    uv_shutdown_t *req = new uv_shutdown_t;
    req->data = handle;

    uv_shutdown(req, (uv_stream_t*) handle, [](uv_shutdown_t *req, int status) {
      auto client = reinterpret_cast<Client*>(req->handle->data);

      auto msg = SSC::format(R"MSG({
        "data": {
          "status": "$i"
        }
      })MSG", status);

     client->cb(client->seq, msg, Post{});

     free(req);
     free(req->handle);
    });

    runDefaultLoop();
  }

  void Core::udpBind (String seq, uint64_t serverId, String ip, int port, Cb cb) const {
    Server* server = servers[serverId] = new Server();
    server->udp = new uv_udp_t;
    server->seq = seq;
    server->serverId = serverId;
    server->cb = cb;
    server->udp->data = server;

    int err;
    struct sockaddr_in addr;

    err = uv_ip4_addr((char*) ip.c_str(), port, &addr);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp",
        "err": {
          "serverId": "$S",
          "message": "uv_ip4_addr: $S"
        }
      })MSG", std::to_string(serverId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    uv_udp_init(getDefaultLoop(), server->udp);
    err = uv_udp_bind(server->udp, (const struct sockaddr*)&addr, UV_UDP_REUSEADDR);

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "source": "udp",
        "err": {
          "serverId": "$S",
          "message": "uv_udp_bind: $S"
        }
      })MSG", std::to_string(server->serverId), std::string(uv_strerror(err)));
      server->cb("-1", msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({
      "data": {}
    })MSG");

    server->cb(server->seq, msg, Post{});

    runDefaultLoop();
  }

  void Core::udpSend (String seq, uint64_t clientId, String message, int offset, int len, int port, const char* ip, Cb cb) const {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "message": "no such client"
        }
      })MSG", std::to_string(clientId));
      cb(seq, msg, Post{});
      return;
    }

    client->cb = cb;
    client->seq = seq;

    int err;
    uv_udp_send_t* req = new uv_udp_send_t;
    req->data = client;

    err = uv_ip4_addr((char*) ip, port, &addr);

    if (err) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "clientId": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(clientId), uv_strerror(err));
      cb(seq, msg, Post{});
      return;
    }

    uv_buf_t bufs[1];
    char* base = (char*) message.c_str();
    bufs[0] = uv_buf_init(base + offset, len);

    err = uv_udp_send(req, client->udp, bufs, 1, (const struct sockaddr *) &addr, [] (uv_udp_send_t *req, int status) {
      auto client = reinterpret_cast<Client*>(req->data);

      auto msg = SSC::format(R"MSG({
        "data": {
          "clientId": "$S",
          "status": "$i"
        }
      })MSG", std::to_string(client->clientId), status);

      client->cb(client->seq, msg, Post{});

      delete[] req->bufs;
      free(req);
    });

    if (err) {
      auto msg = SSC::format(R"MSG({
        "data": {
          "clientId": "$S",
          "message": "Write error $S"
        }
      })MSG", std::to_string(client->clientId), uv_strerror(err));
      client->cb("-1", msg, Post{});
      return;
    }

    runDefaultLoop();
  }

  void Core::udpReadStart (String seq, uint64_t serverId, Cb cb) const {
    Server* server = servers[serverId];

    if (server == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "serverId": "$S",
          "message": "no such server"
        }
      })MSG", std::to_string(serverId));
      cb(seq, msg, Post{});
      return;
    }

    server->cb = cb;
    server->seq = seq;

    auto allocate = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      buf->len = suggested_size;
      memset(buf->base, 0, buf->len);
    };

    int err = uv_udp_recv_start(server->udp, allocate, [](uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
      Server *server = (Server*)handle->data;

      if (nread == UV_EOF) {
        auto msg = SSC::format(R"MSG({
          "serverId": "$S",
          "EOF": true
        })MSG", std::to_string(server->serverId));
        server->cb("-1", msg, Post{});
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
        )MSG", (int)buf->len);

        Post post;
        post.body = buf->base;
        post.length = (int)buf->len;
        post.headers = headers;

        auto msg = SSC::format(R"MSG(
          "serverId": "$S",
          "event": "udpReadStart",
          "port": $i,
          "ip": "$S"
        )MSG", std::to_string(server->serverId), port, ip);

        server->cb("-1", msg, post);
      }
    });

    if (err < 0) {
      auto msg = SSC::format(R"MSG({
        "err": {
          "serverId": "$S",
          "message": "$S"
        }
      })MSG", std::to_string(serverId), std::string(uv_strerror(err)));
      cb(seq, msg, Post{});
      return;
    }

    auto msg = SSC::format(R"MSG({ "data": {} })MSG");
    server->cb(server->seq, msg, Post{});
    runDefaultLoop();
  }

  void Core::dnsLookup (String seq, String hostname, Cb cb) const {
    auto ctxId = SSC::rand64();
    GenericContext* ctx = contexts[ctxId] = new GenericContext;
    ctx->id = ctxId;
    ctx->cb = cb;
    ctx->seq = seq;

    struct addrinfo hints;
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

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
        })MSG", String(uv_err_name((int)status)), String(uv_strerror(status)));
        ctx->cb(ctx->seq, msg, Post{});
        contexts.erase(ctx->id);
        return;
      }

      char addr[17] = {'\0'};
      uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
      String ip(addr, 17);

      auto msg = SSC::format(R"MSG({ "data": "$S" })MSG", ip);
      ctx->cb(ctx->seq, msg, Post{});
      contexts.erase(ctx->id);

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
