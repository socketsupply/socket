#include "core.hh"

namespace SSC {
  static Mutex instanceMutex;
  static Core *instance = nullptr;

  Core::Core ()
    : dns(this), fs(this), os(this), platform(this), udp(this)
  {
    Lock lock(instanceMutex);
    this->posts = std::shared_ptr<Posts>(new Posts());

    if (instance == nullptr) {
      instance = this;
    }

    initEventLoop();
  }

  Core::~Core () {
    Lock lock(instanceMutex);
    if (instance == this) {
      instance = nullptr;
    }
  }

  Post Core::getPost (uint64_t id) {
    Lock lock(postsMutex);
    if (posts->find(id) == posts->end()) return Post{};
    return posts->at(id);
  }

  bool Core::hasPost (uint64_t id) {
    Lock lock(postsMutex);
    return posts->find(id) != posts->end();
  }

  void Core::expirePosts () {
    Lock lock(postsMutex);
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
    Lock lock(postsMutex);
    p.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(32 * 1024)
    )
      .time_since_epoch()
      .count();
    posts->insert_or_assign(id, p);
  }

  void Core::removePost (uint64_t id) {
    Lock lock(postsMutex);
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
    Lock lock(postsMutex);

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
    Lock lock(postsMutex);
    std::vector<uint64_t> ids;

    for (auto const &tuple : *posts) {
      auto id = tuple.first;
      ids.push_back(id);
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }

  JSON::Object Core::getNetworkInterfaces () const {
    uv_interface_address_t *infos = nullptr;
    StringStream value;
    StringStream v4;
    StringStream v6;
    int count = 0;

    int rc = uv_interface_addresses(&infos, &count);

    if (rc != 0) {
      return JSON::Object(JSON::Object::Entries {
        {"source", "os.networkInterfaces"},
        {"err", JSON::Object::Entries {
          {"type", "InternalError"},
          {"message",
            String("Unable to get network interfaces: ") + String(uv_strerror(rc))
          }
        }}
      });
    }

    JSON::Object::Entries ipv4;
    JSON::Object::Entries ipv6;
    JSON::Object::Entries data;

    for (int i = 0; i < count; ++i) {
      uv_interface_address_t info = infos[i];
      struct sockaddr_in *addr = (struct sockaddr_in*) &info.address.address4;
      char mac[18] = {0};
      snprintf(mac, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
        (unsigned char) info.phys_addr[0],
        (unsigned char) info.phys_addr[1],
        (unsigned char) info.phys_addr[2],
        (unsigned char) info.phys_addr[3],
        (unsigned char) info.phys_addr[4],
        (unsigned char) info.phys_addr[5]
      );

      if (addr->sin_family == AF_INET) {
        JSON::Object::Entries entries;
        entries["internal"] = info.is_internal == 0 ? "false" : "true";
        entries["address"] = addrToIPv4(addr);
        entries["mac"] = String(mac, 17);
        ipv4[String(info.name)] = entries;
      }

      if (addr->sin_family == AF_INET6) {
        JSON::Object::Entries entries;
        entries["internal"] = info.is_internal == 0 ? "false" : "true";
        entries["address"] = addrToIPv6((struct sockaddr_in6*) addr);
        entries["mac"] = String(mac, 17);
        ipv6[String(info.name)] = entries;
      }
    }

    uv_free_interface_addresses(infos, count);

    data["ipv4"] = ipv4;
    data["ipv6"] = ipv6;

    return JSON::Object(JSON::Object::Entries {{ "data", data }});
  }

  void Core::OS::bufferSize (String seq, uint64_t peerId, int size, int buffer, Module::Callback cb) {
    static int RECV_BUFFER = 1;
    static int SEND_BUFFER = 0;

    if (buffer < 0) {
      buffer = 0;
    } else if (buffer > 1) {
      buffer = 1;
    }

    this->core->dispatchEventLoop([=, this]() {
      auto peer = this->core->getPeer(peerId);

      if (peer == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "bufferSize"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"code", "NOT_FOUND_ERR"},
            {"type", "NotFoundError"},
            {"message", "No peer with specified id"}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      Lock lock(peer->mutex);
      auto handle = (uv_handle_t*) &peer->handle;
      auto err = buffer == RECV_BUFFER
       ? uv_recv_buffer_size(handle, (int *) &size)
       : uv_send_buffer_size(handle, (int *) &size);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "bufferSize"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"code", "NOT_FOUND_ERR"},
            {"type", "NotFoundError"},
            {"message", String(uv_strerror(err))}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto json = JSON::Object::Entries {
        {"source", "bufferSize"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"size", size}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void Core::Platform::event (String seq, String event, String data, Module::Callback cb) {
    this->core->dispatchEventLoop([=, this]() {
      // init page
      if (event == "domcontentloaded") {
        Lock lock(this->core->descriptorsMutex);

        for (auto const &tuple : this->core->descriptors) {
          auto desc = tuple.second;
          if (desc != nullptr) {
            Lock descriptorLock(desc->mutex);
            desc->stale = true;
          } else {
            this->core->descriptors.erase(tuple.first);
          }
        }
      }

      auto json = JSON::Object::Entries {
        {"source", "event"},
        {"data", JSON::Object::Entries{}}
      };

      cb(seq, json, Post{});
    });
  }


  void Core::DNS::lookup (String seq, String hostname, int family, Core::Module::Callback cb) {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new Core::Module::RequestContext(seq, cb);
      auto loop = this->core->getEventLoop();

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
        auto ctx = (Core::DNS::RequestContext*) resolver->data;

        if (status < 0) {
          auto result = JSON::Object::Entries {
            {"source", "dns.lookup"},
            {"err", JSON::Object::Entries {
              {"code", std::to_string(status)},
              {"message", String(uv_strerror(status))}
            }}
          };

          ctx->cb(ctx->seq, result, Post{});
          delete ctx;
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

        auto result = JSON::Object::Entries {
          {"source", "dns.lookup"},
          {"data", JSON::Object::Entries {
            {"address", address},
            {"family", family}
          }}
        };

        ctx->cb(ctx->seq, result, Post{});
        uv_freeaddrinfo(res);
        delete ctx;

      }, hostname.c_str(), nullptr, &hints);

      if (err < 0) {
        auto result = JSON::Object::Entries {
          {"source", "dns.lookup"},
          {"err", JSON::Object::Entries {
            {"code", std::to_string(err)},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->cb(seq, result, Post{});
        delete ctx;
      }
    });
  }
}
