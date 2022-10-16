#include "ipc.hh"

#define IPC_CONTENT_TYPE "application/octet-stream"

using namespace SSC;
using namespace SSC::IPC;

static JSON::Any validateMessageParameters (
  const Message& message,
  const Vector<String> names
) {
  for (const auto& name : names) {
    if (!message.has(name) || message.get(name).size() == 0) {
      return JSON::Object::Entries {
        {"message", "Expecting '" + name + "' in parameters"}
      };
    }
  }

  return nullptr;
}

#define resultCallback(message, reply)                                         \
  [=](auto seq, auto json, auto post) {                                        \
    reply(Result { seq, message, json, post });                                \
  }

#define getMessageParam(var, name, parse, ...)                                 \
  try {                                                                        \
    var = parse(message.get(name, ##__VA_ARGS__));                             \
  } catch (...) {                                                              \
    return reply(Result::Err { message, JSON::Object::Entries {                \
      {"message", "Invalid '" name "' given in parameters"}                    \
    }});                                                                       \
  }

static void registerSchemeHandler (Router *router) {
#if defined(__linux__)
  // prevent this function from registering the `ipc://`
  // URI scheme handler twice
  static std::atomic<bool> registered = false;
  if (registered) return;
  registered = true;

  auto ctx = webkit_web_context_get_default();
  webkit_web_context_register_uri_scheme(ctx, "ipc", [](auto request, auto ptr){
    auto uri = String(webkit_uri_scheme_request_get_uri(request));
    auto router = reinterpret_cast<Router *>(ptr);
    auto message = Message { uri };
    auto invoked = router->invoke(message, [=](auto result) {
      auto json = result.str();
      auto size = result.post.body != nullptr ? result.post.length : json.size();
      auto body = result.post.body != nullptr ? result.post.body : json.c_str();

      auto freeFn = result.post.body != nullptr ? free : nullptr;
      auto stream = g_memory_input_stream_new_from_data(body, size, freeFn);
      auto response = webkit_uri_scheme_response_new(stream, size);

      webkit_uri_scheme_response_set_content_type(response, IPC_CONTENT_TYPE);
      webkit_uri_scheme_request_finish_with_response(request, response);
      g_object_unref(stream);
    });

    if (!invoked) {
      auto err = JSON::Object::Entries {
        {"source", uri},
        {"err", JSON::Object::Entries {
          {"message", "Not found"},
          {"type", "NotFoundError"},
          {"url", uri}
        }}
      };

      auto msg = JSON::Object(err).str();
      auto size = msg.size();
      auto bytes = msg.c_str();
      auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);
      auto response = webkit_uri_scheme_response_new(stream, msg.size());

      webkit_uri_scheme_response_set_status(response, 404, "Not found");
      webkit_uri_scheme_response_set_content_type(response, IPC_CONTENT_TYPE);
      webkit_uri_scheme_request_finish_with_response(request, response);
      g_object_unref(stream);
    }
  },
  router,
  0);
#endif
}

void initFunctionsTable (Router *router) {

  /**
   * Maps a message buffer bytes to an index + sequence.
   *
   * This setup allows us to push a byte array to the bridge and
   * map it to an IPC call index and sequence pair, which is reused for an
   * actual IPC call, subsequently. This is used for systems that do not support
   * a POST/PUT body in XHR requests natively, so instead we decorate
   * `message.buffer` with already an mapped buffer.
   */
  router->map("buffer.map", false, [](auto message, auto router, auto reply) {
    router->setMappedBuffer(
      message.index,
      message.seq,
      message.buffer.bytes,
      message.buffer.size
    );
  });

  /**
   * Look up an IP address by `hostname`.
   * @param hostname Host name to lookup
   * @param family IP address family to resolve [default = 0 (AF_UNSPEC)]
   */
  router->map("dns.lookup", [=](auto message, auto router, auto reply) {
    auto hostname = message.get("hostname");
    auto err = validateMessageParameters(message, {"hostname"});
    auto seq = message.seq;
    int family = 0;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (message.has("family")) {
      getMessageParam(family, "family", std::stoi);
    }

    auto options = Core::DNS::LookupOptions { hostname, family };
    router->core->dns.lookup(seq, options, resultCallback(message, reply));
  });

  /**
   * @param path
   * @param mode
   */
  router->map("fs.access", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "mode"});
    auto seq = message.seq;
    auto path = message.get("path");
    int mode = 0;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    getMessageParam(mode, "mode", std::stoi);

    router->core->fs.access(seq, path, mode, resultCallback(message, reply));
  });

  /**
   * Returns a mapping of file system constants.
   */
  router->map("fs.constants", [=](auto message, auto router, auto reply) {
    router->core->fs.constants(message.seq, resultCallback(message, reply));
  });

  /**
   * @param path
   * @param mode
   */
  router->map("fs.chmod", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "mode"});
    auto seq = message.seq;
    auto path = message.get("path");
    int mode = 0;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    getMessageParam(mode, "mode", std::stoi);

    router->core->fs.chmod(seq, path, mode, resultCallback(message, reply));
  });

  /**
   */
  router->map("fs.chown", [=](auto message, auto router, auto reply) {
  });

  /**
   * @param id
   */
  router->map("fs.close", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;
    uint64_t id;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    getMessageParam(id, "id", std::stoull);

    router->core->fs.close(seq, id, resultCallback(message, reply));
  });

  /**
   * @param id
   */
  router->map("fs.closedir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;
    uint64_t id;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    getMessageParam(id, "id", std::stoull);

    router->core->fs.closedir(seq, id, resultCallback(message, reply));
  });

  /**
   * @param id
   */
  router->map("fs.closeOpenDescriptor", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->fs.closeOpenDescriptor(seq, id, resultCallback(message, reply));
  });

  /**
   * @param preserveRetained (default: true)
   */
  router->map("fs.closeOpenDescriptors", [=](auto message, auto router, auto reply) {
    auto preserveRetained = message.get("retain") != "false";
    auto seq = message.seq;
    router->core->fs.closeOpenDescriptor(seq, preserveRetained, resultCallback(message, reply));
  });

  /**
   * @param src
   * @param dest
   */
  router->map("fs.copyFile", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"src", "dest"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto src = message.get("src");
    auto dest = message.get("dest");
    int mode = 0;

    try { mode = std::stoi(message.get("mode")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'mode' given in parameters"}
      }});
    }

    router->core->fs.copyFile(seq, src, dest, mode, resultCallback(message, reply));
  });

  /**
   * @param id
   */
  router->map("fs.fstat", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->fs.fstat(seq, id, resultCallback(message, reply));
  });

  /**
   */
  router->map("fs.getOpenDescriptors", [=](auto message, auto router, auto reply) {
    auto seq = message.seq;
    router->core->fs.getOpenDescriptors(seq, resultCallback(message, reply));
  });

  /**
   * @param path
   */
  router->map("fs.lstat", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto path = message.get("path");

    router->core->fs.lstat(seq, path, resultCallback(message, reply));
  });

  /**
   * @param path
   */
  router->map("fs.mkdir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "mode"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto path = message.get("path");
    int mode = 0;

    try { mode = std::stoi(message.get("mode")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'mode' given in parameters"}
      }});
    }

    router->core->fs.mkdir(seq, path, mode, resultCallback(message, reply));
  });

  /**
   * @param id
   * @param path
   * @param flags
   * @param mode
   */
  router->map("fs.open", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "path", "flags", "mode"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    int flags = 0;
    int mode = 0;

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { flags = std::stoi(message.get("flags")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'flags' given in parameters"}
      }});
    }

    try { mode = std::stoi(message.get("mode")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'mode' given in parameters"}
      }});
    }

    auto path = message.get("path");
    auto seq = message.seq;

    router->core->fs.open(seq, id, path, flags, mode, resultCallback(message, reply));
  });

  /**
   * @param id
   * @param path
   */
  router->map("fs.opendir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    auto path = message.get("path");
    auto seq = message.seq;

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->fs.opendir(seq, id, path, resultCallback(message, reply));
  });

  /**
   * @param id
   * @param size
   * @param offset
   */
  router->map("fs.read", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "size", "offset"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    int offset = 0;
    int size = 0;

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { size = std::stoi(message.get("size")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'size' given in parameters"}
      }});
    }

    try { offset = std::stoi(message.get("offset")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'offset' given in parameters"}
      }});
    }

    auto seq = message.seq;

    router->core->fs.read(seq, id, size, offset, resultCallback(message, reply));
  });

  /**
   * @param id
   * @param entries (default: 256)
   */
  router->map("fs.readdir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    int entries = 0;

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { entries = std::stoi(message.get("entries", "256")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'entries' given in parameters"}
      }});
    }

    auto path = message.get("path");
    auto seq = message.seq;

    router->core->fs.readdir(seq, id, entries, resultCallback(message, reply));
  });

  /**
   * @param id
   */
  router->map("fs.retainOpenDescriptor", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->fs.retainOpenDescriptor(seq, id, resultCallback(message, reply));
  });

  /**
   * @param src
   * @param dest
   */
  router->map("fs.rename", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"src", "dest"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto seq = message.seq;
    auto src = message.get("src");
    auto dest = message.get("dest");

    router->core->fs.rename(seq, src, dest, resultCallback(message, reply));
  });

  /**
   * @param path
   */
  router->map("fs.rmdir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto seq = message.seq;
    auto path = message.get("path");

    router->core->fs.rmdir(seq, path, resultCallback(message, reply));
  });

  /**
   * @param path
   */
  router->map("fs.stat", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto seq = message.seq;
    auto path = message.get("path");

    router->core->fs.stat(seq, path, resultCallback(message, reply));
  });

  /**
   * @param path
   */
  router->map("fs.unlink", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto seq = message.seq;
    auto path = message.get("path");

    router->core->fs.unlink(seq, path, resultCallback(message, reply));
  });

  /**
   * Writes buffer at `message.buffer.bytes` of size `message.buffers.size`
   * at `offset` for an opened file handle.
   * @param id Handle ID for an open file descriptor
   * @param offset The offset to start writing at
   */
  router->map("fs.write", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "offset"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (message.buffer.bytes == nullptr || message.buffer.size == 0) {
      auto err = JSON::Object::Entries {{ "message", "Missing buffer in message" }};
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    int offset = 0;

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { offset = std::stoi(message.get("offset")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'offset' given in parameters"}
      }});
    }

    auto bytes = message.buffer.bytes;
    auto size = message.buffer.size;
    auto seq = message.seq;

    router->core->fs.write(seq, id, bytes.get(), size, offset, resultCallback(message, reply));
  });

  /**
   * Read or modify the `SEND_BUFFER` or `RECV_BUFFER` for a peer socket.
   * @param id Handle ID for the buffer to read/modify
   * @param size If given, the size to set in the buffer [default = 0]
   * @param buffer The buffer to read/modify (SEND_BUFFER, RECV_BUFFER) [default = 0 (SEND_BUFFER)]
   */
  router->map("os.bufferSize", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto buffer = std::stoi(message.get("buffer", "0"));
    auto size = std::stoi(message.get("size", "0"));
    auto seq = message.seq;
    auto id  = std::stoull(message.get("id"));

    router->core->os.bufferSize(seq, id, size, buffer, resultCallback(message, reply));
  });

  /**
   * Returns the platform OS.
   */
  router->map("os.platform", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = platform.os;
    reply(result);
  });

  /**
   * Returns the platform type.
   */
  router->map("os.type", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = platform.os;
    reply(result);
  });

  /**
   * Returns the platform architecture.
   */
  router->map("os.arch", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = platform.arch;
    reply(result);
  });

  /**
   * Returns a mapping of network interfaces.
   */
  router->map("os.networkInterfaces", [=](auto message, auto router, auto reply) {
    router->core->os.networkInterfaces(message.seq, resultCallback(message, reply));
  });

  /**
   * Simply returns `pong`.
   */
  router->map("ping", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = "pong";
    reply(result);
  });

  /**
   * Handles platform events.
   * @param value The event name [domcontentloaded]
   * @param data Optional data associated with the platform event.
   */
  router->map("platform.event", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    auto value = message.value;
    auto data = message.get("data");
    auto seq = message.seq;

    router->core->platform.event(seq, value, data, resultCallback(message, reply));
  });

  /**
   * Returns pending post data typically returned in the response of an
   * `ipc://post` IPC call intercepted by an XHR request.
   * @param id The id of the post data.
   */
  router->map("post", [](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    if (!router->core->hasPost(id)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'id' for post"}
      }});
    }

    auto result = Result { message.seq, message };
    result.post = router->core->getPost(id);
    reply(result);
    router->core->removePost(id);
  });

  /**
   * Prints incoming message value to stdout.
   */
  router->map("stdout", [](auto message, auto router, auto reply) {
#if defined(__ANDROID__)
  // @TODO(jwerle): implement this
#else
      stdWrite(message.value, false);
#endif
  });

  /**
   * Prints incoming message value to stderr.
   */
  router->map("stderr", [](auto message, auto router, auto reply) {
#if defined(__ANDROID__)
  // @TODO(jwerle): implement this
#else
      stdWrite(message.value, true);
#endif
  });

  /**
   * Binds an UDP socket to a specified port, and optionally a host
   * address (default: 0.0.0.0).
   * @param id Handle ID of underlying socket
   * @param port Port to bind the UDP socket to
   * @param address The address to bind the UDP socket to (default: 0.0.0.0)
   * @param reuseAddr Reuse underlying UDP socket address (default: false)
   */
  router->map("udp.bind", [=](auto message, auto router, auto reply) {
    Core::UDP::BindOptions options;
    uint64_t id;
    auto err = validateMessageParameters(message, {"id", "port"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { options.port = std::stoi(message.get("port")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'port' given in parameters"}
      }});
    }

    options.reuseAddr = message.get("reuseAddr") == "true";
    options.address = message.get("address", "0.0.0.0");

    router->core->udp.bind(seq, id, options, resultCallback(message, reply));
  });

  /**
   * Close socket handle and underlying UDP socket.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.close", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.close(seq, id, resultCallback(message, reply));
  });

  /**
   * Connects an UDP socket to a specified port, and optionally a host
   * address (default: 0.0.0.0).
   * @param id Handle ID of underlying socket
   * @param port Port to connect the UDP socket to
   * @param address The address to connect the UDP socket to (default: 0.0.0.0)
   */
  router->map("udp.connect", [=](auto message, auto router, auto reply) {
    Core::UDP::ConnectOptions options;
    uint64_t id;
    auto err = validateMessageParameters(message, {"id", "port"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { options.port = std::stoi(message.get("port")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'port' given in parameters"}
      }});
    }

    options.address = message.get("address", "0.0.0.0");

    router->core->udp.connect(seq, id, options, resultCallback(message, reply));
  });

  /**
   * Disconnects a connected socket handle and underlying UDP socket.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.disconnect", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.disconnect(seq, id, resultCallback(message, reply));
  });

  /**
   * Returns connected peer socket address information.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.getPeerName", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.getPeerName(seq, id, resultCallback(message, reply));
  });

  /**
   * Returns local socket address information.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.getSockName", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.getSockName(seq, id, resultCallback(message, reply));
  });

  /**
   * Returns socket state information.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.getState", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.getState(seq, id, resultCallback(message, reply));
  });

  /**
   * Initializes socket handle to start receiving data from the underlying
   * socket and route through the IPC bridge to the WebView.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.readStart", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.readStart(seq, id, resultCallback(message, reply));
  });

  /**
   * Stops socket handle from receiving data from the underlying
   * socket and routing through the IPC bridge to the WebView.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.readStop", [=](auto message, auto router, auto reply) {
    uint64_t id;
    auto err = validateMessageParameters(message, {"id"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.readStop(seq, id, resultCallback(message, reply));
  });

  /**
   * Broadcasts a datagram on the socket. For connectionless sockets, the
   * destination port and address must be specified. Connected sockets, on the
   * other hand, will use their associated remote endpoint, so the port and
   * address arguments must not be set.
   * @param id Handle ID of underlying socket
   * @param port The port to send data to
   * @param size The size of the bytes to send
   * @param bytes A pointer to the bytes to send
   * @param address The address to send to (default: 0.0.0.0)
   * @param ephemeral Indicates that the socket handle, if created is ephemeral and should eventually be destroyed
   */
  router->map("udp.send", [=](auto message, auto router, auto reply) {
    Core::UDP::SendOptions options;
    uint64_t id;
    auto err = validateMessageParameters(message, {"id", "port"});
    auto seq = message.seq;

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { options.port = std::stoi(message.get("port")); } catch (...) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'port' given in parameters"}
      }});
    }

    options.size = message.buffer.size;
    options.bytes = message.buffer.bytes.get();
    options.address = message.get("address", "0.0.0.0");
    options.ephemeral = message.get("ephemeral") == "true";

    router->core->udp.send(seq, id, options, resultCallback(message, reply));
  });
}

namespace SSC::IPC {
  Bridge::Bridge (Core *core) : router(core) {
    this->core = core;
  }

  bool Router::hasMappedBuffer (int index, const Message::Seq seq) {
    Lock lock(this->mutex);
    auto key = std::to_string(index) + seq;
    return this->buffers.find(key) != this->buffers.end();
  }

  MessageBuffer Router::getMappedBuffer (int index, const Message::Seq seq) {
    if (this->hasMappedBuffer(index, seq)) {
      Lock lock(this->mutex);
      auto key = std::to_string(index) + seq;
      return this->buffers.at(key);
    }

    return MessageBuffer {};
  }

  void Router::setMappedBuffer (
    int index,
    const Message::Seq seq,
    std::shared_ptr<char []> bytes,
    size_t size
  ) {
    Lock lock(this->mutex);
    auto key = std::to_string(index) + seq;
    this->buffers.insert_or_assign(key, MessageBuffer { bytes, size });
  }

  void Router::removeMappedBuffer (int index, const Message::Seq seq) {
    Lock lock(this->mutex);
    if (this->hasMappedBuffer(index, seq)) {
      auto key = std::to_string(index) + seq;
      this->buffers.erase(key);
    }
  }

  bool Bridge::route (const String& msg, char *bytes, size_t size) {
    auto message = Message { msg, bytes, size };
    return this->router.invoke(message);
  }

  Router::Router () {
    initFunctionsTable(this);
    registerSchemeHandler(this);
  }

  Router::Router (Core *core) : Router() {
    this->core = core;
  }

  void Router::map (const String& name, MessageCallback callback) {
    return this->map(name, true, callback);
  }

  void Router::map (const String& name, bool async, MessageCallback callback) {
    if (callback != nullptr) {
      table.insert_or_assign(name, MessageCallbackContext { async, callback });
    }
  }

  void Router::unmap (const String& name) {
    if (table.find(name) != table.end()) {
      table.erase(name);
    }
  }

  bool Router::invoke (const String& name, char *bytes, size_t size) {
    auto message = Message { name, bytes, size };
    return this->invoke(message);
  }

  bool Router::invoke (const String& name, char *bytes, size_t size, ResultCallback callback) {
    auto message = Message { name, bytes, size };
    return this->invoke(message);
  }

  bool Router::invoke (const Message& message) {
    return this->invoke(message, [this](auto result) {
      this->send(result.seq, result.str(), result.post);
    });
  }

  bool Router::invoke (const Message& message, ResultCallback callback) {
    if (this->table.find(message.name) == this->table.end()) {
      return false;
    }

    auto ctx = this->table.at(message.name);

    if (ctx.callback != nullptr) {
      Message msg(message);
      // decorate message with buffer if buffer was previously
      // mapped with `ipc://buffer.map`, which we do on Linux
      if (this->hasMappedBuffer(msg.index, msg.seq)) {
        msg.buffer = this->getMappedBuffer(msg.index, msg.seq);
        this->removeMappedBuffer(msg.index, msg.seq);
      }

      if (ctx.async) {
        return this->dispatch([ctx, msg, callback, this] {
          ctx.callback(msg, this, callback);
        });
      } else {
        ctx.callback(msg, this, callback);
        return true;
      }
    }

    return false;
  }

  bool Router::send (
    const Message::Seq& seq,
    const String& data,
    const Post post
  ) {
    if (post.body || seq == "-1") {
      auto script = this->core->createPost(seq, data, post);
      return this->evaluateJavaScript(script);
    }

    // this had a sequence, we need to try to resolve it.
    if (seq != "-1" && seq.size() > 0) {
      auto value = encodeURIComponent(data);
      auto script = getResolveToRenderProcessJavaScript(seq, "0", value);
      return this->evaluateJavaScript(script);
    }

    if (data.size() > 0) {
      return this->evaluateJavaScript(data);
    }

    return false;
  }

  bool Router::emit (
    const String& name,
    const String& data
  ) {
    auto value = encodeURIComponent(data);
    auto script = getEmitToRenderProcessJavaScript(name, value);
    return this->evaluateJavaScript(script);
  }

  bool Router::evaluateJavaScript (const String js) {
    if (this->evaluateJavaScriptFunction != nullptr) {
      this->evaluateJavaScriptFunction(js);
      return true;
    }

    return false;
  }

  bool Router::dispatch (DispatchCallback callback) {
    if (this->dispatchFunction != nullptr) {
      this->dispatchFunction(callback);
      return true;
    }

    return false;
  }
}
