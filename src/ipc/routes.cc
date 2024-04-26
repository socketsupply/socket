#include "../core/types.hh"
#include "../core/json.hh"
#include "../extension/extension.hh"
#include "../window/window.hh"
#include "ipc.hh"

#define REQUIRE_AND_GET_MESSAGE_VALUE(var, name, parse, ...)                   \
  try {                                                                        \
    var = parse(message.get(name, ##__VA_ARGS__));                             \
  } catch (...) {                                                              \
    return reply(Result::Err { message, JSON::Object::Entries {                \
      {"message", "Invalid '" name "' given in parameters"}                    \
    }});                                                                       \
  }

#define RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)                     \
  [message, reply](auto seq, auto json, auto post) {                           \
    reply(Result { seq, message, json, post });                                \
  }

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

static void mapIPCRoutes (Router *router) {
  auto userConfig = router->bridge->userConfig;
#if defined(__APPLE__)
  auto bundleIdentifier = userConfig["meta_bundle_identifier"];
  auto SSC_OS_LOG_BUNDLE = os_log_create(bundleIdentifier.c_str(),
  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    "socket.runtime.mobile"
  #else
    "socket.runtime.desktop"
  #endif
  );
#endif

  /**
   * Starts a bluetooth service
   * @param serviceId
   */
  router->map("bluetooth.start", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"serviceId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (router->bridge->userConfig["permissions_allow_bluetooth"] == "false") {
      auto err =JSON::Object::Entries {
        {"message", "Bluetooth is not allowed"}
      };

      return reply(Result::Err { message, err });
    }

    router->bridge->bluetooth.startService(
      message.seq,
      message.get("serviceId"),
      [reply, message](auto seq, auto json) {
        reply(Result { seq, message, json });
      }
    );
  });

  /**
   * Subscribes to a characteristic for a service.
   * @param serviceId
   * @param characteristicId
   */
  router->map("bluetooth.subscribe", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {
      "characteristicId",
      "serviceId"
    });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (router->bridge->userConfig["permissions_allow_bluetooth"] == "false") {
      auto err =JSON::Object::Entries {
        {"message", "Bluetooth is not allowed"}
      };

      return reply(Result::Err { message, err });
    }

    router->bridge->bluetooth.subscribeCharacteristic(
      message.seq,
      message.get("serviceId"),
      message.get("characteristicId"),
      [reply, message](auto seq, auto json) {
        reply(Result { seq, message, json });
      }
    );
  });

  /**
   * Publishes data to a characteristic for a service.
   * @param serviceId
   * @param characteristicId
   */
  router->map("bluetooth.publish", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {
      "characteristicId",
      "serviceId"
    });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (router->bridge->userConfig["permissions_allow_bluetooth"] == "false") {
      auto err =JSON::Object::Entries {
        {"message", "Bluetooth is not allowed"}
      };

      return reply(Result::Err { message, err });
    }

    auto bytes = *message.buffer.bytes;
    auto size = message.buffer.size;

    if (bytes == nullptr) {
      bytes = message.value.data();
      size = message.value.size();
    }

    router->bridge->bluetooth.publishCharacteristic(
      message.seq,
      bytes,
      size,
      message.get("serviceId"),
      message.get("characteristicId"),
      [reply, message](auto seq, auto json) {
        reply(Result { seq, message, json });
      }
    );
  });

  /**
   * Maps a message buffer bytes to an index + sequence.
   *
   * This setup allows us to push a byte array to the bridge and
   * map it to an IPC call index and sequence pair, which is reused for an
   * actual IPC call, subsequently. This is used for systems that do not support
   * a POST/PUT body in XHR requests natively, so instead we decorate
   * `message.buffer` with already an mapped buffer.
   */
  router->map("buffer.map", false, [=](auto message, auto router, auto reply) {
    router->setMappedBuffer(message.index, message.seq, message.buffer);
    reply(Result { message.seq, message });
  });

  /**
   * Kills an already spawned child process.
   *
   * @param id
   * @param signal
   */
  router->map("child_process.kill", [=](auto message, auto router, auto reply) {
  #if SSC_PLATFORM_IOS
    auto err = JSON::Object::Entries {
      {"type", "NotSupportedError"},
      {"message", "Operation is not supported on this platform"}
    };

    return reply(Result::Err { message, err });
  #else
    auto err = validateMessageParameters(message, {"id", "signal"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    int signal;
    REQUIRE_AND_GET_MESSAGE_VALUE(signal, "signal", std::stoi);

    router->core->childProcess.kill(
      message.seq,
      id,
      signal,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  #endif
  });

  /**
   * Spawns a child process
   *
   * @param id
   * @param args (command, ...args)
   */
  router->map("child_process.spawn", [=](auto message, auto router, auto reply) {
  #if SSC_PLATFORM_IOS
    auto err = JSON::Object::Entries {
      {"type", "NotSupportedError"},
      {"message", "Operation is not supported on this platform"}
    };

    return reply(Result::Err { message, err });
  #else
    auto err = validateMessageParameters(message, {"args", "id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto args = split(message.get("args"), 0x0001);

    if (args.size() == 0 || args.at(0).size() == 0) {
      auto json = JSON::Object::Entries {
        {"source", "child_process.spawn"},
        {"err", JSON::Object::Entries {
          {"message", "Spawn requires at least one argument with a length greater than zero"},
        }}
      };

      return reply(Result { message.seq, message, json });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    const auto options = Core::ChildProcess::SpawnOptions {
      .cwd = message.get("cwd", getcwd()),
      .allowStdin = message.get("stdin") != "false",
      .allowStdout = message.get("stdout") != "false",
      .allowStderr = message.get("stderr") != "false"
    };

    router->core->childProcess.spawn(
      message.seq,
      id,
      args,
      options,
      [message, reply](auto seq, auto json, auto post) {
        reply(Result { seq, message, json, post });
      }
    );
  #endif
  });

  router->map("child_process.exec", [=](auto message, auto router, auto reply) {
  #if SSC_PLATFORM_IOS
    auto err = JSON::Object::Entries {
      {"type", "NotSupportedError"},
      {"message", "Operation is not supported on this platform"}
    };

    return reply(Result::Err { message, err });
  #else
    auto err = validateMessageParameters(message, {"args", "id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto args = split(message.get("args"), 0x0001);

    if (args.size() == 0 || args.at(0).size() == 0) {
      auto json = JSON::Object::Entries {
        {"source", "child_process.exec"},
        {"err", JSON::Object::Entries {
          {"message", "Spawn requires at least one argument with a length greater than zero"},
        }}
      };

      return reply(Result { message.seq, message, json });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    uint64_t timeout = 0;
    int killSignal = 0;

    if (message.has("timeout")) {
      REQUIRE_AND_GET_MESSAGE_VALUE(timeout, "timeout", std::stoull);
    }

    if (message.has("killSignal")) {
      REQUIRE_AND_GET_MESSAGE_VALUE(killSignal, "killSignal", std::stoi);
    }

    const auto options = Core::ChildProcess::ExecOptions {
      .cwd = message.get("cwd", getcwd()),
      .allowStdout = message.get("stdout") != "false",
      .allowStderr = message.get("stderr") != "false",
      .timeout = timeout,
      .killSignal = killSignal
    };

    router->core->childProcess.exec(
      message.seq,
      id,
      args,
      options,
      [message, reply](auto seq, auto json, auto post) {
        reply(Result { seq, message, json, post });
      }
    );
  #endif
  });

  /**
   * Writes to an already spawned child process.
   *
   * @param id
   */
  router->map("child_process.write", [=](auto message, auto router, auto reply) {
  #if SSC_PLATFORM_IOS
    auto err = JSON::Object::Entries {
      {"type", "NotSupportedError"},
      {"message", "Operation is not supported on this platform"}
    };

    return reply(Result::Err { message, err });
  #else
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->childProcess.write(
      message.seq,
      id,
      *message.buffer.bytes,
      message.buffer.size,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  #endif
  });

  /**
   * Look up an IP address by `hostname`.
   * @param hostname Host name to lookup
   * @param family IP address family to resolve [default = 0 (AF_UNSPEC)]
   * @see getaddrinfo(3)
   */
  router->map("dns.lookup", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"hostname"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int family = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(family, "family", std::stoi, "0");

    router->core->dns.lookup(
      message.seq,
      Core::DNS::LookupOptions { message.get("hostname"), family },
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  router->map("extension.stats", [=](auto message, auto router, auto reply) {
    auto extensions = Extension::all();
    auto name = message.get("name");

    if (name.size() > 0) {
      auto type = Extension::getExtensionType(name);
      auto path = Extension::getExtensionPath(name);
      auto json = JSON::Object::Entries {
        {"source", "extension.stats"},
        {"data", JSON::Object::Entries {
          {"abi", SOCKET_RUNTIME_EXTENSION_ABI_VERSION},
          {"name", name},
          {"type", type},
          // `path` is absolute to the location of the resources
          {"path", String("/") + std::filesystem::relative(path, getcwd()).string()}
        }}
      };

      reply(Result { message.seq, message, json });
    } else {
      int loaded = 0;

      for (const auto& tuple : extensions) {
        if (tuple.second != nullptr) {
          loaded++;
        }
      }

      auto json = JSON::Object::Entries {
        {"source", "extension.stats"},
        {"data", JSON::Object::Entries {
          {"abi", SOCKET_RUNTIME_EXTENSION_ABI_VERSION},
          {"loaded", loaded}
        }}
      };

      reply(Result { message.seq, message, json });
    }
  });

  /**
   * Query for type of extension ('shared', 'wasm32', 'unknown')
   * @param name
   */
  router->map("extension.type", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");
    auto type = Extension::getExtensionType(name);
    auto json = SSC::JSON::Object::Entries {
      {"source", "extension.type"},
      {"data", JSON::Object::Entries {
        {"name", name},
        {"type", type}
      }}
    };

    reply(Result { message.seq, message, json });
  });

  /**
   * Load a named native extension.
   * @param name
   * @param allow
   */
  router->map("extension.load", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");

    if (!Extension::load(name)) {
      #if defined(_WIN32)
      auto error = formatWindowsError(GetLastError(), "bridge");
      #else
      auto err = dlerror();
      auto error = String(err ? err : "Unknown error");
      #endif

      std::cout << "Load extension error: " << error << std::endl;

      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Failed to load extension: '" + name + "': " + error}
      }});
    }

    auto extension = Extension::get(name);
    auto allowed = split(message.get("allow"), ',');
    auto context = Extension::getContext(name);
    auto ctx = context->memory.template alloc<Extension::Context>(context, router);

    for (const auto& value : allowed) {
      auto policy = trim(value);
      ctx->setPolicy(policy, true);
    }

    Extension::setRouterContext(name, router, ctx);

    /// init context
    if (!Extension::initialize(ctx, name, nullptr)) {
      if (ctx->state == Extension::Context::State::Error) {
        auto json = JSON::Object::Entries {
          {"source", "extension.load"},
          {"extension", name},
          {"err", JSON::Object::Entries {
            {"code", ctx->error.code},
            {"name", ctx->error.name},
            {"message", ctx->error.message},
            {"location", ctx->error.location},
          }}
        };

        reply(Result { message.seq, message, json });
      } else {
        auto json = JSON::Object::Entries {
          {"source", "extension.load"},
          {"extension", name},
          {"err", JSON::Object::Entries {
            {"message", "Failed to initialize extension: '" + name + "'"},
          }}
        };

        reply(Result { message.seq, message, json });
      }
    } else {
      auto json = JSON::Object::Entries {
        {"source", "extension.load"},
        {"data", JSON::Object::Entries {
         {"abi", (uint64_t) extension->abi},
         {"name", extension->name},
         {"version", extension->version},
         {"description", extension->description}
        }}
      };

      reply(Result { message.seq, message, json });
    }
  });

  /**
   * Unload a named native extension.
   * @param name
   */
  router->map("extension.unload", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");

    if (!Extension::isLoaded(name)) {
      return reply(Result::Err { message, JSON::Object::Entries {
      #if defined(_WIN32)
        {"message", "Extension '" + name + "' is not loaded"}
      #else
        {"message", "Extension '" + name + "' is not loaded" + String(dlerror())}
      #endif
      }});
    }

    auto extension = Extension::get(name);
    auto ctx = Extension::getRouterContext(name, router);

    if (Extension::unload(ctx, name, extension->contexts.size() == 1)) {
      Extension::removeRouterContext(name, router);
      auto json = JSON::Object::Entries {
        {"source", "extension.unload"},
        {"extension", name},
        {"data", JSON::Object::Entries {}}
      };
      return reply(Result { message.seq, message, json });
    }

    if (ctx->state == Extension::Context::State::Error) {
      auto json = JSON::Object::Entries {
        {"source", "extension.unload"},
        {"extension", name},
        {"err", JSON::Object::Entries {
          {"code", ctx->error.code},
          {"name", ctx->error.name},
          {"message", ctx->error.message},
          {"location", ctx->error.location},
        }}
      };

      reply(Result { message.seq, message, json });
    } else {
      auto json = JSON::Object::Entries {
        {"source", "extension.unload"},
        {"extension", name},
        {"err", JSON::Object::Entries {
          {"message", "Failed to unload extension: '" + name + "'"},
        }}
      };

      reply(Result { message.seq, message, json });
    }
  });

  /**
   * Checks if current user can access file at `path` with `mode`.
   * @param path
   * @param mode
   * @see access(2)
   */
  router->map("fs.access", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "mode"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int mode = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(mode, "mode", std::stoi);

    router->core->fs.access(
      message.seq,
      message.get("path"),
      mode,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Returns a mapping of file system constants.
   */
  router->map("fs.constants", [=](auto message, auto router, auto reply) {
    router->core->fs.constants(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Changes `mode` of file at `path`.
   * @param path
   * @param mode
   * @see chmod(2)
   */
  router->map("fs.chmod", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "mode"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int mode = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(mode, "mode", std::stoi);

    router->core->fs.chmod(
      message.seq,
      message.get("path"),
      mode,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Changes uid and gid of file at `path`.
   * @param path
   * @param uid
   * @param gid
   * @see chown(2)
   */
  router->map("fs.chown", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "uid", "gid"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int uid = 0;
    int gid = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(uid, "uid", std::stoi);
    REQUIRE_AND_GET_MESSAGE_VALUE(gid, "gid", std::stoi);

    router->core->fs.chown(
      message.seq,
      message.get("path"),
      static_cast<uv_uid_t>(uid),
      static_cast<uv_gid_t>(gid),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

	/**
   * Changes uid and gid of symbolic link at `path`.
   * @param path
   * @param uid
   * @param gid
   * @see lchown(2)
   */
  router->map("fs.lchown", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "uid", "gid"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int uid = 0;
    int gid = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(uid, "uid", std::stoi);
    REQUIRE_AND_GET_MESSAGE_VALUE(gid, "gid", std::stoi);

    router->core->fs.lchown(
      message.seq,
      message.get("path"),
      static_cast<uv_uid_t>(uid),
      static_cast<uv_gid_t>(gid),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Closes underlying file descriptor handle.
   * @param id
   * @see close(2)
   */
  router->map("fs.close", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.close(message.seq, id, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Closes underlying directory descriptor handle.
   * @param id
   * @see closedir(3)
   */
  router->map("fs.closedir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.closedir(message.seq, id, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Closes an open file or directory descriptor handle.
   * @param id
   * @see close(2)
   * @see closedir(3)
   */
  router->map("fs.closeOpenDescriptor", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.closeOpenDescriptor(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Closes all open file and directory descriptors, optionally preserving
   * explicitly retrained descriptors.
   * @param preserveRetained (default: true)
   * @see close(2)
   * @see closedir(3)
   */
  router->map("fs.closeOpenDescriptors", [=](auto message, auto router, auto reply) {
    router->core->fs.closeOpenDescriptor(
      message.seq,
      message.get("preserveRetained") != "false",
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Copy file at path `src` to path `dest`.
   * @param src
   * @param dest
   * @param flags
   * @see copyfile(3)
   */
  router->map("fs.copyFile", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"src", "dest", "flags"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int flags = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(flags, "flags", std::stoi);

    router->core->fs.copyFile(
      message.seq,
      message.get("src"),
      message.get("dest"),
      flags,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

	/**
   * Creates a link at `dest`
   * @param src
   * @param dest
   * @see link(2)
   */
  router->map("fs.link", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"src", "dest"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    router->core->fs.link(
      message.seq,
      message.get("src"),
      message.get("dest"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Creates a symlink at `dest`
   * @param src
   * @param dest
   * @param flags
   * @see symlink(2)
   */
  router->map("fs.symlink", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"src", "dest", "flags"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int flags = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(flags, "flags", std::stoi);

    router->core->fs.symlink(
      message.seq,
      message.get("src"),
      message.get("dest"),
      flags,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Computes stats for an open file descriptor.
   * @param id
   * @see stat(2)
   * @see fstat(2)
   */
  router->map("fs.fstat", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.fstat(message.seq, id, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Synchronize a file's in-core state with storage device
   * @param id
   * @see fsync(2)
   */
  router->map("fs.fsync", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.fsync(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Truncates opened file
   * @param id
   * @param offset
   * @see ftruncate(2)
   */
  router->map("fs.ftruncate", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "offset"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    int64_t offset;
    REQUIRE_AND_GET_MESSAGE_VALUE(offset, "offset", std::stoll);

    router->core->fs.ftruncate(
      message.seq,
      id,
      offset,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Returns all open file or directory descriptors.
   */
  router->map("fs.getOpenDescriptors", [=](auto message, auto router, auto reply) {
    router->core->fs.getOpenDescriptors(
      message.seq,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Computes stats for a symbolic link at `path`.
   * @param path
   * @see stat(2)
   * @see lstat(2)
   */
  router->map("fs.lstat", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    router->core->fs.lstat(
      message.seq,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Creates a directory at `path` with an optional mode and an optional recursive flag.
   * @param path
   * @param mode
   * @param recursive
   * @see mkdir(2)
   */
  router->map("fs.mkdir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "mode"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int mode = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(mode, "mode", std::stoi);

    router->core->fs.mkdir(
      message.seq,
      message.get("path"),
      mode,
      message.get("recursive") == "true",
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });


  /**
   * Opens a file descriptor at `path` for `id` with `flags` and `mode`
   * @param id
   * @param path
   * @param flags
   * @param mode
   * @see open(2)
   */
  router->map("fs.open", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {
      "id",
      "path",
      "flags",
      "mode"
    });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    int mode = 0;
    int flags = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(mode, "mode", std::stoi);
    REQUIRE_AND_GET_MESSAGE_VALUE(flags, "flags", std::stoi);

    router->core->fs.open(
      message.seq,
      id,
      message.get("path"),
      flags,
      mode,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Opens a directory descriptor at `path` for `id` with `flags` and `mode`
   * @param id
   * @param path
   * @see opendir(3)
   */
  router->map("fs.opendir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.opendir(
      message.seq,
      id,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Reads `size` bytes at `offset` from the underlying file descriptor.
   * @param id
   * @param size
   * @param offset
   * @see read(2)
   */
  router->map("fs.read", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "size", "offset"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    int size = 0;
    int offset = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(size, "size", std::stoi);
    REQUIRE_AND_GET_MESSAGE_VALUE(offset, "offset", std::stoi);

    router->core->fs.read(
      message.seq,
      id,
      size,
      offset,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Reads next `entries` of from the underlying directory descriptor.
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
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(entries, "entries", std::stoi);

    router->core->fs.readdir(
      message.seq,
      id,
      entries,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

	/**
   * Read value of a symbolic link at 'path'
   * @param path
   * @see readlink(2)
   */
  router->map("fs.readlink", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    router->core->fs.readlink(
      message.seq,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Get the realpath at 'path'
   * @param path
   * @see realpath(2)
   */
  router->map("fs.realpath", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    router->core->fs.realpath(
      message.seq,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Marks a file or directory descriptor as retained.
   * @param id
   */
  router->map("fs.retainOpenDescriptor", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.retainOpenDescriptor(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Renames file at path `src` to path `dest`.
   * @param src
   * @param dest
   * @see rename(2)
   */
  router->map("fs.rename", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"src", "dest"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    router->core->fs.rename(
      message.seq,
      message.get("src"),
      message.get("dest"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Removes file at `path`.
   * @param path
   * @see rmdir(2)
   */
  router->map("fs.rmdir", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    router->core->fs.rmdir(
      message.seq,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Computes stats for a file at `path`.
   * @param path
   * @see stat(2)
   */
  router->map("fs.stat", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    router->core->fs.stat(
      message.seq,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Stops a already started watcher
   */
  router->map("fs.stopWatch", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.watch(
      message.seq,
      id,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Removes a file or empty directory at `path`.
   * @param path
   * @see unlink(2)
   */
  router->map("fs.unlink", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    router->core->fs.unlink(
      message.seq,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * TODO
   */
  router->map("fs.watch", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "path"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->fs.watch(
      message.seq,
      id,
      message.get("path"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Writes buffer at `message.buffer.bytes` of size `message.buffers.size`
   * at `offset` for an opened file handle.
   * @param id Handle ID for an open file descriptor
   * @param offset The offset to start writing at
   * @see write(2)
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
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(offset, "offset", std::stoi);

    router->core->fs.write(
      message.seq,
      id,
      *message.buffer.bytes,
      message.buffer.size,
      offset,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

#if defined(__APPLE__)
  router->map("geolocation.getCurrentPosition", [=](auto message, auto router, auto reply) {
    router->core->geolocation.getCurrentPosition(
      message.seq,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  router->map("geolocation.watchPosition", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int id = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoi);

    router->core->geolocation.watchPosition(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  router->map("geolocation.clearWatch", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int id = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoi);
    router->core->geolocation.clearWatch(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );

    reply(Result { message.seq, message, JSON::Object{} });
  });
#endif

  /**
   * A private API for artifically setting the current cached CWD value.
   * This is only useful on platforms that need to set this value from an
   * external source, like Android or ChromeOS.
   */
  router->map("internal.setcwd", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    setcwd(message.value);
    reply(Result { message.seq, message, JSON::Object{} });
  });

  /**
   * Log `value to stdout` with platform dependent logger.
   * @param value
   */
  router->map("log", [=](auto message, auto router, auto reply) {
    auto value = message.value.c_str();
  #if defined(__APPLE__)
    NSLog(@"%s", value);
    os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_INFO, "%{public}s", value);
  #elif defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_DEBUG, "", "%s", value);
  #else
    printf("%s\n", value);
  #endif
  });

  router->map("notification.show", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {
      "id",
      "title"
    });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    const auto options = Notifications::ShowOptions {
      message.get("id"),
      message.get("title"),
      message.get("tag"),
      message.get("lang"),
      message.get("silent") == "true",
      message.get("icon"),
      message.get("image"),
      message.get("body")
    };

    router->bridge->core->notifications.show(options, [=] (const auto result) {
      if (result.error.size() > 0) {
        const auto err = JSON::Object::Entries {{ "message", result.error }};
        return reply(Result::Err { message, err });
      }

      const auto data = JSON::Object::Entries {{"id", result.notification.identifier}};
      reply(Result::Data { message, data });
    });
  });

  router->map("notification.close", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, { "id" });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }


    const auto notification = Notifications::Notification(message.get("id"));
    router->core->notifications.close(notification);

    reply(Result { message.seq, message, JSON::Object::Entries {
      {"id", notification.identifier}
    }});
  });

  router->map("notification.list", [=](auto message, auto router, auto reply) {
    router->core->notifications.list([=](const auto notifications) {
      JSON::Array entries;
      for (const auto& notification : notifications) {
        entries.push(notification.json());
      }

      reply(Result::Data { message.seq, entries, });
    });
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

    uint64_t id;
    int buffer = 0;
    int size = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(buffer, "buffer", std::stoi, "0");
    REQUIRE_AND_GET_MESSAGE_VALUE(size, "size", std::stoi, "0");

    router->core->os.bufferSize(
      message.seq,
      id,
      size,
      buffer,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Returns a mapping of operating  system constants.
   */
  router->map("os.constants", [=](auto message, auto router, auto reply) {
    router->core->os.constants(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Returns a mapping of network interfaces.
   */
  router->map("os.networkInterfaces", [=](auto message, auto router, auto reply) {
    router->core->os.networkInterfaces(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Returns an array of CPUs available to the process.
   */
  router->map("os.cpus", [=](auto message, auto router, auto reply) {
    router->core->os.cpus(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.rusage", [=](auto message, auto router, auto reply) {
    router->core->os.rusage(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.uptime", [=](auto message, auto router, auto reply) {
    router->core->os.uptime(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.uname", [=](auto message, auto router, auto reply) {
    router->core->os.uname(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.hrtime", [=](auto message, auto router, auto reply) {
    router->core->os.hrtime(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.availableMemory", [=](auto message, auto router, auto reply) {
    router->core->os.availableMemory(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.paths", [=](auto message, auto router, auto reply) {
    const auto bundleIdentifier = router->bridge->userConfig["meta_bundle_identifier"];

    JSON::Object json;

    // paths
    String resources = getcwd();
    String downloads;
    String documents;
    String pictures;
    String desktop;
    String videos;
    String config;
    String music;
    String home;
    String data;
    String log;
    String tmp = fs::temp_directory_path().string();

  #if defined(__APPLE__)
    static const auto uid = getuid();
    static const auto pwuid = getpwuid(uid);
    static const auto HOME = pwuid != nullptr
      ? String(pwuid->pw_dir)
      : Env::get("HOME", getcwd());

    static const auto fileManager = NSFileManager.defaultManager;

  #define DIRECTORY_PATH_FROM_FILE_MANAGER(type) (                             \
    String([fileManager                                                        \
        URLForDirectory: type                                                  \
               inDomain: NSUserDomainMask                                      \
      appropriateForURL: nil                                                   \
                 create: NO                                                    \
                  error: nil                                                   \
      ].path.UTF8String)                                                       \
    )

    // overload with main bundle resources path for macos/ios
    resources = String(NSBundle.mainBundle.resourcePath.UTF8String);
    downloads = DIRECTORY_PATH_FROM_FILE_MANAGER(NSDownloadsDirectory);
    documents = DIRECTORY_PATH_FROM_FILE_MANAGER(NSDocumentDirectory);
    pictures = DIRECTORY_PATH_FROM_FILE_MANAGER(NSPicturesDirectory);
    desktop = DIRECTORY_PATH_FROM_FILE_MANAGER(NSDesktopDirectory);
    videos = DIRECTORY_PATH_FROM_FILE_MANAGER(NSMoviesDirectory);
    music = DIRECTORY_PATH_FROM_FILE_MANAGER(NSMusicDirectory);
    config = HOME + "/Library/Application Support/" + bundleIdentifier;
    home = String(NSHomeDirectory().UTF8String);
    data = HOME + "/Library/Application Support/" + bundleIdentifier;
    log = HOME + "/Library/Logs/" + bundleIdentifier;
    tmp = String(NSTemporaryDirectory().UTF8String);

  #undef DIRECTORY_PATH_FROM_FILE_MANAGER

  #elif defined(__linux__)
    static const auto uid = getuid();
    static const auto pwuid = getpwuid(uid);
    static const auto HOME = pwuid != nullptr
      ? String(pwuid->pw_dir)
      : Env::get("HOME", getcwd());

    static const auto XDG_DOCUMENTS_DIR = Env::get("XDG_DOCUMENTS_DIR");
    static const auto XDG_DOWNLOAD_DIR = Env::get("XDG_DOWNLOAD_DIR");
    static const auto XDG_PICTURES_DIR = Env::get("XDG_PICTURES_DIR");
    static const auto XDG_DESKTOP_DIR = Env::get("XDG_DESKTOP_DIR");
    static const auto XDG_VIDEOS_DIR = Env::get("XDG_VIDEOS_DIR");
    static const auto XDG_MUSIC_DIR = Env::get("XDG_MUSIC_DIR");

    static const auto XDG_CONFIG_HOME = Env::get("XDG_CONFIG_HOME", HOME + "/.config");
    static const auto XDG_DATA_HOME = Env::get("XDG_DATA_HOME", HOME + "/.local/share");

    if (XDG_DOCUMENTS_DIR.size() > 0) {
      documents = XDG_DOCUMENTS_DIR;
    } else {
      documents = (Path(HOME) / "Documents").string();
    }

    if (XDG_DOWNLOAD_DIR.size() > 0) {
      downloads = XDG_DOWNLOAD_DIR;
    } else {
      downloads = (Path(HOME) / "Downloads").string();
    }

    if (XDG_DESKTOP_DIR.size() > 0) {
      desktop = XDG_DESKTOP_DIR;
    } else {
      desktop = (Path(HOME) / "Desktop").string();
    }

    if (XDG_PICTURES_DIR.size() > 0) {
      pictures = XDG_PICTURES_DIR;
    } else if (fs::exists(Path(HOME) / "Images")) {
      pictures = (Path(HOME) / "Images").string();
    } else if (fs::exists(Path(HOME) / "Photos")) {
      pictures = (Path(HOME) / "Photos").string();
    } else {
      pictures = (Path(HOME) / "Pictures").string();
    }

    if (XDG_VIDEOS_DIR.size() > 0) {
      videos = XDG_VIDEOS_DIR;
    } else {
      videos = (Path(HOME) / "Videos").string();
    }

    if (XDG_MUSIC_DIR.size() > 0) {
      music = XDG_MUSIC_DIR;
    } else {
      music = (Path(HOME) / "Music").string();
    }

    config = XDG_CONFIG_HOME + "/" + bundleIdentifier;
    home = Path(HOME).string();
    data = XDG_DATA_HOME + "/" + bundleIdentifier;
    log = config;
  #elif defined(_WIN32)
    static const auto HOME = Env::get("HOMEPATH", Env::get("HOME"));
    static const auto USERPROFILE = Env::get("USERPROFILE", HOME);
    downloads = (Path(USERPROFILE) / "Downloads").string();
    documents = (Path(USERPROFILE) / "Documents").string();
    desktop = (Path(USERPROFILE) / "Desktop").string();
    pictures = (Path(USERPROFILE) / "Pictures").string();
    videos = (Path(USERPROFILE) / "Videos").string();
    music = (Path(USERPROFILE) / "Music").string();
    config = (Path(Env::get("APPDATA")) / bundleIdentifier).string();
    home = Path(USERPROFILE).string();
    data = (Path(Env::get("APPDATA")) / bundleIdentifier).string();
    log = config;
  #endif

    json["resources"] = resources;
    json["downloads"] = downloads;
    json["documents"] = documents;
    json["pictures"] = pictures;
    json["desktop"] = desktop;
    json["videos"] = videos;
    json["music"] = music;
    json["config"] = config;
    json["home"] = home;
    json["data"] = data;
    json["log"] = log;
    json["tmp"] = tmp;

    return reply(Result::Data { message, json });
  });

  router->map("permissions.query", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");

  #if defined(__APPLE__)
    if (name == "geolocation") {
      if (router->core->geolocation.locationObserver.isAuthorized) {
        auto data = JSON::Object::Entries {{"state", "granted"}};
        return reply(Result::Data { message, data });
      } else if (router->core->geolocation.locationObserver.locationManager) {
        auto authorizationStatus = (
          router->core->geolocation.locationObserver.locationManager.authorizationStatus
        );

        if (authorizationStatus == kCLAuthorizationStatusDenied) {
          auto data = JSON::Object::Entries {{"state", "denied"}};
          return reply(Result::Data { message, data });
        } else {
          auto data = JSON::Object::Entries {{"state", "prompt"}};
          return reply(Result::Data { message, data });
        }
      }

      auto data = JSON::Object::Entries {{"state", "denied"}};
      return reply(Result::Data { message, data });
    }

    if (name == "notifications") {
      auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
      [notificationCenter getNotificationSettingsWithCompletionHandler: ^(UNNotificationSettings *settings) {
        if (!settings) {
          auto err = JSON::Object::Entries {{ "message", "Failed to reach user notification settings" }};
          return reply(Result::Err { message, err });
        }

        if (settings.authorizationStatus == UNAuthorizationStatusDenied) {
          auto data = JSON::Object::Entries {{"state", "denied"}};
          return reply(Result::Data { message, data });
        } else if (settings.authorizationStatus == UNAuthorizationStatusNotDetermined) {
          auto data = JSON::Object::Entries {{"state", "prompt"}};
          return reply(Result::Data { message, data });
        }

        auto data = JSON::Object::Entries {{"state", "granted"}};
        return reply(Result::Data { message, data });
      }];
    }
  #endif
  });

  router->map("permissions.request", [=](auto message, auto router, auto reply) {
  #if defined(__APPLE__)
    __block auto userConfig = router->bridge->userConfig;
  #else
    auto userConfig = router->bridge->userConfig;
  #endif

    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");

    if (name == "geolocation") {
    #if defined(__APPLE__)
      auto performedActivation = [router->core->geolocation.locationObserver attemptActivationWithCompletion: ^(BOOL isAuthorized) {
        if (!isAuthorized) {
          auto reason = @("Location observer could not be activated");

          if (!router->core->geolocation.locationObserver.locationManager) {
            reason = @("Location observer manager is not initialized");
          } else if (!router->core->geolocation.locationObserver.locationManager.location) {
            reason = @("Location observer manager could not provide location");
          }

          auto error = [NSError
            errorWithDomain: @(userConfig["meta_bundle_identifier"].c_str())
            code: -1
            userInfo: @{
              NSLocalizedDescriptionKey: reason
            }
          ];
        }

        if (isAuthorized) {
          auto data = JSON::Object::Entries {{"state", "granted"}};
          return reply(Result::Data { message, data });
        } else if (router->core->geolocation.locationObserver.locationManager.authorizationStatus == kCLAuthorizationStatusNotDetermined) {
          auto data = JSON::Object::Entries {{"state", "prompt"}};
          return reply(Result::Data { message, data });
        } else {
          auto data = JSON::Object::Entries {{"state", "denied"}};
          return reply(Result::Data { message, data });
        }
      }];

      if (!performedActivation) {
        auto err = JSON::Object::Entries {{ "message", "Location observer could not be activated" }};
        err["type"] = "GeolocationPositionError";
        return reply(Result::Err { message, err });
      }

      return;
    #endif
    }

    if (name == "notifications") {
    #if defined(__APPLE__)
      UNAuthorizationOptions options = UNAuthorizationOptionProvisional;
      auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
      auto requestAlert = message.get("alert") == "true";
      auto requestBadge = message.get("badge") == "true";
      auto requestSound = message.get("sound") == "true";

      if (requestAlert) {
        options |= UNAuthorizationOptionAlert;
      }

      if (requestBadge) {
        options |= UNAuthorizationOptionBadge;
      }

      if (requestSound) {
        options |= UNAuthorizationOptionSound;
      }

      if (requestAlert && requestSound) {
        options |= UNAuthorizationOptionCriticalAlert;
      }

      [notificationCenter
        requestAuthorizationWithOptions: options
                      completionHandler: ^(BOOL granted, NSError *error) {
        [notificationCenter
          getNotificationSettingsWithCompletionHandler: ^(UNNotificationSettings *settings) {
          if (settings.authorizationStatus == UNAuthorizationStatusDenied) {
            auto data = JSON::Object::Entries {{"state", "denied"}};
            return reply(Result::Data { message, data });
          } else if (settings.authorizationStatus == UNAuthorizationStatusNotDetermined) {
            if (error) {
              auto message = String(
                error.localizedDescription.UTF8String != nullptr
                  ? error.localizedDescription.UTF8String
                  : "An unknown error occurred"
              );

              auto err = JSON::Object::Entries {
                { "message", message }
              };

              return reply(Result::Err { message, err });
            }

            auto data = JSON::Object::Entries {
              {"state", granted ? "granted" : "denied" }
            };

            return reply(Result::Data { message, data });
          }

          auto data = JSON::Object::Entries {{"state", "granted"}};
          return reply(Result::Data { message, data });
        }];
      }];
    #endif
    }
  });

  /**
   * Simply returns `pong`.
   */
  router->map("ping", [=](auto message, auto router, auto reply) {
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
    const auto err = validateMessageParameters(message, {"value"});
    const auto frameType = message.get("runtime-frame-type");
    const auto frameSource = message.get("runtime-frame-source");
    auto userConfig = router->bridge->userConfig;

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    if (frameType == "top-level" && frameSource != "serviceworker") {
      if (message.value == "load") {
        const auto href = message.get("location.href");
        if (href.size() > 0) {
          router->location.href = href;
          router->location.workers.clear();
          auto tmp = href;
          tmp = replace(tmp, "socket://", "");
          tmp = replace(tmp, "https://", "");
          tmp = replace(tmp, userConfig["meta_bundle_identifier"], "");
          const auto parsed = URL::Components::parse(tmp);
          router->location.pathname = parsed.pathname;
          router->location.query = parsed.query;
        }
      }

      if (router->bridge == router->core->serviceWorker.bridge) {
        if (router->bridge->userConfig["webview_service_worker_mode"] == "hybrid" || platform.ios || platform.android) {
          if (router->location.href.size() > 0 && message.value == "beforeruntimeinit") {
            router->core->serviceWorker.reset();
            router->core->serviceWorker.isReady = false;
          } else if (message.value == "runtimeinit") {
            router->core->serviceWorker.isReady = true;
          }
        }
      }
    }

    if (message.value == "load" && frameType == "worker") {
      const auto workerLocation = message.get("runtime-worker-location");
      const auto href = message.get("location.href");
      if (href.size() > 0 && workerLocation.size() > 0) {
        router->location.workers[href] = workerLocation;
      }
    }

    router->core->platform.event(
      message.seq,
      message.value,
      message.get("data"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Requests a notification with `title` and `body` to be shown.
   * @param title
   * @param body
   */
  router->map("platform.notify", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"body", "title"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    router->core->platform.notify(
      message.seq,
      message.get("title"),
      message.get("body"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Reveal a file in the native operating system file system explorer.
   * @param value
   */
  router->map("platform.revealFile", [=](auto message, auto router, auto reply) mutable {
    auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    router->core->platform.revealFile(
      message.seq,
      message.get("value"),
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Requests a URL to be opened externally.
   * @param value
   */
  router->map("platform.openExternal", [=](auto message, auto router, auto reply) mutable {
    const auto applicationProtocol = router->bridge->userConfig["meta_application_protocol"];
    auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    if (applicationProtocol.size() > 0 && message.value.starts_with(applicationProtocol + ":")) {
      SSC::JSON::Object json = SSC::JSON::Object::Entries {
        { "url", message.value }
      };

      router->bridge->router.emit("applicationurl", json.str());
      reply(Result {
        message.seq,
        message,
        SSC::JSON::Object::Entries {
          {"data", json}
        }
      });
      return;
    }

    router->core->platform.openExternal(
      message.seq,
      message.value,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Return Socket Runtime primordials.
   */
  router->map("platform.primordials", [=](auto message, auto router, auto reply) {
    std::regex platform_pattern("^mac$", std::regex_constants::icase);
    auto platformRes = std::regex_replace(platform.os, platform_pattern, "darwin");
    auto arch = std::regex_replace(platform.arch, std::regex("x86_64"), "x64");
    arch = std::regex_replace(arch, std::regex("x86"), "ia32");
    arch = std::regex_replace(arch, std::regex("arm(?!64).*"), "arm");
    auto json = JSON::Object::Entries {
      {"source", "platform.primordials"},
      {"data", JSON::Object::Entries {
        {"arch", arch},
        {"cwd", getcwd()},
        {"platform", platformRes},
        {"version", JSON::Object::Entries {
          {"full", SSC::VERSION_FULL_STRING},
          {"short", SSC::VERSION_STRING},
          {"hash", SSC::VERSION_HASH_STRING}}
        },
        {"host-operating-system",
        #if defined(__APPLE__)
          #if TARGET_IPHONE_SIMULATOR
             "iphonesimulator"
          #elif TARGET_OS_IPHONE
            "iphoneos"
          #else
             "macosx"
          #endif
        #elif defined(__ANDROID__)
             (router->bridge->isAndroidEmulator ? "android-emulator" : "android")
        #elif defined(__WIN32)
             "win32"
        #elif defined(__linux__)
             "linux"
        #elif defined(__unix__) || defined(__unix)
             "unix"
        #else
             "unknown"
        #endif
        }
      }}
    };
    reply(Result { message.seq, message, json });
  });

  /**
   * Returns pending post data typically returned in the response of an
   * `ipc://post` IPC call intercepted by an XHR request.
   * @param id The id of the post data.
   */
  router->map("post", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    if (!router->core->hasPost(id)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Post not found for given 'id'"}
      }});
    }

    auto result = Result { message.seq, message };
    result.post = router->core->getPost(id);
    reply(result);
    router->core->removePost(id);
  });

  /**
   * Registers a custom protocol handler scheme. Custom protocols MUST be handled in service workers.
   * @param scheme
   * @param data
   */
  router->map("protocol.register", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"scheme"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    const auto scheme = message.get("scheme");
    const auto data = message.get("data");

    if (data.size() > 0 && router->core->protocolHandlers.hasHandler(scheme)) {
      router->core->protocolHandlers.setHandlerData(scheme, { data });
    } else {
      router->core->protocolHandlers.registerHandler(scheme, { data });
    }

    reply(Result { message.seq, message });
  });

  /**
   * Unregister a custom protocol handler scheme.
   * @param scheme
   */
  router->map("protocol.unregister", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"scheme"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    const auto scheme = message.get("scheme");

    if (!router->core->protocolHandlers.hasHandler(scheme)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Protocol handler scheme is not registered."}
      }});
    }

    router->core->protocolHandlers.unregisterHandler(scheme);

    reply(Result { message.seq, message });
  });

  /**
   * Gets protocol handler data
   * @param scheme
   */
  router->map("protocol.getData", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"scheme"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    const auto scheme = message.get("scheme");

    if (!router->core->protocolHandlers.hasHandler(scheme)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Protocol handler scheme is not registered."}
      }});
    }

    const auto data = router->core->protocolHandlers.getHandlerData(scheme);

    reply(Result { message.seq, message, JSON::Raw(data.json) });
  });

  /**
   * Sets protocol handler data
   * @param scheme
   * @param data
   */
  router->map("protocol.setData", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"scheme", "data"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    const auto scheme = message.get("scheme");
    const auto data = message.get("data");

    if (!router->core->protocolHandlers.hasHandler(scheme)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Protocol handler scheme is not registered."}
      }});
    }

    router->core->protocolHandlers.setHandlerData(scheme, { data });

    reply(Result { message.seq, message });
  });

  /**
   * Prints incoming message value to stdout.
   * @param value
   */
  router->map("stdout", [=](auto message, auto router, auto reply) {
    if (message.value.size() > 0) {
    #if defined(__APPLE__)
      os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_INFO, "%{public}s", message.value.c_str());
    #endif
      IO::write(message.value, false);
    } else if (message.buffer.bytes != nullptr && message.buffer.size > 0) {
      IO::write(String(*message.buffer.bytes, message.buffer.size), false);
    }

    reply(Result { message.seq, message });
  });

  /**
   * Prints incoming message value to stderr.
   * @param value
   */
  router->map("stderr", [=](auto message, auto router, auto reply) {
    if (message.get("debug") == "true") {
      if (message.value.size() > 0) {
        debug("%s", message.value.c_str());
      }
    } else if (message.value.size() > 0) {
    #if defined(__APPLE__)
      os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_ERROR, "%{public}s", message.value.c_str());
    #endif
      IO::write(message.value, true);
    } else if (message.buffer.bytes != nullptr && message.buffer.size > 0) {
      IO::write(String(*message.buffer.bytes, message.buffer.size), true);
    }

    reply(Result { message.seq, message });
  });

  /**
   * Registers a service worker script for a given scope.
   * @param scriptURL
   * @param scope
   */
  router->map("serviceWorker.register", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"scriptURL", "scope"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    const auto options = ServiceWorkerContainer::RegistrationOptions {
      .type = ServiceWorkerContainer::RegistrationOptions::Type::Module,
      .scope = message.get("scope"),
      .scriptURL = message.get("scriptURL")
    };

    const auto registration = router->core->serviceWorker.registerServiceWorker(options);
    auto json = JSON::Object {
      JSON::Object::Entries {
        {"registration", registration.json()}
      }
    };

    reply(Result::Data { message, json });
  });

  /**
   * Resets the service worker container state.
   */
  router->map("serviceWorker.reset", [=](auto message, auto router, auto reply) {
    router->core->serviceWorker.reset();
    reply(Result::Data { message, JSON::Object {}});
  });

  /**
   * Unregisters a service worker for given scoep.
   * @param scope
   */
  router->map("serviceWorker.unregister", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"scope"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    const auto scope = message.get("scope");
    router->core->serviceWorker.unregisterServiceWorker(scope);

    return reply(Result::Data { message, JSON::Object {} });
  });

  /**
   * Gets registration information for a service worker scope.
   * @param scope
   */
  router->map("serviceWorker.getRegistration", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"scope"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    const auto scope = message.get("scope");

    for (const auto& entry : router->core->serviceWorker.registrations) {
      const auto& registration = entry.second;
      if (scope.starts_with(registration.options.scope)) {
        auto json = JSON::Object {
          JSON::Object::Entries {
            {"registration", registration.json()},
            {"client", JSON::Object::Entries {
              {"id", std::to_string(router->bridge->id)}
            }}
          }
        };

        return reply(Result::Data { message, json });
      }
    }

    return reply(Result::Data { message, JSON::Object {} });
  });

  /**
   * Gets all service worker scope registrations.
   */
  router->map("serviceWorker.getRegistrations", [=](auto message, auto router, auto reply) {
    auto json = JSON::Array::Entries {};
    for (const auto& entry : router->core->serviceWorker.registrations) {
      const auto& registration = entry.second;
      json.push_back(registration.json());
    }
    return reply(Result::Data { message, json });
  });

  /**
   * Informs container that a service worker will skip waiting.
   * @param id
   */
  router->map("serviceWorker.skipWaiting", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->serviceWorker.skipWaiting(id);

    reply(Result::Data { message, JSON::Object {}});
  });

  /**
   * Updates service worker controller state.
   * @param id
   * @param state
   */
  router->map("serviceWorker.updateState", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "state"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    const auto workerURL = message.get("workerURL");
    const auto scriptURL = message.get("scriptURL");

    if (workerURL.size() > 0 && scriptURL.size() > 0) {
      router->location.workers[workerURL] = scriptURL;
    }

    router->core->serviceWorker.updateState(id, message.get("state"));
    reply(Result::Data { message, JSON::Object {}});
  });

  /**
   * Sets storage for a service worker.
   * @param id
   * @param key
   * @param value
   */
  router->map("serviceWorker.storage.set", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "key", "value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    for (auto& entry : router->core->serviceWorker.registrations) {
      if (entry.second.id == id) {
        auto& registration = entry.second;
        registration.storage.set(message.get("key"), message.get("value"));
        return reply(Result::Data { message, JSON::Object {}});
      }
    }

    return reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"}
      }
    });
  });

  /**
   * Gets a storage value for a service worker.
   * @param id
   * @param key
   */
  router->map("serviceWorker.storage.get", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "key"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    for (auto& entry : router->core->serviceWorker.registrations) {
      if (entry.second.id == id) {
        auto& registration = entry.second;
        return reply(Result::Data {
          message,
          JSON::Object::Entries {
            {"value", registration.storage.get(message.get("key"))}
          }
        });
      }
    }

    return reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"}
      }
    });
  });

  /**
   * Remoes a storage value for a service worker.
   * @param id
   * @param key
   */
  router->map("serviceWorker.storage.remove", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "key"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    for (auto& entry : router->core->serviceWorker.registrations) {
      if (entry.second.id == id) {
        auto& registration = entry.second;
        registration.storage.remove(message.get("key"));
        return reply(Result::Data {message, JSON::Object {}});
      }
    }

    return reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"}
      }
    });
  });

  /**
   * Clears all storage values for a service worker.
   * @param id
   */
  router->map("serviceWorker.storage.clear", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    for (auto& entry : router->core->serviceWorker.registrations) {
      if (entry.second.id == id) {
        auto& registration = entry.second;
        registration.storage.clear();
        return reply(Result::Data { message, JSON::Object {} });
      }
    }

    return reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"}
      }
    });
  });

  /**
   * Gets all storage values for a service worker.
   * @param id
   */
  router->map("serviceWorker.storage", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    for (auto& entry : router->core->serviceWorker.registrations) {
      if (entry.second.id == id) {
        auto& registration = entry.second;
        return reply(Result::Data { message, registration.storage.json() });
      }
    }

    return reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"}
      }
    });
  });

  router->map("timers.setTimeout", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"timeout"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint32_t timeout;
    REQUIRE_AND_GET_MESSAGE_VALUE(timeout, "timeout", std::stoul);
    const auto wait = message.get("wait") == "true";
    const Core::Timers::ID id = router->core->timers.setTimeout(timeout, [=]() {
      if (wait) {
        reply(Result::Data { message, JSON::Object::Entries {{"id", std::to_string(id) }}});
      }
    });

    if (!wait) {
      reply(Result::Data { message, JSON::Object::Entries {{"id", std::to_string(id) }}});
    }
  });

  router->map("timers.clearTimeout", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    router->core->timers.clearTimeout(id);

    reply(Result::Data { message, JSON::Object::Entries {{"id", std::to_string(id) }}});
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
    auto err = validateMessageParameters(message, {"id", "port"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(options.port, "port", std::stoi);

    options.reuseAddr = message.get("reuseAddr") == "true";
    options.address = message.get("address", "0.0.0.0");

    router->core->udp.bind(
      message.seq,
      id,
      options,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Close socket handle and underlying UDP socket.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.close", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->udp.close(message.seq, id, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Connects an UDP socket to a specified port, and optionally a host
   * address (default: 0.0.0.0).
   * @param id Handle ID of underlying socket
   * @param port Port to connect the UDP socket to
   * @param address The address to connect the UDP socket to (default: 0.0.0.0)
   */
  router->map("udp.connect", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "port"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    Core::UDP::ConnectOptions options;
    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(options.port, "port", std::stoi);

    options.address = message.get("address", "0.0.0.0");

    router->core->udp.connect(
      message.seq,
      id,
      options,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Disconnects a connected socket handle and underlying UDP socket.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.disconnect", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->udp.disconnect(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Returns connected peer socket address information.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.getPeerName", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->udp.getPeerName(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Returns local socket address information.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.getSockName", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->udp.getSockName(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Returns socket state information.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.getState", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->udp.getState(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Initializes socket handle to start receiving data from the underlying
   * socket and route through the IPC bridge to the WebView.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.readStart", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->udp.readStart(
      message.seq,
      id,
      [message, reply](auto seq, auto json, auto post) {
        reply(Result { seq, message, json, post });
      }
    );
  });

  /**
   * Stops socket handle from receiving data from the underlying
   * socket and routing through the IPC bridge to the WebView.
   * @param id Handle ID of underlying socket
   */
  router->map("udp.readStop", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    router->core->udp.readStop(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
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
    auto err = validateMessageParameters(message, {"id", "port"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    Core::UDP::SendOptions options;
    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(options.port, "port", std::stoi);

    options.size = message.buffer.size;
    options.bytes = *message.buffer.bytes;
    options.address = message.get("address", "0.0.0.0");
    options.ephemeral = message.get("ephemeral") == "true";

    router->core->udp.send(
      message.seq,
      id,
      options,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  router->map("window.showFileSystemPicker", [=](auto message, auto router, auto reply) {
    const auto allowMultiple = message.get("allowMultiple") == "true";
    const auto allowFiles = message.get("allowFiles") == "true";
    const auto allowDirs = message.get("allowDirs") == "true";
    const auto isSave = message.get("type") == "save";

    const auto contentTypeSpecs = message.get("contentTypeSpecs");
    const auto defaultName = message.get("defaultName");
    const auto defaultPath = message.get("defaultPath");
    const auto title = message.get("title", isSave ? "Save" : "Open");

    Dialog dialog;
    auto options = Dialog::FileSystemPickerOptions {
      .directories = allowDirs,
      .multiple = allowMultiple,
      .files = allowFiles,
      .contentTypes = contentTypeSpecs,
      .defaultName = defaultName,
      .defaultPath = defaultPath,
      .title = title
    };

    if (isSave) {
      const auto result = dialog.showSaveFilePicker(options);

      if (result.size() == 0) {
        auto err = JSON::Object::Entries {{"type", "AbortError"}};
        reply(Result::Err { message, err });
      } else {
        auto data = JSON::Object::Entries {
          {"paths", JSON::Array::Entries{result}}
        };
        reply(Result::Data { message, data });
      }
    } else {
      JSON::Array paths;
      const auto results = (
        allowFiles && !allowDirs
          ? dialog.showOpenFilePicker(options)
          : dialog.showDirectoryPicker(options)
      );

      for (const auto& result : results) {
        paths.push(result);
      }

      auto data = JSON::Object::Entries {
        {"paths", paths}
      };

      reply(Result::Data { message, data });
    }
  });
}

namespace SSC::IPC {
  void Router::init () {
    mapIPCRoutes(this);
  }
}