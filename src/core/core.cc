#include "core.hh"
namespace SSC {
  static Core *instance = nullptr;

  Core::Core () {
    this->tasks = std::unique_ptr<Tasks>(new Tasks());
    this->posts = std::unique_ptr<Posts>(new Posts());
    initEventLoop();
  }

  void Core::handleEvent (String seq, String event, String data, Callback cb) {
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
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    std::vector<uint64_t> ids;
    auto now = std::chrono::system_clock::now()
      .time_since_epoch()
      .count();

    for (auto const &tuple : *posts) {
      auto id = tuple.first;
      auto post = tuple.second;

      if (post.ttl < now) {
        ids.push_back(id);
      }
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }

  void Core::putPost (uint64_t id, Post p) {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    p.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(32 * 1024)
    )
      .time_since_epoch()
      .count();
    posts->insert_or_assign(id, p);
  }

  void Core::removePost (uint64_t id) {
    std::lock_guard<std::recursive_mutex> guard(postsMutex);
    if (posts->find(id) == posts->end()) return;
    auto post = getPost(id);

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

    putPost(post.id, post);
    return js;
  }

  void Core::removeAllPosts () {
      std::lock_guard<std::recursive_mutex> guard(postsMutex);
    std::vector<uint64_t> ids;
    auto now = std::chrono::system_clock::now()
      .time_since_epoch()
      .count();

    for (auto const &tuple : *posts) {
      auto id = tuple.first;
      ids.push_back(id);
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }

  String Core::getNetworkInterfaces () {
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

  void Core::dnsLookup (String seq, String hostname, int family, Callback cb) {
    dispatchEventLoop([=, this]() {
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

  void Core::bufferSize (String seq, uint64_t peerId, int size, int buffer, Callback cb) {
    static int RECV_BUFFER = 1;
    static int SEND_BUFFER = 0;

    if (buffer < 0) {
      buffer = 0;
    } else if (buffer > 1) {
      buffer = 1;
    }

    dispatchEventLoop([=, this]() {
      auto peer = getPeer(peerId);

      if (peer == nullptr) {
        auto msg = SSC::format(R"MSG({
          "source": "bufferSize",
          "err": {
            "id": "$S",
            "code": "NOT_FOUND_ERR",
            "type": "NotFoundError",
            "message": "No peer with specified id"
          }
        })MSG", std::to_string(peerId));
        cb(seq, msg, Post{});
        return;
      }

      std::lock_guard<std::recursive_mutex> guard(peer->mutex);
      auto handle = (uv_handle_t*) &peer->handle;
      auto err = buffer == RECV_BUFFER
       ? uv_recv_buffer_size(handle, (int *) &size)
       : uv_send_buffer_size(handle, (int *) &size);

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "bufferSize",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG",
        std::to_string(peerId),
        std::string(uv_strerror(err)));
        cb(seq, msg, Post{});
        return;
      }

      auto msg = SSC::format(R"MSG({
        "source": "bufferSize",
        "data": {
          "id": "$S",
          "size": $i
        }
      })MSG", std::to_string(peerId), size);

      cb(seq, msg, Post{});
    });
  }

  void Core::close (String seq, uint64_t peerId, Callback cb) {
    dispatchEventLoop([=, this]() {
      if (!hasPeer(peerId)) {
        auto msg = SSC::format(R"MSG({
          "source": "close",
          "err": {
            "id": "$S",
            "code": "NOT_FOUND_ERR",
            "type": "NotFoundError",
            "message": "No peer with specified id"
          }
        })MSG", std::to_string(peerId));
        cb(seq, msg, Post{});
        return;
      }

      auto peer = getPeer(peerId);

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

      peer->close([=, this]() {
        auto msg = SSC::format(R"MSG({
          "source": "close",
          "data": {
            "id": "$S"
          }
        })MSG", std::to_string(peerId));
        cb(seq, msg, Post{});
      });
    });
  }
}
