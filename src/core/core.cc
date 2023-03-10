#include "core.hh"

namespace SSC {
  Headers::Header::Header (const Header& header) {
    this->key = header.key;
    this->value = header.value;
  }

  Headers::Header::Header (const String& key, const Value& value) {
    this->key = key;
    this->value = value;
  }

  Headers::Headers (const Headers& headers) {
    this->entries = headers.entries;
  }

  Headers::Headers (const Vector<std::map<String, Value>>& entries) {
    for (const auto& entry : entries) {
      for (const auto& pair : entry) {
        this->entries.push_back(Header { pair.first, pair.second });
      }
    }
  }

  Headers::Headers (const Entries& entries) {
    for (const auto& entry : entries) {
      this->entries.push_back(entry);
    }
  }

  size_t Headers::size () const {
    return this->entries.size();
  }

  String Headers::str () const {
    StringStream headers;
    auto count = this->size();
    for (const auto& entry : this->entries) {
      headers << entry.key << ": " << entry.value.str();;
      if (--count > 0) {
        headers << "\n";
      }
    }
    return headers.str();
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

    if (post.body) {
      delete [] post.body;
    }

    posts->erase(id);
  }

  String Core::createPost (String seq, String params, Post post) {
    Lock lock(postsMutex);

    if (post.id == 0) {
      post.id = rand64();
    }

    auto sid = std::to_string(post.id);
    auto js = createJavaScript("post-data.js",
      "const globals = await import('socket:internal/globals');              \n"
      "const id = `" + sid + "`;                                             \n"
      "const seq = `" + seq + "`;                                            \n"
      "const headers = `" + trim(post.headers) + "`                          \n"
      "  .trim()                                                             \n"
      "  .split(/[\\r\\n]+/)                                                 \n"
      "  .filter(Boolean);                                                   \n"
      "                                                                      \n"
      "let params = `" + params + "`;                                        \n"
      "                                                                      \n"
      "try {                                                                 \n"
      "  params = JSON.parse(params);                                        \n"
      "} catch (err) {                                                       \n"
      "  console.error(err.stack || err, params);                            \n"
      "}                                                                     \n"
      "                                                                      \n"
      "globals.get('RuntimeXHRPostQueue').dispatch(                          \n"
      "  id,                                                                 \n"
      "  seq,                                                                \n"
      "  params,                                                             \n"
      "  headers                                                             \n"
      ");                                                                    \n"
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

  void Core::OS::cpus (
    const String seq,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
    #if defined(__ANDROID__)
      {
        auto json = JSON::Object::Entries {
          {"source", "os.cpus"},
          {"data", JSON::Array::Entries {}}
        };

        cb(seq, json, Post{});
        return;
      }
    #endif

      uv_cpu_info_t* infos = nullptr;
      int count = 0;
      int status = uv_cpu_info(&infos, &count);

      if (status != 0) {
        auto json = JSON::Object::Entries {
          {"source", "os.cpus"},
          {"err", JSON::Object::Entries {
            {"message", uv_strerror(status)}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      JSON::Array::Entries entries(count);
      for (int i = 0; i < count; ++i) {
        auto info = infos[i];
        entries[i] = JSON::Object::Entries {
          {"model", info.model},
          {"speed", info.speed},
          {"times", JSON::Object::Entries {
            {"user", info.cpu_times.user},
            {"nice", info.cpu_times.nice},
            {"sys", info.cpu_times.sys},
            {"idle", info.cpu_times.idle},
            {"irq", info.cpu_times.irq}
          }}
        };
      }

      auto json = JSON::Object::Entries {
        {"source", "os.cpus"},
        {"data", entries}
      };

      uv_free_cpu_info(infos, count);
      cb(seq, json, Post{});
    });
  }

  void Core::OS::networkInterfaces (
    const String seq,
    Module::Callback cb
  ) const {
    uv_interface_address_t *infos = nullptr;
    StringStream value;
    StringStream v4;
    StringStream v6;
    int count = 0;

    int status = uv_interface_addresses(&infos, &count);

    if (status != 0) {
      auto json = JSON::Object(JSON::Object::Entries {
        {"source", "os.networkInterfaces"},
        {"err", JSON::Object::Entries {
          {"type", "InternalError"},
          {"message",
            String("Unable to get network interfaces: ") + String(uv_strerror(status))
          }
        }}
      });

      return cb(seq, json, Post{});
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

    auto json = JSON::Object::Entries {
      {"source", "os.networkInterfaces"},
      {"data", data}
    };

    cb(seq, json, Post{});
  }

  void Core::OS::rusage (
    const String seq,
    Module::Callback cb
  ) {
    uv_rusage_t usage;
    auto status = uv_getrusage(&usage);

    if (status != 0) {
      auto json = JSON::Object::Entries {
        {"source", "os.rusage"},
        {"err", JSON::Object::Entries {
          {"message", uv_strerror(status)}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "os.rusage"},
      {"data", JSON::Object::Entries {
        {"ru_maxrss", usage.ru_maxrss}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::OS::uname (
    const String seq,
    Module::Callback cb
  ) {
    uv_utsname_t uname;
    auto status = uv_os_uname(&uname);

    if (status != 0) {
      auto json = JSON::Object::Entries {
        {"source", "os.uname"},
        {"err", JSON::Object::Entries {
          {"message", uv_strerror(status)}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "os.uname"},
      {"data", JSON::Object::Entries {
        {"sysname", uname.sysname},
        {"release", uname.release},
        {"version", uname.version},
        {"machine", uname.machine}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::OS::uptime (
    const String seq,
    Module::Callback cb
  ) {
    double uptime;
    auto status = uv_uptime(&uptime);

    if (status != 0) {
      auto json = JSON::Object::Entries {
        {"source", "os.uptime"},
        {"err", JSON::Object::Entries {
          {"message", uv_strerror(status)}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "os.uptime"},
      {"data", uptime * 1000} // in milliseconds
    };

    cb(seq, json, Post{});
  }

  void Core::OS::hrtime (
    const String seq,
    Module::Callback cb
  ) {
    auto hrtime = uv_hrtime();
    auto bytes = toBytes(hrtime);
    auto size = bytes.size();
    auto post = Post {};
    auto body = new char[size]{0};
    auto json = JSON::Object {};
    post.body = body;
    post.length = size;
    memcpy(body, bytes.data(), size);
    cb(seq, json, post);
  }

  void Core::OS::availableMemory (
    const String seq,
    Module::Callback cb
  ) {
    auto memory = uv_get_available_memory();
    auto bytes = toBytes(memory);
    auto size = bytes.size();
    auto post = Post {};
    auto body = new char[size]{0};
    auto json = JSON::Object {};
    post.body = body;
    post.length = size;
    memcpy(body, bytes.data(), size);
    cb(seq, json, post);
  }

  void Core::OS::bufferSize (
    const String seq,
    uint64_t peerId,
    size_t size,
    int buffer,
    Module::Callback cb
  ) {
    if (buffer < 0) {
      buffer = Core::OS::SEND_BUFFER;
    } else if (buffer > 1) {
      buffer = Core::OS::RECV_BUFFER;
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
          {"size", (int) size}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void Core::Platform::event (
    const String seq,
    const String event,
    const String data,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      // init page
      if (event == "domcontentloaded") {
        Lock lock(this->core->fs.mutex);

        for (auto const &tuple : this->core->fs.descriptors) {
          auto desc = tuple.second;
          if (desc != nullptr) {
            desc->stale = true;
          } else {
            this->core->fs.descriptors.erase(tuple.first);
          }
        }
      }

      auto json = JSON::Object::Entries {
        {"source", "platform.event"},
        {"data", JSON::Object::Entries{}}
      };

      cb(seq, json, Post{});
    });
  }

  void Core::Platform::notify (
    const String seq,
    const String title,
    const String body,
    Module::Callback cb
  ) {
#if defined(__APPLE__)
    auto center = [UNUserNotificationCenter currentNotificationCenter];
    auto content = [[UNMutableNotificationContent alloc] init];
    content.body = [NSString stringWithUTF8String: body.c_str()];
    content.title = [NSString stringWithUTF8String: title.c_str()];
    content.sound = [UNNotificationSound defaultSound];

    auto trigger = [UNTimeIntervalNotificationTrigger
      triggerWithTimeInterval: 1.0f
                      repeats: NO
    ];

    auto request = [UNNotificationRequest
      requestWithIdentifier: @"LocalNotification"
                    content: content
                    trigger: trigger
    ];

    auto options = (
      UNAuthorizationOptionAlert |
      UNAuthorizationOptionBadge |
      UNAuthorizationOptionSound
    );

    [center requestAuthorizationWithOptions: options
                          completionHandler: ^(BOOL granted, NSError* error)
    {
      #if !__has_feature(objc_arc)
      [content release];
      [trigger release];
      #endif

      if (granted) {
        auto json = JSON::Object::Entries {
          {"source", "platform.notify"},
          {"data", JSON::Object::Entries {}}
        };

        cb(seq, json, Post{});
      } else if (error) {
        [center addNotificationRequest: request
                 withCompletionHandler: ^(NSError* error)
        {
          auto json = JSON::Object {};

          #if !__has_feature(objc_arc)
          [request release];
          #endif

          if (error) {
            json = JSON::Object::Entries {
              {"source", "platform.notify"},
              {"err", JSON::Object::Entries {
                {"message", [error.debugDescription UTF8String]}
              }}
            };

            debug("Unable to create notification: %@", error.debugDescription);
          } else {
            json = JSON::Object::Entries {
              {"source", "platform.notify"},
              {"data", JSON::Object::Entries {}}
            };
          }

         cb(seq, json, Post{});
        }];
      } else {
        auto json = JSON::Object::Entries {
          {"source", "platform.notify"},
          {"err", JSON::Object::Entries {
            {"message", "Failed to create notification"}
          }}
        };

        cb(seq, json, Post{});
      }

      if (!error || granted) {
        #if !__has_feature(objc_arc)
        [request release];
        #endif
      }
    }];
#endif
  }

  void Core::Platform::openExternal (
    const String seq,
    const String value,
    Module::Callback cb
  ) {
#if defined(__APPLE__)
    auto string = [NSString stringWithUTF8String: value.c_str()];
    auto url = [NSURL URLWithString: string];

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    auto app = [UIApplication sharedApplication];
    [app openURL: url options: @{} completionHandler: ^(BOOL success) {
      auto json = JSON::Object {};

      if (!success) {
        json = JSON::Object::Entries {
          {"source", "platform.openExternal"},
          {"err", JSON::Object::Entries {
            {"message", "Failed to open external URL"}
          }}
        };
      } else {
        json = JSON::Object::Entries {
          {"source", "platform.openExternal"},
          {"data", JSON::Object::Entries {}}
        };
      }

      cb(seq, json, Post{});
    }];
    #else
    auto workspace = [NSWorkspace sharedWorkspace];
    auto configuration = [NSWorkspaceOpenConfiguration configuration];
    [workspace openURL: url
         configuration: configuration
     completionHandler: ^(NSRunningApplication *app, NSError *error)
    {
       auto json = JSON::Object {};
       if (error) {
         json = JSON::Object::Entries {
           {"source", "platform.openExternal"},
           {"err", JSON::Object::Entries {
             {"message", [error.debugDescription UTF8String]}
           }}
         };
       } else {
        json = JSON::Object::Entries {
          {"source", "platform.openExternal"},
          {"data", JSON::Object::Entries {}}
        };
       }

      cb(seq, json, Post{});
    }];
    #endif
#elif defined(__linux__) && !defined(__ANDROID__)
    auto list = gtk_window_list_toplevels();
    auto json = JSON::Object {};

    // initial state is a failure
    json = JSON::Object::Entries {
      {"source", "platform.openExternal"},
      {"err", JSON::Object::Entries {
        {"message", "Failed to open external URL"}
      }}
    };

    if (list != nullptr) {
      for (auto entry = list; entry != nullptr; entry = entry->next) {
        auto window = GTK_WINDOW(entry->data);

        if (window != nullptr && gtk_window_is_active(window)) {
          auto err = (GError*) nullptr;
          auto uri = value.c_str();
          auto ts = GDK_CURRENT_TIME;

          /**
           * GTK may print a error in the terminal that looks like:
           *
           *   libva error: vaGetDriverNameByIndex() failed with unknown libva error, driver_name = (null)
           *
           * It doesn't prevent the URI from being opened.
           * See https://github.com/intel/media-driver/issues/1349 for more info
           */
          auto success = gtk_show_uri_on_window(window, uri, ts, &err);

          if (success) {
            json = JSON::Object::Entries {
              {"source", "platform.openExternal"},
              {"data", JSON::Object::Entries {}}
            };
          } else if (err != nullptr) {
            json = JSON::Object::Entries {
              {"source", "platform.openExternal"},
              {"err", JSON::Object::Entries {
                {"message", err->message}
              }}
            };
          }

          break;
        }
      }

      g_list_free(list);
    }

    cb(seq, json, Post{});
#elif defined(_WIN32)
    auto uri = value.c_str();
    ShellExecute(nullptr, "Open", uri, nullptr, nullptr, SW_SHOWNORMAL);
    // TODO how to detect success here. do we care?
    cb(seq, JSON::Object{}, Post{});
#endif
  }

  void Core::DNS::lookup (
    const String seq,
    LookupOptions options,
    Core::Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new Core::Module::RequestContext(seq, cb);
      auto loop = this->core->getEventLoop();

      struct addrinfo hints = {0};

      if (options.family == 6) {
        hints.ai_family = AF_INET6;
      } else if (options.family == 4) {
        hints.ai_family = AF_INET;
      } else {
        hints.ai_family = AF_UNSPEC;
      }

      hints.ai_socktype = 0; // `0` for any
      hints.ai_protocol = 0; // `0` for any

      auto resolver = new uv_getaddrinfo_t;
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
          uv_freeaddrinfo(res);
          delete resolver;
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
        delete resolver;
        delete ctx;
      }, options.hostname.c_str(), nullptr, &hints);

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

#if defined(__linux__) && !defined(__ANDROID__)
  struct UVSource {
    GSource base; // should ALWAYS be first member
    gpointer tag;
    Core *core;
  };

    // @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
  static GSourceFuncs loopSourceFunctions = {
    .prepare = [](GSource *source, gint *timeout) -> gboolean {
      auto core = reinterpret_cast<UVSource *>(source)->core;
      if (!core->isLoopAlive() || !core->isLoopRunning) {
        return false;
      }

      *timeout = core->getEventLoopTimeout();
      return 0 == *timeout;
    },

    .dispatch = [](
      GSource *source,
      GSourceFunc callback,
      gpointer user_data
    ) -> gboolean {
      auto core = reinterpret_cast<UVSource *>(source)->core;
      Lock lock(core->loopMutex);
      auto loop = core->getEventLoop();
      uv_run(loop, UV_RUN_NOWAIT);
      return G_SOURCE_CONTINUE;
    }
  };
#endif

  void Core::initEventLoop () {
    if (didLoopInit) {
      return;
    }

    didLoopInit = true;
    Lock lock(loopMutex);
    uv_loop_init(&eventLoop);
    eventLoopAsync.data = (void *) this;
    uv_async_init(&eventLoop, &eventLoopAsync, [](uv_async_t *handle) {
      auto core = reinterpret_cast<SSC::Core  *>(handle->data);
      while (true) {
        Lock lock(core->loopMutex);
        if (core->eventLoopDispatchQueue.size() == 0) break;
        auto dispatch = core->eventLoopDispatchQueue.front();
        if (dispatch != nullptr) dispatch();
        core->eventLoopDispatchQueue.pop();
      }
    });

#if defined(__linux__) && !defined(__ANDROID__)
    GSource *source = g_source_new(&loopSourceFunctions, sizeof(UVSource));
    UVSource *uvSource = (UVSource *) source;
    uvSource->core = this;
    uvSource->tag = g_source_add_unix_fd(
      source,
      uv_backend_fd(&eventLoop),
      (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
    );

    g_source_attach(source, nullptr);
#endif
  }

  uv_loop_t* Core::getEventLoop () {
    initEventLoop();
    return &eventLoop;
  }

  int Core::getEventLoopTimeout () {
    auto loop = getEventLoop();
    uv_update_time(loop);
    return uv_backend_timeout(loop);
  }

  bool Core::isLoopAlive () {
    return uv_loop_alive(getEventLoop());
  }

  void Core::stopEventLoop() {
    isLoopRunning = false;
    uv_stop(&eventLoop);
  #if defined(__ANDROID__) || defined(_WIN32)
    if (eventLoopThread != nullptr) {
      if (eventLoopThread->joinable()) {
        eventLoopThread->join();
      }

      delete eventLoopThread;
      eventLoopThread = nullptr;
    }
  #endif
  }

  void Core::sleepEventLoop (int64_t ms) {
    if (ms > 0) {
      auto timeout = getEventLoopTimeout();
      ms = timeout > ms ? timeout : ms;
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
  }

  void Core::sleepEventLoop () {
    sleepEventLoop(getEventLoopTimeout());
  }

  void Core::signalDispatchEventLoop () {
    initEventLoop();
    runEventLoop();
    uv_async_send(&eventLoopAsync);
  }

  void Core::dispatchEventLoop (EventLoopDispatchCallback callback) {
    {
      Lock lock(loopMutex);
      eventLoopDispatchQueue.push(callback);
    }

    signalDispatchEventLoop();
  }

  void pollEventLoop (Core *core) {
    auto loop = core->getEventLoop();

    while (core->isLoopRunning) {
      core->sleepEventLoop(EVENT_LOOP_POLL_TIMEOUT);

      do {
        uv_run(loop, UV_RUN_DEFAULT);
      } while (core->isLoopRunning && core->isLoopAlive());
    }

    core->isLoopRunning = false;
  }

  void Core::runEventLoop () {
    if (isLoopRunning) {
      return;
    }

    isLoopRunning = true;

    initEventLoop();
    dispatchEventLoop([=, this]() {
      initTimers();
      startTimers();
    });

#if defined(__APPLE__)
    Lock lock(loopMutex);
    dispatch_async(eventLoopQueue, ^{ pollEventLoop(this); });
#elif defined(__ANDROID__) || !defined(__linux__)
    Lock lock(loopMutex);
    // clean up old thread if still running
    if (eventLoopThread != nullptr) {
      if (eventLoopThread->joinable()) {
        eventLoopThread->join();
      }

      delete eventLoopThread;
      eventLoopThread = nullptr;
    }

    eventLoopThread = new std::thread(&pollEventLoop, this);
#endif
  }

  static Timer releaseWeakDescriptors = {
    .timeout = 256, // in milliseconds
    .invoke = [](uv_timer_t *handle) {
      auto core = reinterpret_cast<Core *>(handle->data);
      Vector<uint64_t> ids;
      String msg = "";

      Lock lock(core->fs.mutex);
      for (auto const &tuple : core->fs.descriptors) {
        ids.push_back(tuple.first);
      }

      for (auto const id : ids) {
        Lock lock(core->fs.mutex);
        auto desc = core->fs.descriptors.at(id);

        if (desc == nullptr) {
          core->fs.descriptors.erase(id);
          continue;
        }

        if (desc->isRetained() || !desc->isStale()) {
          continue;
        }

        if (desc->isDirectory()) {
          core->fs.closedir("", id, [](auto seq, auto msg, auto post) {});
        } else if (desc->isFile()) {
          core->fs.close("", id, [](auto seq, auto msg, auto post) {});
        } else {
          // free
          core->fs.descriptors.erase(id);
          delete desc;
        }
      }
    }
  };

  void Core::initTimers () {
    if (didTimersInit) {
      return;
    }

    Lock lock(timersMutex);

    auto loop = getEventLoop();

    std::vector<Timer *> timersToInit = {
      &releaseWeakDescriptors
    };

    for (const auto& timer : timersToInit) {
      uv_timer_init(loop, &timer->handle);
      timer->handle.data = (void *) this;
    }

    didTimersInit = true;
  }

  void Core::startTimers () {
    Lock lock(timersMutex);

    std::vector<Timer *> timersToStart = {
      &releaseWeakDescriptors
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

  void Core::stopTimers () {
    if (didTimersStart == false) {
      return;
    }

    Lock lock(timersMutex);

    std::vector<Timer *> timersToStop = {
      &releaseWeakDescriptors
    };

    for (const auto& timer : timersToStop) {
      if (timer->started) {
        uv_timer_stop(&timer->handle);
      }
    }

    didTimersStart = false;
  }
}
