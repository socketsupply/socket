#include "core.hh"

namespace SSC {
  #define SET_CONSTANT(c) constants[#c] = (c);
  static std::map<String, int32_t> getOSConstantsMap () {
    std::map<String, int32_t> constants;

    #if defined(E2BIG)
    SET_CONSTANT(E2BIG)
    #endif
    #if defined(EACCES)
    SET_CONSTANT(EACCES)
    #endif
    #if defined(EADDRINUSE)
    SET_CONSTANT(EADDRINUSE)
    #endif
    #if defined(EADDRNOTAVAIL)
    SET_CONSTANT(EADDRNOTAVAIL)
    #endif
    #if defined(EAFNOSUPPORT)
    SET_CONSTANT(EAFNOSUPPORT)
    #endif
    #if defined(EAGAIN)
    SET_CONSTANT(EAGAIN)
    #endif
    #if defined(EALREADY)
    SET_CONSTANT(EALREADY)
    #endif
    #if defined(EBADF)
    SET_CONSTANT(EBADF)
    #endif
    #if defined(EBADMSG)
    SET_CONSTANT(EBADMSG)
    #endif
    #if defined(EBUSY)
    SET_CONSTANT(EBUSY)
    #endif
    #if defined(ECANCELED)
    SET_CONSTANT(ECANCELED)
    #endif
    #if defined(ECHILD)
    SET_CONSTANT(ECHILD)
    #endif
    #if defined(ECONNABORTED)
    SET_CONSTANT(ECONNABORTED)
    #endif
    #if defined(ECONNREFUSED)
    SET_CONSTANT(ECONNREFUSED)
    #endif
    #if defined(ECONNRESET)
    SET_CONSTANT(ECONNRESET)
    #endif
    #if defined(EDEADLK)
    SET_CONSTANT(EDEADLK)
    #endif
    #if defined(EDESTADDRREQ)
    SET_CONSTANT(EDESTADDRREQ)
    #endif
    #if defined(EDOM)
    SET_CONSTANT(EDOM)
    #endif
    #if defined(EDQUOT)
    SET_CONSTANT(EDQUOT)
    #endif
    #if defined(EEXIST)
    SET_CONSTANT(EEXIST)
    #endif
    #if defined(EFAULT)
    SET_CONSTANT(EFAULT)
    #endif
    #if defined(EFBIG)
    SET_CONSTANT(EFBIG)
    #endif
    #if defined(EHOSTUNREACH)
    SET_CONSTANT(EHOSTUNREACH)
    #endif
    #if defined(EIDRM)
    SET_CONSTANT(EIDRM)
    #endif
    #if defined(EILSEQ)
    SET_CONSTANT(EILSEQ)
    #endif
    #if defined(EINPROGRESS)
    SET_CONSTANT(EINPROGRESS)
    #endif
    #if defined(EINTR)
    SET_CONSTANT(EINTR)
    #endif
    #if defined(EINVAL)
    SET_CONSTANT(EINVAL)
    #endif
    #if defined(EIO)
    SET_CONSTANT(EIO)
    #endif
    #if defined(EISCONN)
    SET_CONSTANT(EISCONN)
    #endif
    #if defined(EISDIR)
    SET_CONSTANT(EISDIR)
    #endif
    #if defined(ELOOP)
    SET_CONSTANT(ELOOP)
    #endif
    #if defined(EMFILE)
    SET_CONSTANT(EMFILE)
    #endif
    #if defined(EMLINK)
    SET_CONSTANT(EMLINK)
    #endif
    #if defined(EMSGSIZE)
    SET_CONSTANT(EMSGSIZE)
    #endif
    #if defined(EMULTIHOP)
    SET_CONSTANT(EMULTIHOP)
    #endif
    #if defined(ENAMETOOLONG)
    SET_CONSTANT(ENAMETOOLONG)
    #endif
    #if defined(ENETDOWN)
    SET_CONSTANT(ENETDOWN)
    #endif
    #if defined(ENETRESET)
    SET_CONSTANT(ENETRESET)
    #endif
    #if defined(ENETUNREACH)
    SET_CONSTANT(ENETUNREACH)
    #endif
    #if defined(ENFILE)
    SET_CONSTANT(ENFILE)
    #endif
    #if defined(ENOBUFS)
    SET_CONSTANT(ENOBUFS)
    #endif
    #if defined(ENODATA)
    SET_CONSTANT(ENODATA)
    #endif
    #if defined(ENODEV)
    SET_CONSTANT(ENODEV)
    #endif
    #if defined(ENOENT)
    SET_CONSTANT(ENOENT)
    #endif
    #if defined(ENOEXEC)
    SET_CONSTANT(ENOEXEC)
    #endif
    #if defined(ENOLCK)
    SET_CONSTANT(ENOLCK)
    #endif
    #if defined(ENOLINK)
    SET_CONSTANT(ENOLINK)
    #endif
    #if defined(ENOMEM)
    SET_CONSTANT(ENOMEM)
    #endif
    #if defined(ENOMSG)
    SET_CONSTANT(ENOMSG)
    #endif
    #if defined(ENOPROTOOPT)
    SET_CONSTANT(ENOPROTOOPT)
    #endif
    #if defined(ENOSPC)
    SET_CONSTANT(ENOSPC)
    #endif
    #if defined(ENOSR)
    SET_CONSTANT(ENOSR)
    #endif
    #if defined(ENOSTR)
    SET_CONSTANT(ENOSTR)
    #endif
    #if defined(ENOSYS)
    SET_CONSTANT(ENOSYS)
    #endif
    #if defined(ENOTCONN)
    SET_CONSTANT(ENOTCONN)
    #endif
    #if defined(ENOTDIR)
    SET_CONSTANT(ENOTDIR)
    #endif
    #if defined(ENOTEMPTY)
    SET_CONSTANT(ENOTEMPTY)
    #endif
    #if defined(ENOTSOCK)
    SET_CONSTANT(ENOTSOCK)
    #endif
    #if defined(ENOTSUP)
    SET_CONSTANT(ENOTSUP)
    #endif
    #if defined(ENOTTY)
    SET_CONSTANT(ENOTTY)
    #endif
    #if defined(ENXIO)
    SET_CONSTANT(ENXIO)
    #endif
    #if defined(EOPNOTSUPP)
    SET_CONSTANT(EOPNOTSUPP)
    #endif
    #if defined(EOVERFLOW)
    SET_CONSTANT(EOVERFLOW)
    #endif
    #if defined(EPERM)
    SET_CONSTANT(EPERM)
    #endif
    #if defined(EPIPE)
    SET_CONSTANT(EPIPE)
    #endif
    #if defined(EPROTO)
    SET_CONSTANT(EPROTO)
    #endif
    #if defined(EPROTONOSUPPORT)
    SET_CONSTANT(EPROTONOSUPPORT)
    #endif
    #if defined(EPROTOTYPE)
    SET_CONSTANT(EPROTOTYPE)
    #endif
    #if defined(ERANGE)
    SET_CONSTANT(ERANGE)
    #endif
    #if defined(EROFS)
    SET_CONSTANT(EROFS)
    #endif
    #if defined(ESPIPE)
    SET_CONSTANT(ESPIPE)
    #endif
    #if defined(ESRCH)
    SET_CONSTANT(ESRCH)
    #endif
    #if defined(ESTALE)
    SET_CONSTANT(ESTALE)
    #endif
    #if defined(ETIME)
    SET_CONSTANT(ETIME)
    #endif
    #if defined(ETIMEDOUT)
    SET_CONSTANT(ETIMEDOUT)
    #endif
    #if defined(ETXTBSY)
    SET_CONSTANT(ETXTBSY)
    #endif
    #if defined(EWOULDBLOCK)
    SET_CONSTANT(EWOULDBLOCK)
    #endif
    #if defined(EXDEV)
    SET_CONSTANT(EXDEV)
    #endif

    #if defined(SIGHUP)
    SET_CONSTANT(SIGHUP)
    #endif
    #if defined(SIGINT)
    SET_CONSTANT(SIGINT)
    #endif
    #if defined(SIGQUIT)
    SET_CONSTANT(SIGQUIT)
    #endif
    #if defined(SIGILL)
    SET_CONSTANT(SIGILL)
    #endif
    #if defined(SIGTRAP)
    SET_CONSTANT(SIGTRAP)
    #endif
    #if defined(SIGABRT)
    SET_CONSTANT(SIGABRT)
    #endif
    #if defined(SIGIOT)
    SET_CONSTANT(SIGIOT)
    #endif
    #if defined(SIGBUS)
    SET_CONSTANT(SIGBUS)
    #endif
    #if defined(SIGFPE)
    SET_CONSTANT(SIGFPE)
    #endif
    #if defined(SIGKILL)
    SET_CONSTANT(SIGKILL)
    #endif
    #if defined(SIGUSR1)
    SET_CONSTANT(SIGUSR1)
    #endif
    #if defined(SIGSEGV)
    SET_CONSTANT(SIGSEGV)
    #endif
    #if defined(SIGUSR2)
    SET_CONSTANT(SIGUSR2)
    #endif
    #if defined(SIGPIPE)
    SET_CONSTANT(SIGPIPE)
    #endif
    #if defined(SIGALRM)
    SET_CONSTANT(SIGALRM)
    #endif
    #if defined(SIGTERM)
    SET_CONSTANT(SIGTERM)
    #endif
    #if defined(SIGCHLD)
    SET_CONSTANT(SIGCHLD)
    #endif
    #if defined(SIGCONT)
    SET_CONSTANT(SIGCONT)
    #endif
    #if defined(SIGSTOP)
    SET_CONSTANT(SIGSTOP)
    #endif
    #if defined(SIGTSTP)
    SET_CONSTANT(SIGTSTP)
    #endif
    #if defined(SIGTTIN)
    SET_CONSTANT(SIGTTIN)
    #endif
    #if defined(SIGTTOU)
    SET_CONSTANT(SIGTTOU)
    #endif
    #if defined(SIGURG)
    SET_CONSTANT(SIGURG)
    #endif
    #if defined(SIGXCPU)
    SET_CONSTANT(SIGXCPU)
    #endif
    #if defined(SIGXFSZ)
    SET_CONSTANT(SIGXFSZ)
    #endif
    #if defined(SIGVTALRM)
    SET_CONSTANT(SIGVTALRM)
    #endif
    #if defined(SIGPROF)
    SET_CONSTANT(SIGPROF)
    #endif
    #if defined(SIGWINCH)
    SET_CONSTANT(SIGWINCH)
    #endif
    #if defined(SIGIO)
    SET_CONSTANT(SIGIO)
    #endif
    #if defined(SIGINFO)
    SET_CONSTANT(SIGINFO)
    #endif
    #if defined(SIGSYS)
    SET_CONSTANT(SIGSYS)
    #endif


    return constants;
  }
  #undef SET_CONSTANT

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
    if (buffer == 0) {
      buffer = Core::OS::SEND_BUFFER;
    } else if (buffer == 1) {
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

  void Core::OS::constants (const String seq, Module::Callback cb) {
    static auto constants = getOSConstantsMap();
    static auto data = JSON::Object {constants};
    static auto json = JSON::Object::Entries {
      {"source", "os.constants"},
      {"data", data}
    };

    static auto headers = Headers {{
      Headers::Header {"Cache-Control", "public, max-age=86400"}
    }};

    static auto post = Post {
      .id = 0,
      .ttl = 0,
      .body = nullptr,
      .length = 0,
      .headers = headers.str()
    };

    cb(seq, json, post);
  }
}
