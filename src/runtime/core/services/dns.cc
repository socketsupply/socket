#include "dns.hh"

namespace ssc::runtime::core::services {
  void DNS::lookup (
    const String& seq,
    const LookupOptions& options,
    const Callback callback
  ) const {
    const auto family = options.family;
    const auto hostname = options.hostname;
    this->loop.dispatch([this, seq, callback, family, hostname]() {
      const auto ctx = new RequestContext {seq, callback};
      auto loop = this->loop.get();

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

      const auto resolver = new uv_getaddrinfo_t;
      resolver->data = ctx;

      const auto err = uv_getaddrinfo(loop, resolver, [](uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
        const auto ctx = (RequestContext*) resolver->data;

        if (status < 0) {
          const auto result = JSON::Object::Entries {
            {"source", "dns.lookup"},
            {"err", JSON::Object::Entries {
              {"code", std::to_string(status)},
              {"message", String(uv_strerror(status))}
            }}
          };

          ctx->callback(ctx->seq, result, QueuedResponse{});
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

        const auto family = res->ai_family == AF_INET
          ? 4
          : res->ai_family == AF_INET6
            ? 6
            : 0;

        const auto result = JSON::Object::Entries {
          {"source", "dns.lookup"},
          {"data", JSON::Object::Entries {
            {"address", address},
            {"family", family}
          }}
        };

        ctx->callback(ctx->seq, result, QueuedResponse{});
        uv_freeaddrinfo(res);
        delete resolver;
        delete ctx;
      }, hostname.c_str(), nullptr, &hints);

      if (err < 0) {
        const auto result = JSON::Object::Entries {
          {"source", "dns.lookup"},
          {"err", JSON::Object::Entries {
            {"code", std::to_string(err)},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(seq, result, QueuedResponse{});
        delete ctx;
      }
    });
  }
}
