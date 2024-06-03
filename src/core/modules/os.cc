#include "../core.hh"
#include "../json.hh"
#include "udp.hh"
#include "os.hh"

namespace SSC {
  #define CONSTANT(c) { #c, (c) },
  static const std::map<String, int32_t> OS_CONSTANTS = {
    #if defined(E2BIG)
    CONSTANT(E2BIG)
    #endif
    #if defined(EACCES)
    CONSTANT(EACCES)
    #endif
    #if defined(EADDRINUSE)
    CONSTANT(EADDRINUSE)
    #endif
    #if defined(EADDRNOTAVAIL)
    CONSTANT(EADDRNOTAVAIL)
    #endif
    #if defined(EAFNOSUPPORT)
    CONSTANT(EAFNOSUPPORT)
    #endif
    #if defined(EAGAIN)
    CONSTANT(EAGAIN)
    #endif
    #if defined(EALREADY)
    CONSTANT(EALREADY)
    #endif
    #if defined(EBADF)
    CONSTANT(EBADF)
    #endif
    #if defined(EBADMSG)
    CONSTANT(EBADMSG)
    #endif
    #if defined(EBUSY)
    CONSTANT(EBUSY)
    #endif
    #if defined(ECANCELED)
    CONSTANT(ECANCELED)
    #endif
    #if defined(ECHILD)
    CONSTANT(ECHILD)
    #endif
    #if defined(ECONNABORTED)
    CONSTANT(ECONNABORTED)
    #endif
    #if defined(ECONNREFUSED)
    CONSTANT(ECONNREFUSED)
    #endif
    #if defined(ECONNRESET)
    CONSTANT(ECONNRESET)
    #endif
    #if defined(EDEADLK)
    CONSTANT(EDEADLK)
    #endif
    #if defined(EDESTADDRREQ)
    CONSTANT(EDESTADDRREQ)
    #endif
    #if defined(EDOM)
    CONSTANT(EDOM)
    #endif
    #if defined(EDQUOT)
    CONSTANT(EDQUOT)
    #endif
    #if defined(EEXIST)
    CONSTANT(EEXIST)
    #endif
    #if defined(EFAULT)
    CONSTANT(EFAULT)
    #endif
    #if defined(EFBIG)
    CONSTANT(EFBIG)
    #endif
    #if defined(EHOSTUNREACH)
    CONSTANT(EHOSTUNREACH)
    #endif
    #if defined(EIDRM)
    CONSTANT(EIDRM)
    #endif
    #if defined(EILSEQ)
    CONSTANT(EILSEQ)
    #endif
    #if defined(EINPROGRESS)
    CONSTANT(EINPROGRESS)
    #endif
    #if defined(EINTR)
    CONSTANT(EINTR)
    #endif
    #if defined(EINVAL)
    CONSTANT(EINVAL)
    #endif
    #if defined(EIO)
    CONSTANT(EIO)
    #endif
    #if defined(EISCONN)
    CONSTANT(EISCONN)
    #endif
    #if defined(EISDIR)
    CONSTANT(EISDIR)
    #endif
    #if defined(ELOOP)
    CONSTANT(ELOOP)
    #endif
    #if defined(EMFILE)
    CONSTANT(EMFILE)
    #endif
    #if defined(EMLINK)
    CONSTANT(EMLINK)
    #endif
    #if defined(EMSGSIZE)
    CONSTANT(EMSGSIZE)
    #endif
    #if defined(EMULTIHOP)
    CONSTANT(EMULTIHOP)
    #endif
    #if defined(ENAMETOOLONG)
    CONSTANT(ENAMETOOLONG)
    #endif
    #if defined(ENETDOWN)
    CONSTANT(ENETDOWN)
    #endif
    #if defined(ENETRESET)
    CONSTANT(ENETRESET)
    #endif
    #if defined(ENETUNREACH)
    CONSTANT(ENETUNREACH)
    #endif
    #if defined(ENFILE)
    CONSTANT(ENFILE)
    #endif
    #if defined(ENOBUFS)
    CONSTANT(ENOBUFS)
    #endif
    #if defined(ENODATA)
    CONSTANT(ENODATA)
    #endif
    #if defined(ENODEV)
    CONSTANT(ENODEV)
    #endif
    #if defined(ENOENT)
    CONSTANT(ENOENT)
    #endif
    #if defined(ENOEXEC)
    CONSTANT(ENOEXEC)
    #endif
    #if defined(ENOLCK)
    CONSTANT(ENOLCK)
    #endif
    #if defined(ENOLINK)
    CONSTANT(ENOLINK)
    #endif
    #if defined(ENOMEM)
    CONSTANT(ENOMEM)
    #endif
    #if defined(ENOMSG)
    CONSTANT(ENOMSG)
    #endif
    #if defined(ENOPROTOOPT)
    CONSTANT(ENOPROTOOPT)
    #endif
    #if defined(ENOSPC)
    CONSTANT(ENOSPC)
    #endif
    #if defined(ENOSR)
    CONSTANT(ENOSR)
    #endif
    #if defined(ENOSTR)
    CONSTANT(ENOSTR)
    #endif
    #if defined(ENOSYS)
    CONSTANT(ENOSYS)
    #endif
    #if defined(ENOTCONN)
    CONSTANT(ENOTCONN)
    #endif
    #if defined(ENOTDIR)
    CONSTANT(ENOTDIR)
    #endif
    #if defined(ENOTEMPTY)
    CONSTANT(ENOTEMPTY)
    #endif
    #if defined(ENOTSOCK)
    CONSTANT(ENOTSOCK)
    #endif
    #if defined(ENOTSUP)
    CONSTANT(ENOTSUP)
    #endif
    #if defined(ENOTTY)
    CONSTANT(ENOTTY)
    #endif
    #if defined(ENXIO)
    CONSTANT(ENXIO)
    #endif
    #if defined(EOPNOTSUPP)
    CONSTANT(EOPNOTSUPP)
    #endif
    #if defined(EOVERFLOW)
    CONSTANT(EOVERFLOW)
    #endif
    #if defined(EPERM)
    CONSTANT(EPERM)
    #endif
    #if defined(EPIPE)
    CONSTANT(EPIPE)
    #endif
    #if defined(EPROTO)
    CONSTANT(EPROTO)
    #endif
    #if defined(EPROTONOSUPPORT)
    CONSTANT(EPROTONOSUPPORT)
    #endif
    #if defined(EPROTOTYPE)
    CONSTANT(EPROTOTYPE)
    #endif
    #if defined(ERANGE)
    CONSTANT(ERANGE)
    #endif
    #if defined(EROFS)
    CONSTANT(EROFS)
    #endif
    #if defined(ESPIPE)
    CONSTANT(ESPIPE)
    #endif
    #if defined(ESRCH)
    CONSTANT(ESRCH)
    #endif
    #if defined(ESTALE)
    CONSTANT(ESTALE)
    #endif
    #if defined(ETIME)
    CONSTANT(ETIME)
    #endif
    #if defined(ETIMEDOUT)
    CONSTANT(ETIMEDOUT)
    #endif
    #if defined(ETXTBSY)
    CONSTANT(ETXTBSY)
    #endif
    #if defined(EWOULDBLOCK)
    CONSTANT(EWOULDBLOCK)
    #endif
    #if defined(EXDEV)
    CONSTANT(EXDEV)
    #endif

    #if defined(SIGHUP)
    CONSTANT(SIGHUP)
    #endif
    #if defined(SIGINT)
    CONSTANT(SIGINT)
    #endif
    #if defined(SIGQUIT)
    CONSTANT(SIGQUIT)
    #endif
    #if defined(SIGILL)
    CONSTANT(SIGILL)
    #endif
    #if defined(SIGTRAP)
    CONSTANT(SIGTRAP)
    #endif
    #if defined(SIGABRT)
    CONSTANT(SIGABRT)
    #endif
    #if defined(SIGIOT)
    CONSTANT(SIGIOT)
    #endif
    #if defined(SIGBUS)
    CONSTANT(SIGBUS)
    #endif
    #if defined(SIGFPE)
    CONSTANT(SIGFPE)
    #endif
    #if defined(SIGKILL)
    CONSTANT(SIGKILL)
    #endif
    #if defined(SIGUSR1)
    CONSTANT(SIGUSR1)
    #endif
    #if defined(SIGSEGV)
    CONSTANT(SIGSEGV)
    #endif
    #if defined(SIGUSR2)
    CONSTANT(SIGUSR2)
    #endif
    #if defined(SIGPIPE)
    CONSTANT(SIGPIPE)
    #endif
    #if defined(SIGALRM)
    CONSTANT(SIGALRM)
    #endif
    #if defined(SIGTERM)
    CONSTANT(SIGTERM)
    #endif
    #if defined(SIGCHLD)
    CONSTANT(SIGCHLD)
    #endif
    #if defined(SIGCONT)
    CONSTANT(SIGCONT)
    #endif
    #if defined(SIGSTOP)
    CONSTANT(SIGSTOP)
    #endif
    #if defined(SIGTSTP)
    CONSTANT(SIGTSTP)
    #endif
    #if defined(SIGTTIN)
    CONSTANT(SIGTTIN)
    #endif
    #if defined(SIGTTOU)
    CONSTANT(SIGTTOU)
    #endif
    #if defined(SIGURG)
    CONSTANT(SIGURG)
    #endif
    #if defined(SIGXCPU)
    CONSTANT(SIGXCPU)
    #endif
    #if defined(SIGXFSZ)
    CONSTANT(SIGXFSZ)
    #endif
    #if defined(SIGVTALRM)
    CONSTANT(SIGVTALRM)
    #endif
    #if defined(SIGPROF)
    CONSTANT(SIGPROF)
    #endif
    #if defined(SIGWINCH)
    CONSTANT(SIGWINCH)
    #endif
    #if defined(SIGIO)
    CONSTANT(SIGIO)
    #endif
    #if defined(SIGINFO)
    CONSTANT(SIGINFO)
    #endif
    #if defined(SIGSYS)
    CONSTANT(SIGSYS)
    #endif
  };
  #undef CONSTANT

  void CoreOS::cpus (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      {
        auto json = JSON::Object::Entries {
          {"source", "os.cpus"},
          {"data", JSON::Array::Entries {}}
        };

        callback(seq, json, Post{});
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

        callback(seq, json, Post{});
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
      callback(seq, json, Post{});
    });
  }

  void CoreOS::networkInterfaces (
    const String& seq,
    const CoreModule::Callback& callback
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

      return callback(seq, json, Post{});
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
        entries["address"] = IP::addrToIPv4(addr);
        entries["mac"] = String(mac, 17);
        ipv4[String(info.name)] = entries;
      }

      if (addr->sin_family == AF_INET6) {
        JSON::Object::Entries entries;
        entries["internal"] = info.is_internal == 0 ? "false" : "true";
        entries["address"] = IP::addrToIPv6((struct sockaddr_in6*) addr);
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

    callback(seq, json, Post{});
  }

  void CoreOS::rusage (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    uv_rusage_t usage;
    auto status = uv_getrusage(&usage);

    if (status != 0) {
      auto json = JSON::Object::Entries {
        {"source", "os.rusage"},
        {"err", JSON::Object::Entries {
          {"message", uv_strerror(status)}
        }}
      };

      callback(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "os.rusage"},
      {"data", JSON::Object::Entries {
        {"ru_maxrss", usage.ru_maxrss}
      }}
    };

    callback(seq, json, Post{});
  }

  void CoreOS::uname (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    uv_utsname_t uname;
    auto status = uv_os_uname(&uname);

    if (status != 0) {
      auto json = JSON::Object::Entries {
        {"source", "os.uname"},
        {"err", JSON::Object::Entries {
          {"message", uv_strerror(status)}
        }}
      };

      callback(seq, json, Post{});
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

    callback(seq, json, Post{});
  }

  void CoreOS::uptime (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    double uptime;
    auto status = uv_uptime(&uptime);

    if (status != 0) {
      auto json = JSON::Object::Entries {
        {"source", "os.uptime"},
        {"err", JSON::Object::Entries {
          {"message", uv_strerror(status)}
        }}
      };

      callback(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "os.uptime"},
      {"data", uptime * 1000} // in milliseconds
    };

    callback(seq, json, Post{});
  }

  void CoreOS::hrtime (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    auto hrtime = uv_hrtime();
    auto bytes = toBytes(hrtime);
    auto size = bytes.size();
    auto post = Post {};
    auto body = new char[size]{0};
    auto json = JSON::Object {};
    post.body.reset(body);
    post.length = size;
    memcpy(body, bytes.data(), size);
    callback(seq, json, post);
  }

  void CoreOS::availableMemory (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    auto memory = uv_get_available_memory();
    auto bytes = toBytes(memory);
    auto size = bytes.size();
    auto post = Post {};
    auto body = new char[size]{0};
    auto json = JSON::Object {};
    post.body.reset(body);
    post.length = size;
    memcpy(body, bytes.data(), size);
    callback(seq, json, post);
  }

  void CoreOS::bufferSize (
    const String& seq,
    CoreUDP::ID id,
    size_t size,
    int buffer,
    const CoreModule::Callback& callback
  ) const {
    if (buffer == 0) {
      buffer = CoreOS::SEND_BUFFER;
    } else if (buffer == 1) {
      buffer = CoreOS::RECV_BUFFER;
    }

    this->core->dispatchEventLoop([=, this]() {
      auto socket = this->core->udp.getSocket(id);

      if (socket == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "bufferSize"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "NOT_FOUND_ERR"},
            {"type", "NotFoundError"},
            {"message", "No socket with specified id"}
          }}
        };

        callback(seq, json, Post{});
        return;
      }

      Lock lock(socket->mutex);
      auto handle = (uv_handle_t*) &socket->handle;
      auto err = buffer == RECV_BUFFER
       ? uv_recv_buffer_size(handle, (int *) &size)
       : uv_send_buffer_size(handle, (int *) &size);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "bufferSize"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "NOT_FOUND_ERR"},
            {"type", "NotFoundError"},
            {"message", String(uv_strerror(err))}
          }}
        };

        callback(seq, json, Post{});
        return;
      }

      auto json = JSON::Object::Entries {
        {"source", "bufferSize"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"size", (int) size}
        }}
      };

      callback(seq, json, Post{});
    });
  }

  void CoreOS::constants (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      static const auto data = JSON::Object(OS_CONSTANTS);
      static const auto json = JSON::Object::Entries {
        {"source", "os.constants"},
        {"data", data}
      };

      callback(seq, json, Post {});
    });
  }
}
