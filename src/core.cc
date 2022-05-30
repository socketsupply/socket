#include "common.hh"
#import <Network/Network.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include "include/uv.h"

namespace SSC {
  using callback = std::function<void(std::string, std::string)>;

  class Core {
    public:
      void fsOpen (std::string seq, uint64_t id, std::string path, int flags, callback cb);
      void fsClose (std::string seq, uint64_t id, callback cb);
      void fsRead (std::string seq, uint64_t id, int len, int offset, callback cb);
      void fsWrite (std::string seq, uint64_t id, std::string data, int64_t offset, callback cb);
      void fsStat (std::string seq, std::string path, callback cb);
      void fsUnlink (std::string seq, std::string path, callback cb);
      void fsRename (std::string seq, std::string pathA, std::string pathB, callback cb);
      void fsCopyFile (std::string seq, std::string pathA, std::string pathB, int flags, callback cb);
      void fsRmDir (std::string seq, std::string path, callback cb);
      void fsMkDir (std::string seq, std::string path, int mode, callback cb);
      void fsReadDir (std::string seq, std::string path, callback cb);

      void tcpBind (std::string seq, uint64_t serverId, std::string ip, int port, callback cb);
      void tcpConnect (std::string seq, uint64_t clientId, int port, std::string ip, callback cb);
      void tcpSetTimeout (std::string seq, uint64_t clientId, int timeout, callback cb);
      void tcpSetKeepAlive (std::string seq, uint64_t clientId, int timeout, callback cb);
      void tcpSend (uint64_t clientId, std::string message, callback cb);
      void tcpReadStart (std::string seq, uint64_t clientId, callback cb);

      void udpBind (std::string seq, uint64_t serverId, std::string ip, int port, callback cb);
      void udpSend (std::string seq, uint64_t clientId, std::string message, int offset, int len, int port, const char* ip, callback cb);
      void udpReadStart (std::string seq, uint64_t serverId, callback cb);

      void sendBufferSize (std::string seq, uint64_t clientId, int size, callback cb);
      void recvBufferSize (std::string seq, uint64_t clientId, int size, callback cb);
      void close (std::string seq, uint64_t clientId, callback cb);
      void shutdown (std::string seq, uint64_t clientId, callback cb);
      void readStop (std::string seq, uint64_t clientId, callback cb);

      void dnsLookup (std::string seq, std::string hostname, callback cb);
  }

  struct GenericContext {
    callback cb;
    uint64_t id;
    std::string seq;
  };

  struct DescriptorContext {
    uv_file fd;
    std::string seq;
    callback cb;
    uint64_t id;
  };

  struct DirectoryReader {
    uv_dirent_t dirents;
    uv_dir_t* dir;
    uv_fs_t reqOpendir;
    uv_fs_t reqReaddir;
    callback cb;
    std::string seq;
  };

  struct Peer {
    callback cb;
    std::string seq;

    uv_tcp_t* tcp;
    uv_udp_t* udp;
    uv_stream_t* stream;

    ~Peer () {
      delete this->tcp;
      delete this->udp;
      delete this->udx;
    };
  };

  struct Server : public Peer {
    uint64_t serverId;
  };

  struct Client : public Peer {
    Server* server;
    uint64_t clientId;
  };

  std::string addrToIPv4 (struct sockaddr_in* sin) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin->sin_addr, buf, INET_ADDRSTRLEN);
    return std::string(buf);
  }

  std::string addrToIPv6 (struct sockaddr_in6* sin) {
    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sin->sin6_addr, buf, INET6_ADDRSTRLEN);
    return std::string(buf);
  }

  struct PeerInfo {
    std::string ip = "";
    std::string family = "";
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
      port = (int) htons(((struct sockaddr_in*) &addr)->sin_port);
    } else {
      family = "ipv6";
      ip = addrToIPv6((struct sockaddr_in6*) &addr);
      port = (int) htons(((struct sockaddr_in6*) &addr)->sin6_port);
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
      port = (int) htons(((struct sockaddr_in*) &addr)->sin_port);
    } else {
      family = "ipv6";
      ip = addrToIPv6((struct sockaddr_in6*) &addr);
      port = (int) htons(((struct sockaddr_in6*) &addr)->sin6_port);
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

  using UDXSendRequest = udx_socket_send_t;
  using UDXWriteRequest = udx_stream_write_t;

  struct sockaddr_in addr;

  typedef struct {
    uv_write_t req;
    uv_buf_t buf;
  } write_req_t;

  static uv_loop_t *loop = uv_default_loop();

  void loopCheck () {
    if (uv_loop_alive(loop) == 0) {
      uv_run(loop, UV_RUN_DEFAULT);
    }
  }

  void Core::fsOpen (std::string seq, uint64_t id, std::string path, int flags, callback cb) {
    auto desc = descriptors[id] = new DescriptorContext;
    desc->id = id;
    desc->seq = seq;
    desc->cb = cb;

    uv_fs_t req;
    req.data = desc;

    int fd = uv_fs_open(loop, &req, (char*) path.c_str(), flags, 0, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      auto json = SSC::format(R"JSON({
        "value": {
          "data": {
            "fd": $S
          }
        }
      })JSON", std::to_string(desc->id));

      desc->cb(desc->seq, json);
      uv_fs_req_cleanup(req);
    });

    if (fd < 0) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "id": "$S",
            "message": "$S"
          }
        }
      })", std::to_string(id), std::string(uv_strerror(fd)));

      cb(seq, json);
      return;
    }

    desc->fd = fd;
    loopCheck();
  }

  void Core::fsClose (std::string seq, uint64_t id, callback cb) {
    auto desc = descriptors[id];
    desc->seq = seq;
    desc->cb = cb;

    if (desc == nullptr) {
      auto json = SSC::format(R"JSON({
        "value": {
          "err": {
            "code": "ENOTOPEN",
            "message": "No file descriptor found with that id"
          }
        }
      })JSON");
      
      cb(seq, json);
      return;
    }

    uv_fs_t req;
    req.data = desc;

    int err = uv_fs_close(loop, &req, desc->fd, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto json = SSC::format(R"JSON({
        "value": {
          "data": {
            "fd": $S
          }
        }
      })JSON", std::to_string(desc->id));
      
      desc->cb(desc->seq, json);
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "id": "$S",
            "message": "$S"
          }
        }
      })", std::to_string(id), std::string(uv_strerror(err)));

      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::fsRead (std::string seq, uint64_t id, int len, int offset, callback cb) {
    auto desc = descriptors[id];

    if (desc == nullptr) {
      auto json = SSC::format(R"JSON({
        "value": {
          "err": {
            "code": "ENOTOPEN",
            "message": "No file descriptor found with that id"
          }
        }
      })JSON");

      cb(seq, json);
      return;
    }

    desc->seq = seq;
    desc->cb = cb;

    uv_fs_t req;
    req.data = desc;

    auto buf = new char[len];
    const uv_buf_t iov = uv_buf_init(buf, (int) len);

    int err = uv_fs_read(loop, &req, desc->fd, &iov, 1, offset, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      if (req->result < 0) {
        auto json = SSC::format(R"JSON({
          "value": {
            "err": {
              "code": "ENOTOPEN",
              "message": "No file descriptor found with that id"
            }
          }
        })JSON");
        
        desc->cb(desc->seq, json);
        return;
      }

      char *data = req->bufs[0].base;

      NSString* str = [NSString stringWithUTF8String: data];
      NSData *nsdata = [str dataUsingEncoding:NSUTF8StringEncoding];
      NSString *base64Encoded = [nsdata base64EncodedStringWithOptions:0];

      auto message = std::string([base64Encoded UTF8String]);

      auto json = SSC::format(R"({
        "value": {
          "data": {
            "id": "$S",
            "result": "$i",
            "data": "$S"
          }
        }
      })", std::to_string(desc->id), (int)req->result, message);
      
      desc->cb(desc->seq, json);
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "id": "$S",
            "message": "$S"
          }
        }
      })", std::to_string(id), std::string(uv_strerror(err)));

      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::fsWrite (std::string seq, uint64_t id, std::string data, int64_t offset, callback cb) {
    auto desc = descriptors[id];

    if (desc == nullptr) {
      auto json = SSC::format(R"JSON({
        "value": {
          "err": {
            "code": "ENOTOPEN",
            "message": "No file descriptor found with that id"
          }
        }
      })JSON");

      cb(seq, json);
      return;
    }

    desc->seq = seq;
    desc->cb = cb;

    uv_fs_t req;
    req.data = desc;

    const uv_buf_t buf = uv_buf_init((char*) data.c_str(), (int) data.size());

    int err = uv_fs_write(uv_default_loop(), &req, desc->fd, &buf, 1, offset, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      auto json = message: SSC::format(R"({
        "value": {
          "data": {
            "id": "$S",
            "result": "$i"
          }
        }
      })", std::to_string(desc->id), (int)req->result);
      
      desc->cb(desc->seq, json);
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "id": "$S",
            "message": "$S"
          }
        }
      })", std::to_string(id), std::string(uv_strerror(err)));

      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::fsStat (std::string seq, std::string path, callback cb) {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_stat(loop, &req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      auto json = message: SSC::format(R"({
        "value": {
          "data": {
            "id": "$S",
            "result": "$i"
          }
        }
      })", std::to_string(desc->id), (int)req->result);
      
      desc->cb(desc->seq, json);

      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::string(uv_strerror(err)));

      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::fsUnlink (std::string seq, std::string path, callback cb) {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_unlink(loop, &req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto json = SSC::format(R"({
        "value": {
          "data": {
            "id": "$S",
            "result": "$i"
          }
        }
      })", std::to_string(desc->id), (int)req->result);

      desc->cb(desc->seq, json);

      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::string(uv_strerror(err)));
      
      cb(seq, json);
      return;
    }
    loopCheck();
  }

  void Core::fsRename (std::string seq, std::string pathA, std::string pathB, callback cb) {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_rename(loop, &req, (const char*) pathA.c_str(), (const char*) pathB.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto json = SSC::format(R"({
        "value": {
          "data": {
            "id": "$S",
            "result": "$i"
          }
        }
      })", std::to_string(desc->id), (int)req->result);

      desc->cb(desc->seq, json);

      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::string(uv_strerror(err)));

      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::fsCopyFile (std::string seq, std::string pathA, std::string pathB, int flags, callback cb) {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_copyfile(loop, &req, (const char*) pathA.c_str(), (const char*) pathB.c_str(), flags, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      auto json = SSC::format(R"({
        "value": {
          "data": {
            "id": "$S",
            "result": "$i"
          }
        }
      })", std::to_string(desc->id), (int)req->result);
      
      desc->cb(desc->seq, json);
      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::string(uv_strerror(err)));

      cb(seq, json);
      return;
    }
    loopCheck();
  }

  void Core::fsRmDir (std::string seq, std::string path, callback cb) {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_rmdir(loop, &req, (const char*) path.c_str(), [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);

      auto json = SSC::format(R"({
        "value": {
          "data": {
            "id": "$S",
            "result": "$i"
          }
        }
      })", std::to_string(desc->id), (int)req->result);
      
      desc->cb(desc->seq, json);
      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::string(uv_strerror(err)));
      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::fsMkDir (std::string seq, std::string path int mode, callback cb) {
    uv_fs_t req;
    DescriptorContext* desc = new DescriptorContext;
    desc->seq = seq;
    desc->cb = cb;
    req.data = desc;

    int err = uv_fs_mkdir(loop, &req, (const char*) path.c_str(), mode, [](uv_fs_t* req) {
      auto desc = static_cast<DescriptorContext*>(req->data);
      auto json = SSC::format(R"({
        "value": {
          "data": {
            "id": "$S",
            "result": "$i"
          }
        }
      })", std::to_string(desc->id), (int)req->result);

      desc->cb(desc->seq, json);

      delete desc;
      uv_fs_req_cleanup(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::string(uv_strerror(err)));
      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::fsReadDir (std::string seq, std::string path, callback cb) {
    DirectoryReader* ctx = new DirectoryReader;
    ctx->seq = seq;
    ctx->cb = cb;

    ctx->reqOpendir.data = ctx;
    ctx->reqReaddir.data = ctx;

    int err = uv_fs_opendir(loop, &ctx->reqOpendir, (const char*) path.c_str(), nullptr);

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::string(uv_strerror(err)));
      cb(seq, json);
      return;
    }

    err = uv_fs_readdir(loop, &ctx->reqReaddir, ctx->dir, [](uv_fs_t* req) {
      auto ctx = static_cast<DirectoryReader*>(req->data);

      if (req->result < 0) {
        auto json = SSC::format(R"({
          "value": {
            "err": {
              "message": "$S"
            }
          }
        })", std::string(uv_strerror((int)req->result)));

        ctx->cb(ctx->seq, json);
        return;
      }

      std::stringstream value;
      auto len = ctx->dir->nentries;

      for (int i = 0; i < len; i++) {
        value << "\"" << ctx->dir->dirents[i].name << "\"";

        if (i < len - 1) {
          // Assumes the user does not use commas in their file names.
          value << ",";
        }
      }
      
      auto json = SSC::format(R"({
        "value": {
          "data": "$S"
        }
      })", encodeURIComponent(value));
      
      ctx->cb(ctx->seq, json);

      uv_fs_t reqClosedir;
      uv_fs_closedir(loop, &reqClosedir, ctx->dir, [](uv_fs_t* req) {
        uv_fs_req_cleanup(req);
      });
    });

    if (err) {
      auto json = SSC::format(R"({
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::string(uv_strerror(err)));
      
      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::sendBufferSize (std::string seq, uint64_t clientId, int size, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "method": "callback",
        "value": {
          "err": {
            "message": "Not connected"
          }
        }
      })JSON", std::to_string(clientId));

      cb(seq, json);
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

    auto json = SSC::format(R"JSON({
      "clientId": "$S",
      "method": "callback",
      "value": {
        "data": {
          "size": $i
        }
      }
    })JSON", std::to_string(clientId), rSize);

    cb(seq, json);
    return;
  }

  void Core::recvBufferSize (std::string seq, uint64_t clientId, int size, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      autp json = SSC::format(R"JSON({
        "client": "$S",
        "method": "callback",
        "value": {
          "err": {
            "message": "Not connected"
          }
        }
      })JSON", std::to_string(clientId));
      cb(seq, json);
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

    auto json = SSC::format(R"JSON({
      "clientId": "$S",
      "method": "callback",
      "value": {
        "data": {
          "size": $i
        }
      }
    })JSON", std::to_string(clientId), rSize);

    cb(seq, json);
    return;
  }

  void Core::tcpSend (uint64_t clientId, std::string message, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "method": "emit",
        "value": {
          "err": {
            "message": "Not connected"
          }
        }
      })JSON", std::to_string(clientId));
      
      cb("-1", json);
      return;
    }
    
    GenericContext* ctx = contexts[clientId] = new GenericContext;
    ctx->id = clientId;
    ctx->cb = cb;

    write_req_t *wr = (write_req_t*) malloc(sizeof(write_req_t));
    wr->req.data = ctx;
    wr->buf = uv_buf_init((char* const) message.c_str(), (int) message.size());

    auto onWrite = [](uv_write_t *req, int status) {
      auto ctx = reinterpret_cast<GenericContext*>(req->data);

      if (status) {
        auto json = SSC::format(R"({
          "clientId": "$S",
          "method": "emit",
          "value": {
            "err": {
              "message": "Write error $S"
            }
          }
        })", std::to_string(ctx->clientId), uv_strerror(status));
        
        ctx->cb("-1", json);
        return;
      }

      write_req_t *wr = (write_req_t*) req;
      free(wr->buf.base);
      free(wr);
    };

    uv_write((uv_write_t*) wr, (uv_stream_t*) client->tcp, &wr->buf, 1, onWrite);
    loopCheck();
  }

  void Core::tcpConnect (std::string seq, uint64_t clientId, int port, std::string ip, callback cb) {
    uv_connect_t connect;

    Client* client = clients[clientId] = new Client();
    client->cb = cb;
    client->clientId = clientId;
    client->tcp = new uv_tcp_t;

    uv_tcp_init(loop, client->tcp);

    client->tcp->data = client;

    uv_tcp_nodelay(client->tcp, 0);
    uv_tcp_keepalive(client->tcp, 1, 60);

    struct sockaddr_in dest4;
    struct sockaddr_in6 dest6;

    // check to validate the ip is actually an ipv6 address with a regex
    if (ip.find(":") != std::string::npos) {
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
        auto json = SSC::format(R"({
          "clientId": "$S",
          "method": "emit",
          "value": {
            "err": {
              "message": "$S"
            }
          }
        })", std::to_string(client->clientId), std::string(uv_strerror(status)));
        client->cb("-1", json);
        return;
      }
      
      auto json = SSC::format(R"({
        "clientId": "$S",
        "method": "emit",
        "value": {
          "data": {
            "message": "connection"
          }
        }
      })", std::to_string(client->clientId));

      client->cb("-1", json);

      auto onRead = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
        auto client = reinterpret_cast<Client*>(handle->data);

        NSString* str = [NSString stringWithUTF8String: buf->base];
        NSData *nsdata = [str dataUsingEncoding:NSUTF8StringEncoding];
        NSString *base64Encoded = [nsdata base64EncodedStringWithOptions:0];

        auto clientId = std::to_string(client->clientId);
        auto message = std::string([base64Encoded UTF8String]);

        auto = SSC::format(R"({
          "clientId": "$S",
          "method": "emit",
          "value": {
            "data": {
              "message": "$S"
            }
          }
        })", clientId, message);
        
        client->cb("-1", json);
      };

      auto allocate = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        buf->base = (char*) malloc(suggested_size);
        buf->len = suggested_size;
      };

      uv_read_start((uv_stream_t*) connect->handle, allocate, onRead);
    };

    int r = 0;

    if (ip.find(":") != std::string::npos) {
      r = uv_tcp_connect(&connect, client->tcp, (const struct sockaddr*) &dest6, onConnect);
    } else {
      r = uv_tcp_connect(&connect, client->tcp, (const struct sockaddr*) &dest4, onConnect);
    }

    if (r) {
      auto json = SSC::format(R"({
        "clientId": "$S",
        "method": "callback",
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })", std::to_string(clientId), std::string(uv_strerror(r)));
      cb(seq, json);
      return;
    }

    loopCheck();
  }

  void Core::tcpBind (std::string seq, uint64_t serverId, std::string ip, int port, callback cb) {
    loop = uv_default_loop();

    Server* server = servers[serverId] = new Server();
    server->tcp = new uv_tcp_t;
    server->cb = cb;
    server->serverId = serverId;
    server->tcp->data = &server;

    uv_tcp_init(loop, server->tcp);
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
        server->cb("-1", SSC::format(R"JSON({
            "serverId": "$S",
            "method": "emit",
            "value": {
              "err": {
                "message": "connection error $S"
              }
            }
          })JSON", std::to_string(server->serverId), uv_strerror(status))];
        });
        return;
      }

      auto clientId = SSC::rand64();
      Client* client = clients[clientId] = new Client();
      client->clientId = clientId;
      client->server = server;
      client->stream = handle;
      client->tcp = new uv_tcp_t;

      client->tcp->data = client;

      uv_tcp_init(loop, client->tcp);

      if (uv_accept(handle, (uv_stream_t*) handle) == 0) {
        PeerInfo info;
        info.init(client->tcp);
        
        auto json = SSC::format(
          R"JSON({
            "serverId": "$S",
            "clientId": "$S",
            "value": {
              "data": {
                "ip": "$S",
                "family": "$S",
                "port": "$i"
              }
            }
          })JSON",
          std::to_string(server->serverId),
          std::to_string(clientId),
          info.ip,
          info.family,
          info.port
        );

        server->cb("-1", json);
      } else {
        uv_close((uv_handle_t*) handle, [](uv_handle_t* handle) {
          free(handle);
        });
      }
    });

    if (r) {
      auto json = SSC::format(R"JSON({
        "serverId": "$S",
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })JSON", std::to_string(server->serverId), std::string(uv_strerror(r)));
      cb(seq, json);

      NSLog(@"Listener failed: %s", uv_strerror(r));
      return;
    }

    auto json = SSC::format(R"JSON({
      "serverId": "$S",
      "value": {
        "data": {
          "port": "$i",
          "ip": "$S"
        }
      }
    })JSON", std::to_string(server->serverId), port, ip);

    cb(seq, json);
    // NSLog(@"Listener started");
    loopCheck();
  }

  void Core::tcpSetKeepAlive (std::string seq, uint64_t clientId, int timeout, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "value": {
          "err": {
            "message": "No connection found with the specified id"
          }
        }
      })JSON", std::to_string(clientId));

      cb(seq, json);
      return;
    }

    client->seq = seq;
    client->cb = cb;
    client->clientId = clientId;

    uv_tcp_keepalive((uv_tcp_t*) client->tcp, 1, timeout);

    auto json = SSC::format(R"JSON({
      "value": {
        "data": {}
      }
    })JSON");
    
    client->cb(client->seq, json);
  }

  void Core::tcpReadStart (std::string seq, uint64_t clientId, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "value": {
          "err": {
            "message": "No connection found with the specified id"
          }
        }
      })JSON", std::to_string(clientId));
      cb(seq, json);
      return;
    }

    client->seq = seq;
    client->cb = cb;

    auto onRead = [](uv_stream_t* handle, ssize_t nread, const uv_buf_t *buf) {
      auto client = reinterpret_cast<Client*>(handle->data);

      if (nread > 0) {
        write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, (int) nread);

        NSString* str = [NSString stringWithUTF8String: req->buf.base];
        NSData *nsdata = [str dataUsingEncoding:NSUTF8StringEncoding];
        NSString *base64Encoded = [nsdata base64EncodedStringWithOptions:0];

        auto serverId = std::to_string(client->server->serverId);
        auto clientId = std::to_string(client->clientId);
        auto message = std::string([base64Encoded UTF8String]);

        auto json = SSC::format(R"JSON({
          "serverId": "$S",
          "clientId": "$S",
          "value": {
            "data": "$S"
          }
        })JSON", serverId, clientId, message);

        client->server->cb("-1", json);
        return;
      }

      if (nread < 0) {
        if (nread != UV_EOF) {
          auto json = SSC::format(R"JSON({
            "serverId": "$S",
            "value": {
              "data": "$S"
            }
          })JSON", std::to_string(client->server->serverId), uv_err_name((int) nread));
          client->server->cb("-1", json);
        }

        uv_close((uv_handle_t*) client->tcp, [](uv_handle_t* handle) {
          free(handle);
        });
      }

      free(buf->base);
    };

    auto allocateBuffer = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      buf->len = suggested_size;
    };

    int err = uv_read_start((uv_stream_t*) client->stream, allocateBuffer, onRead);

    if (err < 0) {
      auto json = SSC::format(R"JSON({
        "serverId": "$S",
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })JSON", std::to_string(client->server->serverId), uv_strerror(err));

      cb(seq, json);
      return;
    }
    
    auto json = SSC::format(R"JSON({
      "value": {
        "data": {}
      }
    })JSON");

    client->server->cb(client->server->seq, json);

    loopCheck();
  }

  void Core::readStop (std::string seq, uint64_t clientId, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "value": {
          "err": {
            "message": "No connection with specified id"
          }
        }
      })JSON", std::to_string(clientId));
      cb(seq, json);
      return;
    }

    int r;

    if (client->tcp) {
      r = uv_read_stop((uv_stream_t*) client->tcp);
    } else {
      r = uv_read_stop((uv_stream_t*) client->udp);
    }
    
    auto json = SSC::format(R"JSON({
      "value": {
        "data": $i
      }
    })JSON", r);
    
    cb(seq, json);
  }

 void Core::close (std::string seq, uint64_t clientId, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "value": {
          "err": {
            "message": "No connection with specified id"
          }
        }
      })JSON", std::to_string(clientId));
      cb(seq, json);
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

      auto json = SSC::format(R"JSON({
        "value": {
          "data": {}
        }
      })JSON");
      
      client->cb(client->seq json);
      free(handle);
    });

    loopCheck();
  }

  void Core::shutdown (std::string seq, uint64_t clientId, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "value": {
          "err": {
             "message": "No connection with specified id"
            }
          }
        })JSON",
        std::to_string(clientId)
      );
      cb(seq, json);
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

      auto json = SSC::format(R"JSON({
        "value": {
          "data": {
            "status": "$i"
          }
        }
      })JSON", status)
      
     client->cb(client->seq, json);
     free(req);
     free(req->handle);
    });

    loopCheck();
  }

  void Core::udpBind (std::string seq, uint64_t serverId, std::string ip, int port, callback cb) {
    loop = uv_default_loop();
    Server* server = servers[serverId] = new Server();
    server->udp = new uv_udp_t;
    server->seq = seq;
    server->serverId = serverId;
    server->cb = cb;
    server->udp->data = server;

    int err;
    struct sockaddr_in addr;

    err = uv_ip4_addr((char *) ip.c_str(), port, &addr);

    if (err < 0) {
      auto json = SSC::format(R"JSON({
        "serverId": "$S",
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })JSON", std::to_string(serverId), uv_strerror(err));
      cb(seq, json);
      return;
    }

    err = uv_udp_bind(server->udp, (const struct sockaddr*) &addr, 0);

    if (err < 0) {
      auto json = SSC::format(R"JSON({
        "serverId": "$S",
        "value": {
          "data": "$S"
        }
      })JSON", std::to_string(server->serverId), uv_strerror(err));
      server->cb("-1", json);
      return;
    }
    
    auto json = SSC::format(R"JSON({
      "value": {
        "data": {}
      }
    })JSON");

    server->cb(server->seq, json);

    loopCheck();
  }

 void Core::udpSend (std::string seq, uint64_t clientId, std::string message, int offset, int len, int port, const char* ip, callback cb) {
    Client* client = clients[clientId];

    if (client == nullptr) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "value": {
          "err": {
            "message": "no such client"
          }
        }
      })JSON", std::to_string(clientId));
      cb(seq, json);
      return;
    }

    client->cb = cb;
    client->seq = seq;

    int err;
    uv_udp_send_t* req = new uv_udp_send_t;
    req->data = client;

    err = uv_ip4_addr((char *) ip, port, &addr);

    if (err) {
      auto json = SSC::format(R"JSON({
        "clientId": "$S",
        "value": {
          "err": {
            "message": "$S"
          }
        }
      })JSON", std::to_string(clientId), uv_strerror(err));
      cb(seq, json);
      return;
    }

    uv_buf_t bufs[1];
    char* base = (char*) message.c_str();
    bufs[0] = uv_buf_init(base + offset, len);

    err = uv_udp_send(req, client->udp, bufs, 1, (const struct sockaddr *) &addr, [] (uv_udp_send_t *req, int status) {
      auto client = reinterpret_cast<Client*>(req->data);
      
      auto json = SSC::format(R"JSON({
        "value": {
          "data": {
            "clientId": "$S",
            "status": "$i"
          }
        }
      })JSON", std::to_string(client->clientId), status);

      client->cb(client->seq, json);

      delete[] req->bufs;
      free(req);
    });

    if (err) {
      auto json = SSC::format(R"({
        "clientId": "$S",
        "value": {
          "data": {
            "message": "Write error $S"
          }
        }
      })", std::to_string(client->clientId), uv_strerror(err));
      cleint->cb("-1", json);
      return;
    }
   
    loopCheck();
  }

  void Close::udpReadStart (std::string seq, uint64_t serverId, callback cb) {
    Server* server = servers[serverId];

    if (server == nullptr) {
      auto json = SSC::format(R"JSON({
        "value": {
          "err": {
            "serverId": "$S",
            "message": "no such server"
          }
        }
      })JSON", std::to_string(serverId));
      cb(seq, json);
      return;
    }

    server->cb = cb;
    server->seq = seq;

    auto allocate = [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      buf->base = (char*) malloc(suggested_size);
      buf->len = suggested_size;
    };

    int err = uv_udp_recv_start(server->udp, allocate, [](uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
      Server *server = (Server*) handle->data;

      if (nread > 0) {
        int port;
        char ipbuf[17];
        std::string data(buf->base);
        parseAddress((struct sockaddr *) addr, &port, ipbuf);
        std::string ip(ipbuf);

        SSC::format(R"JSON({
            "serverId": "$S",
            "port": "$i",
            "ip": "$S",
            "data": "$S"
          })JSON", std::to_string(server->serverId), port, ip, data)
        
        server->cb("-1", json);
        return;
      }
    });

    if (err < 0) {
      auto json = SSC::format(R"JSON({
        "value": {
          "err": {
            "serverId": "$S",
            "message": "$S"
          }
        }
      })JSON", std::to_string(serverId), uv_strerror(err));
      cb(seq, json);
      return;
    }
    
    auto json = SSC::format(R"JSON({
      "value": {
        "data": {}
      }
    })JSON");

    server->cb(server->seq, json);
    loopCheck();
  }

  void Core::dnsLookup (std::string seq, std::string hostname, callback cb) {
    loop = uv_default_loop();

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

    uv_getaddrinfo(loop, resolver, [](uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
      auto ctx = (GenericContext*) resolver->data;

      if (status < 0) {
        auto json = SSC::format(R"JSON({
          "value": {
            "err": {
              "code": "$S",
              "message": "$S"
            }
          }
        })JSON", std::string(uv_err_name((int) status)), std::string(uv_strerror(status)));
        ctx->cb(ctx->seq, json);
        contexts.erase(ctx->id);
        return;
      }

      char addr[17] = {'\0'};
      uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
      std::string ip(addr, 17);

      auto json = SSC::format(R"JSON({
        "value": {
          "data": "$S"
        }
      })JSON", ip);

      ctx->cb(ctx->seq, json);
      contexts.erase(ctx->id);

      uv_freeaddrinfo(res);
    }, hostname.c_str(), nullptr, &hints);

    loopCheck();
  }
} // SSC
