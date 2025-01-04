#include "../../string.hh"
#include "../../bytes.hh"
#include "../../json.hh"
#include "../../os.hh"

#include "../services.hh"

#include "os.hh"
#include "udp.hh"

using ssc::runtime::bytes::toByteArray;

namespace ssc::runtime::core::services {
  void OS::cpus (
    const String& seq,
    const Callback callback
  ) const {
    this->loop.dispatch([=, this]() {
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      {
        auto json = JSON::Object::Entries {
          {"source", "os.cpus"},
          {"data", JSON::Array::Entries {}}
        };

        callback(seq, json, QueuedResponse{});
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

        callback(seq, json, QueuedResponse{});
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
      callback(seq, json, QueuedResponse{});
    });
  }

  void OS::networkInterfaces (
    const String& seq,
    const Callback callback
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

      return callback(seq, json, QueuedResponse{});
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
        entries["address"] = udp::ip::addrToIPv4(addr);
        entries["mac"] = String(mac, 17);
        ipv4[String(info.name)] = entries;
      }

      if (addr->sin_family == AF_INET6) {
        JSON::Object::Entries entries;
        entries["internal"] = info.is_internal == 0 ? "false" : "true";
        entries["address"] = udp::ip::addrToIPv6((struct sockaddr_in6*) addr);
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

    callback(seq, json, QueuedResponse{});
  }

  void OS::rusage (
    const String& seq,
    const Callback callback
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

      callback(seq, json, QueuedResponse{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "os.rusage"},
      {"data", JSON::Object::Entries {
        {"ru_maxrss", usage.ru_maxrss}
      }}
    };

    callback(seq, json, QueuedResponse{});
  }

  void OS::uname (
    const String& seq,
    const Callback callback
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

      callback(seq, json, QueuedResponse{});
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

    callback(seq, json, QueuedResponse{});
  }

  void OS::uptime (
    const String& seq,
    const Callback callback
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

      callback(seq, json, QueuedResponse{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "os.uptime"},
      {"data", uptime * 1000} // in milliseconds
    };

    callback(seq, json, QueuedResponse{});
  }

  void OS::hrtime (
    const String& seq,
    const Callback callback
  ) const {
    const auto hrtime = uv_hrtime();
    const auto bytes = toByteArray(hrtime);
    const auto size = bytes.size();
    const auto body = new unsigned char[size]{0};
    const auto json = JSON::Object {};
    auto post = QueuedResponse {};
    post.body.reset(body);
    post.length = size;
    memcpy(body, bytes.data(), size);
    callback(seq, json, post);
  }

  void OS::availableMemory (
    const String& seq,
    const Callback callback
  ) const {
    const auto memory = uv_get_available_memory();
    const auto bytes = toByteArray(memory);
    const auto size = bytes.size();
    const auto body = new unsigned char[size]{0};
    const auto json = JSON::Object {};
    auto post = QueuedResponse {};
    post.body.reset(body);
    post.length = size;
    memcpy(body, bytes.data(), size);
    callback(seq, json, post);
  }

  void OS::bufferSize (
    const String& seq,
    UDP::ID id,
    size_t size,
    int buffer,
    const Callback callback
  ) const {
    if (buffer == 0) {
      buffer = OS::SEND_BUFFER;
    } else if (buffer == 1) {
      buffer = OS::RECV_BUFFER;
    }

    this->loop.dispatch([=, this]() {
      auto socket = this->services.udp.getSocket(id);

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

        callback(seq, json, QueuedResponse{});
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

        callback(seq, json, QueuedResponse{});
        return;
      }

      auto json = JSON::Object::Entries {
        {"source", "bufferSize"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"size", (int) size}
        }}
      };

      callback(seq, json, QueuedResponse{});
    });
  }

  void OS::constants (
    const String& seq,
    const Callback callback
  ) const {
    static const auto data = JSON::Object(os::constants());
    static const auto json = JSON::Object::Entries {
      {"source", "os.constants"},
      {"data", data}
    };

    callback(seq, json, QueuedResponse {});
  }
}
