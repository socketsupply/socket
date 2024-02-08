#include <regex>
#include <unordered_map>

#if defined(__APPLE__)
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

#include "../extension/extension.hh"
#include "../window/window.hh"
#include "ipc.hh"

#define SOCKET_MODULE_CONTENT_TYPE "text/javascript"
#define IPC_BINARY_CONTENT_TYPE "application/octet-stream"
#define IPC_JSON_CONTENT_TYPE "text/json"

extern const SSC::Map SSC::getUserConfig ();
extern bool SSC::isDebugEnabled ();

using namespace SSC;
using namespace SSC::IPC;

#if defined(__APPLE__)
static std::map<String, Router*> notificationRouterMap;
static Mutex notificationRouterMapMutex;

static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create(
  "socket.runtime.ipc.bridge.queue",
  qos
);
#endif

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

static struct { Mutex mutex; String value = ""; } cwdstate;

static void setcwd (String cwd) {
  Lock lock(cwdstate.mutex);
  cwdstate.value = cwd;
}

static String getcwd () {
  Lock lock(cwdstate.mutex);
  String cwd = cwdstate.value;
#if defined(__linux__) && !defined(__ANDROID__)
  try {
    auto canonical = fs::canonical("/proc/self/exe");
    cwd = fs::path(canonical).parent_path().string();
  } catch (...) {}
#elif defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
  NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
  cwd = String([[resourcePath stringByAppendingPathComponent: @"ui"] UTF8String]);
#elif defined(__APPLE__)
  auto fileManager = [NSFileManager defaultManager];
  auto currentDirectory = [fileManager currentDirectoryPath];
  cwd = String([currentDirectory UTF8String]);
#elif defined(_WIN32)
  wchar_t filename[MAX_PATH];
  GetModuleFileNameW(NULL, filename, MAX_PATH);
  auto path = fs::path { filename }.remove_filename();
  cwd = path.string();
  size_t last_pos = 0;
  while ((last_pos = cwd.find('\\', last_pos)) != std::string::npos) {
    cwd.replace(last_pos, 1, "\\\\");
    last_pos += 2;
  }
#endif

#ifndef _WIN32
  std::replace(cwd.begin(), cwd.end(), '\\', '/');
#endif

  cwdstate.value = cwd;
  return cwd;
}

#define RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)                     \
  [message, reply](auto seq, auto json, auto post) {                           \
    reply(Result { seq, message, json, post });                                \
  }

#define REQUIRE_AND_GET_MESSAGE_VALUE(var, name, parse, ...)                   \
  try {                                                                        \
    var = parse(message.get(name, ##__VA_ARGS__));                             \
  } catch (...) {                                                              \
    return reply(Result::Err { message, JSON::Object::Entries {                \
      {"message", "Invalid '" name "' given in parameters"}                    \
    }});                                                                       \
  }

#define CLEANUP_AFTER_INVOKE_CALLBACK(router, message, result) {               \
  if (!router->hasMappedBuffer(message.index, message.seq)) {                  \
    if (message.buffer.bytes != nullptr) {                                     \
      delete [] message.buffer.bytes;                                          \
      message.buffer.bytes = nullptr;                                          \
    }                                                                          \
  }                                                                            \
                                                                               \
  if (!router->core->hasPostBody(result.post.body)) {                          \
    if (result.post.body != nullptr) {                                         \
      delete [] result.post.body;                                              \
    }                                                                          \
  }                                                                            \
}

static void initRouterTable (Router *router) {
  static auto userConfig = SSC::getUserConfig();
#if defined(__APPLE__)
  static auto bundleIdentifier = userConfig["meta_bundle_identifier"];
  static auto SSC_OS_LOG_BUNDLE = os_log_create(bundleIdentifier.c_str(),
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
  router->map("bluetooth.start", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"serviceId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (userConfig["permissions_allow_bluetooth"] == "false") {
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
  router->map("bluetooth.subscribe", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {
      "characteristicId",
      "serviceId"
    });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (userConfig["permissions_allow_bluetooth"] == "false") {
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
  router->map("bluetooth.publish", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {
      "characteristicId",
      "serviceId"
    });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (userConfig["permissions_allow_bluetooth"] == "false") {
      auto err =JSON::Object::Entries {
        {"message", "Bluetooth is not allowed"}
      };

      return reply(Result::Err { message, err });
    }

    auto bytes = message.buffer.bytes;
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
  router->map("buffer.map", false, [](auto message, auto router, auto reply) {
    router->setMappedBuffer(message.index, message.seq, message.buffer);
    reply(Result { message.seq, message });
  });

  /**
   * Look up an IP address by `hostname`.
   * @param hostname Host name to lookup
   * @param family IP address family to resolve [default = 0 (AF_UNSPEC)]
   * @see getaddrinfo(3)
   */
  router->map("dns.lookup", [](auto message, auto router, auto reply) {
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

  /**
   * LLM
   */
  router->map("llm.createModel", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"path", "gpuLayers", "vocabOnly", "useMmap", "useMlock"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    Core::LLM::ModelOptions options;
    options.path = message.get("path");
    options.gpuLayers = message.get("gpuLayers").compare("true");
    options.vocabOnly = message.get("vocabOnly").compare("true");
    options.useMmap = message.get("useMmap").compare("true");
    options.useMlock = message.get("useMlock").compare("true");

    router->core->llm.createModel(message.seq, options, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.destroyModel", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"modelId"});
    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "modelId", std::stoi);
    router->core->llm.destroyModel(message.seq, modelId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.createContext", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"modelId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "modelId", std::stoi);

    Core::LLM::ContextOptions options;
    options.modelId = modelId;

    router->core->llm.createContext(message.seq, options, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.destoryContext", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"contextId"});
    uint64_t contextId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(contextId, "contextId", std::stoi);
    router->core->llm.destroyContext(message.seq, contextId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.createEvaluator", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"grammarId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t grammarId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(grammarId, "grammarId", std::stoi);

    router->core->llm.createEvaluator(message.seq, grammarId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.destroyEvaluator", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"evaluatorId"});
    uint64_t evaluatorId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(evaluatorId, "evaluatorId", std::stoi);
    router->core->llm.destroyEvaluator(message.seq, evaluatorId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.parseGrammar", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"text"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    Core::LLM::GrammarOptions options;
    options.text = message.get("text");

    router->core->llm.parseGrammar(message.seq, options, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.destoryGrammar", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"grammarId"});
    uint64_t grammarId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(grammarId, "grammarId", std::stoi);
    router->core->llm.destroyGrammar(message.seq, grammarId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.decode", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"contextId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto length = message.buffer.size;
    const char* bytes = message.buffer.bytes;

    std::vector<uint32_t> tokens;
    for (int i = 0; i < length; i += 4) {
      auto token = bytes[i] | (bytes[i+1] << 8) | (bytes[i+2] << 16) | (bytes[i+3] << 24);
      tokens.push_back(token);
    }

    uint64_t contextId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(contextId, "contextId", std::stoi);

    router->core->llm.decode(message.seq, contextId, tokens, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.encode", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"contextId", "text"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t contextId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(contextId, "contextId", std::stoi);

    auto text = message.get("text");

    router->core->llm.encode(message.seq, contextId, text, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.tokenBos", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"modelId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "modelId", std::stoi);

    router->core->llm.tokenBos(message.seq, modelId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.tokenEos", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"modelId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "modelId", std::stoi);

    router->core->llm.tokenEos(message.seq, modelId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.tokenNl", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"modelId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "modelId", std::stoi);

    router->core->llm.tokenNl(message.seq, modelId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.getTokenString", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"modelId", "token"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "modelId", std::stoi);

    uint32_t token = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "modelId", std::stoi);

    router->core->llm.getTokenString(message.seq, modelId, token, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("llm.eval", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"modelId", "contextId"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "modelId", std::stoi);

    uint64_t contextId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(contextId, "contextId", std::stoi);

    Core::LLM::WorkerOptions options;
    options.modelId = modelId;
    options.contextId = contextId;

    router->core->llm.eval(message.seq, options, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Extensions
   */
  router->map("extension.stats", [](auto message, auto router, auto reply) {
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
  router->map("extension.type", [](auto message, auto router, auto reply) {
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
  router->map("extension.load", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");

    if (!Extension::load(name)) {
      #if defined(_WIN32)
      auto error = FormatError(GetLastError(), "bridge");
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
  router->map("extension.unload", [](auto message, auto router, auto reply) {
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
  router->map("fs.access", [](auto message, auto router, auto reply) {
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
  router->map("fs.constants", [](auto message, auto router, auto reply) {
    router->core->fs.constants(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Changes `mode` of file at `path`.
   * @param path
   * @param mode
   * @see chmod(2)
   */
  router->map("fs.chmod", [](auto message, auto router, auto reply) {
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
  router->map("fs.chown", [](auto message, auto router, auto reply) {
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
  router->map("fs.close", [](auto message, auto router, auto reply) {
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
  router->map("fs.closedir", [](auto message, auto router, auto reply) {
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
  router->map("fs.closeOpenDescriptor", [](auto message, auto router, auto reply) {
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
  router->map("fs.closeOpenDescriptors", [](auto message, auto router, auto reply) {
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
  router->map("fs.copyFile", [](auto message, auto router, auto reply) {
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
  router->map("fs.fstat", [](auto message, auto router, auto reply) {
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
  router->map("fs.fsync", [](auto message, auto router, auto reply) {
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
  router->map("fs.ftruncate", [](auto message, auto router, auto reply) {
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
  router->map("fs.getOpenDescriptors", [](auto message, auto router, auto reply) {
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
  router->map("fs.lstat", [](auto message, auto router, auto reply) {
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
  router->map("fs.mkdir", [](auto message, auto router, auto reply) {
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
  router->map("fs.open", [](auto message, auto router, auto reply) {
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
  router->map("fs.opendir", [](auto message, auto router, auto reply) {
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
  router->map("fs.read", [](auto message, auto router, auto reply) {
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
  router->map("fs.readdir", [](auto message, auto router, auto reply) {
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
  router->map("fs.retainOpenDescriptor", [](auto message, auto router, auto reply) {
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
  router->map("fs.rename", [](auto message, auto router, auto reply) {
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
  router->map("fs.rmdir", [](auto message, auto router, auto reply) {
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
  router->map("fs.stat", [](auto message, auto router, auto reply) {
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
  router->map("fs.stopWatch", [](auto message, auto router, auto reply) {
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
  router->map("fs.unlink", [](auto message, auto router, auto reply) {
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
  router->map("fs.watch", [](auto message, auto router, auto reply) {
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
  router->map("fs.write", [](auto message, auto router, auto reply) {
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
      message.buffer.bytes,
      message.buffer.size,
      offset,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

#if defined(__APPLE__)
  router->map("geolocation.getCurrentPosition", [](auto message, auto router, auto reply) {
    if (!router->locationObserver) {
      auto err = JSON::Object::Entries {{ "message", "Location observer is not initialized",  }};
      err["type"] = "GeolocationPositionError";
      return reply(Result::Err { message, err });
    }

    auto performedActivation = [router->locationObserver getCurrentPositionWithCompletion: ^(NSError* error, CLLocation* location) {
      if (error != nullptr) {
        auto message = String(
           error.localizedDescription.UTF8String != nullptr
             ? error.localizedDescription.UTF8String
             : "An unknown error occurred"
         );
        auto err = JSON::Object::Entries {{ "message", message }};
        err["type"] = "GeolocationPositionError";
        return reply(Result::Err { message, err });
      }

      auto heading = router->locationObserver.locationManager.heading;
      auto json = JSON::Object::Entries {
        {"coords", JSON::Object::Entries {
          {"latitude", location.coordinate.latitude},
          {"longitude", location.coordinate.longitude},
          {"altitude", location.altitude},
          {"accuracy", location.horizontalAccuracy},
          {"altitudeAccuracy", location.verticalAccuracy},
          {"floorLevel", location.floor.level},
          {"heading", heading.trueHeading},
          {"speed", location.speed}
        }}
      };

      reply(Result { message.seq, message, json });
    }];

    if (!performedActivation) {
      auto err = JSON::Object::Entries {{ "message", "Location observer could not be activated" }};
      err["type"] = "GeolocationPositionError";
      return reply(Result::Err { message, err });
    }
  });

  router->map("geolocation.watchPosition", [](auto message, auto router, auto reply) {
    if (!router->locationObserver) {
      auto err = JSON::Object::Entries {{ "message", "Location observer is not initialized",  }};
      err["type"] = "GeolocationPositionError";
      return reply(Result::Err { message, err });
    }

    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int id = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoi);

    const int identifier = [router->locationObserver watchPositionForIdentifier: id  completion: ^(NSError* error, CLLocation* location) {
      if (error != nullptr) {
        auto message = String(
           error.localizedDescription.UTF8String != nullptr
             ? error.localizedDescription.UTF8String
             : "An unknown error occurred"
        );
        auto err = JSON::Object::Entries {{ "message", message }};
        err["type"] = "GeolocationPositionError";
        return reply(Result::Err { message, err });
      }

      auto heading = router->locationObserver.locationManager.heading;
      auto json = JSON::Object::Entries {
        {"watch", JSON::Object::Entries {
          {"identifier", identifier},
        }},
        {"coords", JSON::Object::Entries {
          {"latitude", location.coordinate.latitude},
          {"longitude", location.coordinate.longitude},
          {"altitude", location.altitude},
          {"accuracy", location.horizontalAccuracy},
          {"altitudeAccuracy", location.verticalAccuracy},
          {"floorLevel", location.floor.level},
          {"heading", heading.trueHeading},
          {"speed", location.speed}
        }}
      };

      reply(Result { "-1", message, json });
    }];

    if (identifier == -1) {
      auto err = JSON::Object::Entries {{ "message", "Location observer could not be activated" }};
      err["type"] = "GeolocationPositionError";
      return reply(Result::Err { message, err });
    }

    auto json = JSON::Object::Entries {
      {"watch", JSON::Object::Entries {
        {"identifier", identifier}
      }}
    };

    reply(Result { message.seq, message, json });
  });

  router->map("geolocation.clearWatch", [](auto message, auto router, auto reply) {
    if (!router->locationObserver) {
      auto err = JSON::Object::Entries {{ "message", "Location observer is not initialized",  }};
      err["type"] = "GeolocationPositionError";
      return reply(Result::Err { message, err });
    }

    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int id = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoi);

    [router->locationObserver clearWatch: id];

    reply(Result { message.seq, message, JSON::Object{} });
  });
#endif

  /**
   * A private API for artifically setting the current cached CWD value.
   * This is only useful on platforms that need to set this value from an
   * external source, like Android or ChromeOS.
   */
  router->map("internal.setcwd", [](auto message, auto router, auto reply) {
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
  router->map("log", [](auto message, auto router, auto reply) {
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

#if defined(__APPLE__)
  router->map("notification.show", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {
      "id",
      "title"
    });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
    auto attachments = [NSMutableArray new];
    auto userInfo = [NSMutableDictionary new];
    auto content = [UNMutableNotificationContent new];
    auto __block id = message.get("id");

    if (message.has("tag")) {
      userInfo[@"tag"]  = @(message.get("tag").c_str());
      content.threadIdentifier = @(message.get("tag").c_str());
    }

    if (message.has("lang")) {
      userInfo[@"lang"]  = @(message.get("lang").c_str());
    }

    if (!message.has("silent") && message.get("silent") == "false") {
      content.sound = [UNNotificationSound defaultSound];
    }

    if (message.has("icon")) {
      NSError* error = nullptr;
      auto url = [NSURL URLWithString: @(message.get("icon").c_str())];

      if (message.get("icon").starts_with("socket://")) {
        url = [NSURL URLWithString: [NSBundle.mainBundle.resourcePath
            stringByAppendingPathComponent: [NSString
            #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
              stringWithFormat: @"/ui/%s", url.path.UTF8String
            #else
              stringWithFormat: @"/%s", url.path.UTF8String
            #endif
            ]
        ]];

        url = [NSURL fileURLWithPath: url.path];
      }

      auto types = [UTType
            typesWithTag: url.pathExtension
                tagClass: UTTagClassFilenameExtension
        conformingToType: nullptr
      ];

      auto options = [NSMutableDictionary new];

      if (types.count > 0) {
        options[UNNotificationAttachmentOptionsTypeHintKey] = types.firstObject.preferredMIMEType;
      };

      auto attachment = [UNNotificationAttachment
        attachmentWithIdentifier: @("")
                             URL: url
                         options: options
                           error: &error
      ];

      if (error != nullptr) {
        auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
        );

        auto err = JSON::Object::Entries { { "message", message } };
        return reply(Result::Err { message, err });
      }

      [attachments addObject: attachment];
    } else {
    // using an asset from the resources directory will require a code signed application
    #if WAS_CODESIGNED
      NSError* error = nullptr;
      auto url = [NSURL URLWithString: [NSBundle.mainBundle.resourcePath
        stringByAppendingPathComponent: [NSString
        #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
          stringWithFormat: @"/ui/icon.png"
        #else
          stringWithFormat: @"/icon.png"
        #endif
        ]
      ]];

      url = [NSURL fileURLWithPath: url.path];

      auto types = [UTType
            typesWithTag: url.pathExtension
                tagClass: UTTagClassFilenameExtension
        conformingToType: nullptr
      ];

      auto options = [NSMutableDictionary new];

      auto attachment = [UNNotificationAttachment
        attachmentWithIdentifier: @("")
                             URL: url
                         options: options
                           error: &error
      ];

      if (error != nullptr) {
        auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
        );

        auto err = JSON::Object::Entries { { "message", message } };

        return reply(Result::Err { message, err });
      }

      [attachments addObject: attachment];
    #endif
    }

    if (message.has("image")) {
      NSError* error = nullptr;
      auto url = [NSURL URLWithString: @(message.get("image").c_str())];

      if (message.get("image").starts_with("socket://")) {
        url = [NSURL URLWithString: [NSBundle.mainBundle.resourcePath
            stringByAppendingPathComponent: [NSString
            #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
              stringWithFormat: @"/ui/%s", url.path.UTF8String
            #else
              stringWithFormat: @"/%s", url.path.UTF8String
            #endif
            ]
        ]];

        url = [NSURL fileURLWithPath: url.path];
      }

      auto types = [UTType
            typesWithTag: url.pathExtension
                tagClass: UTTagClassFilenameExtension
        conformingToType: nullptr
      ];

      auto options = [NSMutableDictionary new];

      if (types.count > 0) {
        options[UNNotificationAttachmentOptionsTypeHintKey] = types.firstObject.preferredMIMEType;
      };

      auto attachment = [UNNotificationAttachment
        attachmentWithIdentifier: @("")
                             URL: url
                         options: options
                           error: &error
      ];

      if (error != nullptr) {
        auto message = String(
           error.localizedDescription.UTF8String != nullptr
             ? error.localizedDescription.UTF8String
             : "An unknown error occurred"
        );
        auto err = JSON::Object::Entries {{ "message", message }};

        return reply(Result::Err { message, err });
      }

      [attachments addObject: attachment];
    }

    content.attachments = attachments;
    content.userInfo = userInfo;
    content.title = @(message.get("title").c_str());
    content.body = @(message.get("body", "").c_str());

    auto request = [UNNotificationRequest
      requestWithIdentifier: @(id.c_str())
                    content: content
                    trigger: nil
    ];

    {
      Lock lock(notificationRouterMapMutex);
      notificationRouterMap.insert_or_assign(id, router);
    }

    [notificationCenter addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
      if (error != nullptr) {
        auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
        );

        auto err = JSON::Object::Entries {
          { "message", message }
        };

        reply(Result::Err { message, err });
        Lock lock(notificationRouterMapMutex);
        notificationRouterMap.erase(id);
        return;
      }

      reply(Result { message.seq, message, JSON::Object::Entries {
        {"id", request.identifier.UTF8String}
      }});
    }];
  });

  router->map("notification.close", [](auto message, auto router, auto reply) {
    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
    auto err = validateMessageParameters(message, { "id" });

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto id = message.get("id");
    auto identifiers = @[@(id.c_str())];

    [notificationCenter removePendingNotificationRequestsWithIdentifiers: identifiers];
    [notificationCenter removeDeliveredNotificationsWithIdentifiers: identifiers];

    reply(Result { message.seq, message, JSON::Object::Entries {
      {"id", id}
    }});

    Lock lock(notificationRouterMapMutex);
    if (notificationRouterMap.contains(id)) {
      auto notificationRouter = notificationRouterMap.at(id);
      JSON::Object json = JSON::Object::Entries {
        {"id", id},
        {"action",  "dismiss"}
      };

      notificationRouter->emit("notificationresponse", json.str());
      notificationRouterMap.erase(id);
    }
  });

  router->map("notification.list", [](auto message, auto router, auto reply) {
    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
    [notificationCenter getDeliveredNotificationsWithCompletionHandler: ^(NSArray<UNNotification*> *notifications) {
      JSON::Array::Entries entries;

      Lock lock(notificationRouterMapMutex);
      for (UNNotification* notification in notifications) {
        auto id = String(notification.request.identifier.UTF8String);

        if (
          !notificationRouterMap.contains(id) ||
          notificationRouterMap.at(id) != router
        ) {
          continue;
        }

        entries.push_back(JSON::Object::Entries {
          {"id", id}
        });
      }

      reply(Result { message.seq, message, entries });
    }];
  });
#endif

  /**
   * Read or modify the `SEND_BUFFER` or `RECV_BUFFER` for a peer socket.
   * @param id Handle ID for the buffer to read/modify
   * @param size If given, the size to set in the buffer [default = 0]
   * @param buffer The buffer to read/modify (SEND_BUFFER, RECV_BUFFER) [default = 0 (SEND_BUFFER)]
   */
  router->map("os.bufferSize", [](auto message, auto router, auto reply) {
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
   * Returns a mapping of network interfaces.
   */
  router->map("os.networkInterfaces", [](auto message, auto router, auto reply) {
    router->core->os.networkInterfaces(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Returns an array of CPUs available to the process.
   */
  router->map("os.cpus", [](auto message, auto router, auto reply) {
    router->core->os.cpus(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.rusage", [](auto message, auto router, auto reply) {
    router->core->os.rusage(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.uptime", [](auto message, auto router, auto reply) {
    router->core->os.uptime(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.uname", [](auto message, auto router, auto reply) {
    router->core->os.uname(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.hrtime", [](auto message, auto router, auto reply) {
    router->core->os.hrtime(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.availableMemory", [](auto message, auto router, auto reply) {
    router->core->os.availableMemory(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.paths", [](auto message, auto router, auto reply) {
    JSON::Object data;

    // paths
    String resources = getcwd();
    String downloads;
    String documents;
    String pictures;
    String desktop;
    String videos;
    String music;
    String home;

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
    home = String(NSHomeDirectory().UTF8String);

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

    home = Path(HOME).string();
  #elif defined(_WIN32)
    static const auto HOME = Env::get("HOMEPATH", Env::get("HOME"));
    static const auto USERPROFILE = Env::get("USERPROFILE", HOME);
    downloads = (Path(USERPROFILE) / "Downloads").string();
    documents = (Path(USERPROFILE) / "Documents").string();
    desktop = (Path(USERPROFILE) / "Desktop").string();
    pictures = (Path(USERPROFILE) / "Pictures").string();
    videos = (Path(USERPROFILE) / "Videos").string();
    music = (Path(USERPROFILE) / "Music").string();
    home = Path(USERPROFILE).string();
  #endif

    data["resources"] = resources;
    data["downloads"] = downloads;
    data["documents"] = documents;
    data["pictures"] = pictures;
    data["desktop"] = desktop;
    data["videos"] = videos;
    data["music"] = music;
    data["home"] = home;

    return reply(Result::Data { message, data });
  });

  router->map("permissions.query", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");

  #if defined(__APPLE__)
    if (name == "geolocation") {
      if (router->locationObserver.isAuthorized) {
        auto data = JSON::Object::Entries {{"state", "granted"}};
        return reply(Result::Data { message, data });
      } else if (router->locationObserver.locationManager) {
        auto authorizationStatus = (
          router->locationObserver.locationManager.authorizationStatus
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

  router->map("permissions.request", [](auto message, auto router, auto reply) {
    static auto userConfig = SSC::getUserConfig();
    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");

    if (name == "geolocation") {
    #if defined(__APPLE__)
      auto performedActivation = [router->locationObserver attemptActivationWithCompletion: ^(BOOL isAuthorized) {
        if (!isAuthorized) {
          auto reason = @("Location observer could not be activated");

          if (!router->locationObserver.locationManager) {
            reason = @("Location observer manager is not initialized");
          } else if (!router->locationObserver.locationManager.location) {
            reason = @("Location observer manager could not provide location");
          }

          auto error = [NSError
            errorWithDomain: @(userConfig["bundle_identifier"].c_str())
            code: -1
            userInfo: @{
              NSLocalizedDescriptionKey: reason
            }
          ];
        }

        if (isAuthorized) {
          auto data = JSON::Object::Entries {{"state", "granted"}};
          return reply(Result::Data { message, data });
        } else if (router->locationObserver.locationManager.authorizationStatus == kCLAuthorizationStatusNotDetermined) {
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
  router->map("platform.event", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    if (!router->isReady) router->isReady = true;

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
  router->map("platform.notify", [](auto message, auto router, auto reply) {
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
   * Requests a URL to be opened externally.
   * @param value
   */
  router->map("platform.openExternal", [](auto message, auto router, auto reply) mutable {
    static const auto applicationProtocol = userConfig["meta_application_protocol"];
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
  router->map("platform.primordials", [](auto message, auto router, auto reply) {
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
  router->map("post", [](auto message, auto router, auto reply) {
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
   * Prints incoming message value to stdout.
   */
  router->map("stdout", [](auto message, auto router, auto reply) {
  #if defined(__APPLE__)
    os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_INFO, "%{public}s", message.value.c_str());
  #endif
    IO::write(message.value, false);
    reply(Result::Data { message, JSON::Object {}});
  });

  /**
   * Prints incoming message value to stderr.
   */
  router->map("stderr", [](auto message, auto router, auto reply) {
  #if defined(__APPLE__)
    os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_ERROR, "%{public}s", message.value.c_str());
  #endif
    IO::write(message.value, true);
    reply(Result::Data { message, JSON::Object {}});
  });

  /**
   * Binds an UDP socket to a specified port, and optionally a host
   * address (default: 0.0.0.0).
   * @param id Handle ID of underlying socket
   * @param port Port to bind the UDP socket to
   * @param address The address to bind the UDP socket to (default: 0.0.0.0)
   * @param reuseAddr Reuse underlying UDP socket address (default: false)
   */
  router->map("udp.bind", [](auto message, auto router, auto reply) {
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
  router->map("udp.close", [](auto message, auto router, auto reply) {
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
  router->map("udp.connect", [](auto message, auto router, auto reply) {
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
  router->map("udp.disconnect", [](auto message, auto router, auto reply) {
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
  router->map("udp.getPeerName", [](auto message, auto router, auto reply) {
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
  router->map("udp.getSockName", [](auto message, auto router, auto reply) {
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
  router->map("udp.getState", [](auto message, auto router, auto reply) {
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
  router->map("udp.readStart", [](auto message, auto router, auto reply) {
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
  router->map("udp.readStop", [](auto message, auto router, auto reply) {
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
  router->map("udp.send", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "port"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    Core::UDP::SendOptions options;
    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);
    REQUIRE_AND_GET_MESSAGE_VALUE(options.port, "port", std::stoi);

    options.size = message.buffer.size;
    options.bytes = message.buffer.bytes;
    options.address = message.get("address", "0.0.0.0");
    options.ephemeral = message.get("ephemeral") == "true";

    router->core->udp.send(
      message.seq,
      id,
      options,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  router->map("window.showFileSystemPicker", [](auto message, auto router, auto reply) {
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

static void registerSchemeHandler (Router *router) {
  static auto userConfig = SSC::getUserConfig();
  static auto bundleIdentifier = userConfig["meta_bundle_identifier"];

#if defined(__linux__) && !defined(__ANDROID__)
  auto ctx = router->webkitWebContext;
  auto security = webkit_web_context_get_security_manager(ctx);

  webkit_web_context_register_uri_scheme(ctx, "ipc", [](auto request, auto ptr) {
    auto uri = String(webkit_uri_scheme_request_get_uri(request));
    auto router = reinterpret_cast<Router *>(ptr);
    auto invoked = router->invoke(uri, [=](auto result) {
      auto json = result.str();
      auto size = result.post.body != nullptr ? result.post.length : json.size();
      auto body = result.post.body != nullptr ? result.post.body : json.c_str();

      char* data = nullptr;

      if (size > 0) {
        data = new char[size]{0};
        memcpy(data, body, size);
      }

      auto stream = g_memory_input_stream_new_from_data(data, size, nullptr);
      auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
      auto response = webkit_uri_scheme_response_new(stream, size);

      for (const auto& header : result.headers.entries) {
        soup_message_headers_append(headers, header.key.c_str(), header.value.c_str());
      }

      if (result.post.body) {
        webkit_uri_scheme_response_set_content_type(response, IPC_BINARY_CONTENT_TYPE);
      } else {
        webkit_uri_scheme_response_set_content_type(response, IPC_JSON_CONTENT_TYPE);
      }

      webkit_uri_scheme_request_finish_with_response(request, response);
      g_input_stream_close_async(stream, 0, nullptr, +[](
        GObject* object,
        GAsyncResult* asyncResult,
        gpointer userData
      ) {
        auto stream = (GInputStream*) object;
        g_input_stream_close_finish(stream, asyncResult, nullptr);
        g_object_unref(stream);
        g_idle_add_full(
          G_PRIORITY_DEFAULT_IDLE,
          (GSourceFunc) [](gpointer userData) {
            return G_SOURCE_REMOVE;
          },
          userData,
           [](gpointer userData) {
            delete [] static_cast<char *>(userData);
          }
        );
      }, data);
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
      webkit_uri_scheme_response_set_content_type(response, IPC_JSON_CONTENT_TYPE);
      webkit_uri_scheme_request_finish_with_response(request, response);
      g_object_unref(stream);
    }
  },
  router,
  0);

  webkit_web_context_register_uri_scheme(ctx, "socket", [](auto request, auto ptr) {
    static auto userConfig = SSC::getUserConfig();
    bool isModule = false;
    auto uri = String(webkit_uri_scheme_request_get_uri(request));
    auto cwd = getcwd();

    if (uri.starts_with("socket:///")) {
      uri = uri.substr(10);
    } else if (uri.starts_with("socket://")) {
      uri = uri.substr(9);
    } else if (uri.starts_with("socket:")) {
      uri = uri.substr(7);
    }

    auto path = String(
      uri.starts_with(bundleIdentifier)
        ? uri.substr(bundleIdentifier.size())
        : "socket/" + uri
    );

    auto ext = fs::path(path).extension().string();

    if (ext.size() > 0 && !ext.starts_with(".")) {
      ext = "." + ext;
    }

    if (!uri.starts_with(bundleIdentifier)) {
      if (ext.size() == 0 && !path.ends_with(".js")) {
        path += ".js";
        ext = ".js";
      }

      uri = "socket://" + bundleIdentifier + "/" + path;
      auto moduleSource = trim(tmpl(
        moduleTemplate,
        Map { {"url", String(uri)} }
      ));

      auto size = moduleSource.size();
      auto bytes = moduleSource.data();
      auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);
      auto response = webkit_uri_scheme_response_new(stream, size);

      webkit_uri_scheme_response_set_content_type(response, SOCKET_MODULE_CONTENT_TYPE);
      webkit_uri_scheme_request_finish_with_response(request, response);
      g_object_unref(stream);
      return;
    }

    auto parsedPath = Router::parseURL(path);
    auto resolved = Router::resolveURLPathForWebView(parsedPath.path, cwd);
    auto mount = Router::resolveNavigatorMountForWebView(parsedPath.path);
    path = resolved.path;

    if (mount.path.size() > 0) {
      if (mount.resolution.redirect) {
        auto redirectURL = resolved.path;
        if (parsedPath.queryString.size() > 0) {
          redirectURL += "?" + parsedPath.queryString;
        }

        if (parsedPath.fragment.size() > 0) {
          redirectURL += "#" + parsedPath.fragment;
        }

        auto redirectSource = String(
          "<meta http-equiv=\"refresh\" content=\"0; url='" + redirectURL + "'\" />"
        );

        auto size = redirectSource.size();
        auto bytes = redirectSource.data();
        auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);
        auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
        auto response = webkit_uri_scheme_response_new(stream, (gint64) size);

        soup_message_headers_append(headers, "location", redirectURL.c_str());
        soup_message_headers_append(headers, "content-location", redirectURL.c_str());

        webkit_uri_scheme_response_set_http_headers(response, headers);
        webkit_uri_scheme_response_set_content_type(response, "text/html");
        webkit_uri_scheme_request_finish_with_response(request, response);

        g_object_unref(stream);
        return;
      }
    } else if (path.size() == 0 && userConfig.contains("webview_default_index")) {
      path = userConfig["webview_default_index"];
    } else if (resolved.redirect) {
      auto redirectURL = resolved.path;
      if (parsedPath.queryString.size() > 0) {
        redirectURL += "?" + parsedPath.queryString;
      }

      if (parsedPath.fragment.size() > 0) {
        redirectURL += "#" + parsedPath.fragment;
      }

      auto redirectSource = String(
        "<meta http-equiv=\"refresh\" content=\"0; url='" + redirectURL + "'\" />"
      );

      auto size = redirectSource.size();
      auto bytes = redirectSource.data();
      auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);
      auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
      auto response = webkit_uri_scheme_response_new(stream, (gint64) size);

      soup_message_headers_append(headers, "location", redirectURL.c_str());
      soup_message_headers_append(headers, "content-location", redirectURL.c_str());

      webkit_uri_scheme_response_set_http_headers(response, headers);
      webkit_uri_scheme_response_set_content_type(response, "text/html");
      webkit_uri_scheme_request_finish_with_response(request, response);

      g_object_unref(stream);
      return;
    }

    if (mount.path.size() > 0) {
      path = mount.path;
    } else if (path.size() > 0) {
      path = fs::absolute(fs::path(cwd) / path.substr(1)).string();
    }

    if (path.size() == 0 || !fs::exists(path)) {
      auto stream = g_memory_input_stream_new_from_data(nullptr, 0, 0);
      auto response = webkit_uri_scheme_response_new(stream, 0);

      webkit_uri_scheme_response_set_status(response, 404, "Not found");
      webkit_uri_scheme_request_finish_with_response(request, response);
      g_object_unref(stream);
      return;
    }

    GError* error = nullptr;

    auto file = g_file_new_for_path(path.c_str());
    auto stream = (GInputStream*) g_file_read(file, nullptr, &error);

    if (!stream) {
      webkit_uri_scheme_request_finish_error(request, error);
      g_error_free(error);
      return;
    }

    gchar* mimeType = nullptr;
    auto size = fs::file_size(path);
    auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
    auto response = webkit_uri_scheme_response_new(stream, (gint64) size);
    auto webviewHeaders = split(userConfig["webview_headers"], '\n');

    soup_message_headers_append(headers, "access-control-allow-origin", "*");
    soup_message_headers_append(headers, "access-control-allow-methods", "*");
    soup_message_headers_append(headers, "access-control-allow-headers", "*");

    for (const auto& line : webviewHeaders) {
      auto pair = split(trim(line), ':');
      auto key = trim(pair[0]);
      auto value = trim(pair[1]);
      soup_message_headers_append(headers, key.c_str(), value.c_str());
    }

    webkit_uri_scheme_response_set_http_headers(response, headers);

    if (path.ends_with(".wasm")) {
      webkit_uri_scheme_response_set_content_type(response, "application/wasm");
    } else if (path.ends_with(".cjs") || path.ends_with(".mjs")) {
      webkit_uri_scheme_response_set_content_type(response, "text/javascript");
    } else if (path.ends_with(".ts")) {
      webkit_uri_scheme_response_set_content_type(response, "application/typescript");
    } else {
      mimeType = g_content_type_guess(path.c_str(), nullptr, 0, nullptr);
      if (mimeType) {
        webkit_uri_scheme_response_set_content_type(response, mimeType);
      } else {
        webkit_uri_scheme_response_set_content_type(response, SOCKET_MODULE_CONTENT_TYPE);
      }
    }

    webkit_uri_scheme_request_finish_with_response(request, response);
    g_object_unref(stream);

    if (mimeType) {
      g_free(mimeType);
    }
  },
  router,
  0);

  webkit_security_manager_register_uri_scheme_as_display_isolated(security, "ipc");
  webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "ipc");
  webkit_security_manager_register_uri_scheme_as_secure(security, "ipc");
  webkit_security_manager_register_uri_scheme_as_local(security, "ipc");

  static const auto devHost = SSC::getDevHost();

  if (devHost.starts_with("http:")) {
    webkit_security_manager_register_uri_scheme_as_display_isolated(security, "http");
    webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "http");
    webkit_security_manager_register_uri_scheme_as_secure(security, "http");
    webkit_security_manager_register_uri_scheme_as_local(security, "http");
  }

  webkit_security_manager_register_uri_scheme_as_display_isolated(security, "socket");
  webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "socket");
  webkit_security_manager_register_uri_scheme_as_secure(security, "socket");
  webkit_security_manager_register_uri_scheme_as_local(security, "socket");
#endif
}

#if defined(__APPLE__)
@implementation SSCIPCSchemeHandler
{
  SSC::Mutex mutex;
  std::unordered_map<Task, IPC::Message> tasks;
}

- (void) enqueueTask: (Task) task withMessage: (IPC::Message) message {
  Lock lock(mutex);
  if (task != nullptr && !tasks.contains(task)) {
    tasks.emplace(task, message);
  }
}

- (void) finalizeTask: (Task) task {
  Lock lock(mutex);
  if (task != nullptr && tasks.contains(task)) {
    tasks.erase(task);
  }
}

- (bool) waitingForTask: (Task) task {
  Lock lock(mutex);
  return task != nullptr && tasks.contains(task);
}

- (void) webView: (SSCBridgedWebView*) webview stopURLSchemeTask: (Task) task {
  Lock lock(mutex);
  if (tasks.contains(task)) {
    auto message = tasks[task];
    if (message.cancel->handler != nullptr) {
      message.cancel->handler(message.cancel->data);
    }
  }
  [self finalizeTask: task];
}

- (void) webView: (SSCBridgedWebView*) webview startURLSchemeTask: (Task) task {
  static auto userConfig = SSC::getUserConfig();
  static auto bundleIdentifier = userConfig["meta_bundle_identifier"];
  static auto fileManager = [[NSFileManager alloc] init];

  auto request = task.request;
  auto url = String(request.URL.absoluteString.UTF8String);
  auto message = Message(url, true);
  message.isHTTP = true;
  message.cancel = std::make_shared<MessageCancellation>();

  auto webviewHeaders = split(userConfig["webview_headers"], '\n');
  auto headers = [NSMutableDictionary dictionary];

  for (const auto& line : webviewHeaders) {
    auto pair = split(trim(line), ':');
    auto key = [NSString stringWithUTF8String: trim(pair[0]).c_str()];
    auto value = [NSString stringWithUTF8String: trim(pair[1]).c_str()];
    headers[key] = value;
  }

  headers[@"access-control-allow-origin"] = @"*";
  headers[@"access-control-allow-methods"] = @"*";
  headers[@"access-control-allow-headers"] = @"*";

  if (String(request.HTTPMethod.UTF8String) == "OPTIONS") {
    auto response = [[NSHTTPURLResponse alloc]
      initWithURL: request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: headers
    ];

    [task didReceiveResponse: response];
    [task didFinish];
    #if !__has_feature(objc_arc)
    [response release];
    #endif

    return;
  }

  if (String(request.URL.scheme.UTF8String) == "socket") {
    auto host = request.URL.host;
    auto components = [NSURLComponents
            componentsWithURL: request.URL
      resolvingAgainstBaseURL: YES
    ];

    components.scheme = @"file";
    components.host = request.URL.host;

    NSData* data = nullptr;
    bool isModule = false;
  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    const auto basePath = String(NSBundle.mainBundle.resourcePath.UTF8String) + "/ui";
  #else
    const auto basePath = String(NSBundle.mainBundle.resourcePath.UTF8String);
  #endif
    auto path = String(components.path.UTF8String);

    auto ext = String(
      components.URL.pathExtension.length > 0
        ? components.URL.pathExtension.UTF8String
        : ""
    );

    if (ext.size() > 0 && !ext.starts_with(".")) {
      ext = "." + ext;
    }

    // assumes `import 'socket:<bundle_identifier>/module'` syntax
    if (host == nullptr && path.starts_with(bundleIdentifier)) {
      host = @(path.substr(0, bundleIdentifier.size()).c_str());
      path = path.substr(bundleIdentifier.size());

      if (ext.size() == 0 && !path.ends_with(".js")) {
        path += ".js";
      }

      components.path = @(path.c_str());
    }

    if (
      host.UTF8String != nullptr &&
      String(host.UTF8String) == bundleIdentifier
    ) {
      auto parsedPath = Router::parseURL(path);
      auto resolved = Router::resolveURLPathForWebView(path, basePath);
      auto mount = Router::resolveNavigatorMountForWebView(path);
      path = resolved.path;

      if (mount.path.size() > 0) {
        if (mount.resolution.redirect) {
          auto redirectURL = mount.resolution.path + "?" + parsedPath.queryString + "#" + parsedPath.fragment;
          auto redirectSource = String(
            "<meta http-equiv=\"refresh\" content=\"0; url='" + redirectURL + "'\" />"
          );

          data = [@(redirectSource.c_str()) dataUsingEncoding: NSUTF8StringEncoding];

          auto response = [[NSHTTPURLResponse alloc]
            initWithURL: [NSURL URLWithString: @(redirectURL.c_str())]
             statusCode: 200
            HTTPVersion: @"HTTP/1.1"
           headerFields: headers
          ];

          [task didReceiveResponse: response];
          [task didReceiveData: data];
          [task didFinish];

        #if !__has_feature(objc_arc)
          [response release];
        #endif
          return;
        } else {
          auto url = [NSURL fileURLWithPath: @(mount.path.c_str())];

          if (path.ends_with(".wasm")) {
            headers[@"content-type"] = @("application/wasm");
          } else if (path.ends_with(".ts")) {
            headers[@"content-type"] = @("application/typescript");
          } else if (path.ends_with(".cjs") || path.ends_with(".mjs")) {
            headers[@"content-type"] = @("text/javascript");
          } else if (components.URL.pathExtension != nullptr) {
            auto types = [UTType
                  typesWithTag: components.URL.pathExtension
                      tagClass: UTTagClassFilenameExtension
              conformingToType: nullptr
            ];

            if (types.count > 0 && types.firstObject.preferredMIMEType) {
              headers[@"content-type"] = types.firstObject.preferredMIMEType;
            }
          }

          [url startAccessingSecurityScopedResource];
          const auto data = [NSData dataWithContentsOfURL: url];
          headers[@"content-length"] = [@(data.length) stringValue];
          [url stopAccessingSecurityScopedResource];

          auto response = [[NSHTTPURLResponse alloc]
            initWithURL: request.URL
             statusCode: 200
            HTTPVersion: @"HTTP/1.1"
           headerFields: headers
          ];

          [task didReceiveResponse: response];
          [task didReceiveData: data];
          [task didFinish];

        #if !__has_feature(objc_arc)
          [response release];
        #endif
          return;
        }
      } else if (path.size() == 0) {
        if (userConfig.contains("webview_default_index")) {
          path = userConfig["webview_default_index"];
        } else {
          auto response = [[NSHTTPURLResponse alloc]
            initWithURL: request.URL
            statusCode: 404
            HTTPVersion: @"HTTP/1.1"
            headerFields: headers
          ];

          [task didReceiveResponse: response];
          [task didFinish];

        #if !__has_feature(objc_arc)
          [response release];
        #endif
          return;
        }
      } else if (resolved.redirect) {
        auto redirectURL = path + "?" + parsedPath.queryString + "#" + parsedPath.fragment;
        auto redirectSource = String(
          "<meta http-equiv=\"refresh\" content=\"0; url='" + redirectURL + "'\" />"
        );

        data = [@(redirectSource.c_str()) dataUsingEncoding: NSUTF8StringEncoding];

        auto response = [[NSHTTPURLResponse alloc]
          initWithURL: [NSURL URLWithString: @(redirectURL.c_str())]
           statusCode: 200
          HTTPVersion: @"HTTP/1.1"
         headerFields: headers
        ];

        [task didReceiveResponse: response];
        [task didReceiveData: data];
        [task didFinish];

        #if !__has_feature(objc_arc)
        [response release];
        #endif
        return;
      }

      components.host = @("");

      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        components.path = [[[NSBundle mainBundle] resourcePath]
          stringByAppendingPathComponent: [NSString
            stringWithFormat: @"/ui/%s", path.c_str()
          ]
        ];
      #else
        components.path = [[[NSBundle mainBundle] resourcePath]
          stringByAppendingPathComponent: [NSString
            stringWithFormat: @"/%s", path.c_str()
          ]
        ];
      #endif

      if (String(request.HTTPMethod.UTF8String) == "GET") {
        data = [NSData dataWithContentsOfURL: components.URL];
      }

      components.host = request.URL.host;
    } else {
      isModule = true;
      if (ext.size() == 0 && !path.ends_with(".js")) {
        path += ".js";
      }

      auto prefix = String(
        path.starts_with(bundleIdentifier)
          ? ""
          : "socket/"
      );

      path = replace(path, bundleIdentifier + "/", "");

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      components.path = [[[NSBundle mainBundle] resourcePath]
        stringByAppendingPathComponent: [NSString
          stringWithFormat: @"/ui/%s%s", prefix.c_str(), path.c_str()
        ]
      ];
    #else
      components.path = [[[NSBundle mainBundle] resourcePath]
        stringByAppendingPathComponent: [NSString
          stringWithFormat: @"/%s%s", prefix.c_str(), path.c_str()
        ]
      ];
    #endif
      auto moduleUri = "socket://" + bundleIdentifier + "/" + prefix + path;
      auto moduleSource = trim(tmpl(
        moduleTemplate,
        Map { {"url", String(moduleUri)} }
      ));

      if (String(request.HTTPMethod.UTF8String) == "GET") {
        data = [@(moduleSource.c_str()) dataUsingEncoding: NSUTF8StringEncoding];
      }
    }

    auto exists = [fileManager
      fileExistsAtPath: components.path
           isDirectory: NULL];

    components.path = @(path.c_str());

    if (exists && data) {
      headers[@"content-length"] = [@(data.length) stringValue];
      if (isModule && data.length > 0) {
        headers[@"content-type"] = @"text/javascript";
      } else if (path.ends_with(".ts")) {
        headers[@"content-type"] = @("application/typescript");
      } else if (path.ends_with(".cjs") || path.ends_with(".mjs")) {
        headers[@"content-type"] = @("text/javascript");
      } else if (path.ends_with(".wasm")) {
        headers[@"content-type"] = @("application/wasm");
      } else if (components.URL.pathExtension != nullptr) {
        auto types = [UTType
              typesWithTag: components.URL.pathExtension
                  tagClass: UTTagClassFilenameExtension
          conformingToType: nullptr
        ];

        if (types.count > 0 && types.firstObject.preferredMIMEType) {
          headers[@"content-type"] = types.firstObject.preferredMIMEType;
        }
      }
    }

    components.scheme = @("socket");
    headers[@"content-location"] = components.URL.absoluteString;

    auto statusCode = exists ? 200 : 404;
    auto response = [[NSHTTPURLResponse alloc]
      initWithURL: components.URL
       statusCode: statusCode
      HTTPVersion: @"HTTP/1.1"
     headerFields: headers
    ];

    [task didReceiveResponse: response];

    if (data && data.length > 0) {
      [task didReceiveData: data];
    }
    [task didFinish];

    #if !__has_feature(objc_arc)
    [response release];
    #endif

    return;
  }

  if (message.name == "post") {
    auto id = std::stoull(message.get("id"));
    auto post = self.router->core->getPost(id);

    headers[@"content-length"] = [@(post.length) stringValue];

    if (post.headers.size() > 0) {
      auto lines = SSC::split(SSC::trim(post.headers), '\n');

      for (auto& line : lines) {
        auto pair = split(trim(line), ':');
        auto key = [NSString stringWithUTF8String: trim(pair[0]).c_str()];
        auto value = [NSString stringWithUTF8String: trim(pair[1]).c_str()];
        headers[key] = value;
      }
    }

    auto response = [[NSHTTPURLResponse alloc]
       initWithURL: request.URL
        statusCode: 200
       HTTPVersion: @"HTTP/1.1"
      headerFields: headers
    ];

    [task didReceiveResponse: response];

    if (post.body) {
      auto data = [NSData dataWithBytes: post.body length: post.length];
      [task didReceiveData: data];
    } else {
      auto string = [NSString stringWithUTF8String: ""];
      auto data = [string dataUsingEncoding: NSUTF8StringEncoding];
      [task didReceiveData: data];
    }

    [task didFinish];
    #if !__has_feature(objc_arc)
    [response release];
    #endif

    self.router->core->removePost(id);
    return;
  }

  size_t bufsize = 0;
  char *body = NULL;

  // if there is a body on the reuqest, pass it into the method router.
  auto rawBody = request.HTTPBody;

  if (rawBody) {
    const void* data = [rawBody bytes];
    bufsize = [rawBody length];
    body = (char *) data;
  }

  [self enqueueTask: task withMessage: message];

  auto invoked = self.router->invoke(message, body, bufsize, [=](Result result) {
    // @TODO Communicate task cancellation to the route, so it can cancel its work.
    if (![self waitingForTask: task]) {
      return;
    }

    auto id = result.id;
    auto headers = [NSMutableDictionary dictionary];

    headers[@"access-control-allow-origin"] = @"*";
    headers[@"access-control-allow-methods"] = @"*";
    headers[@"access-control-allow-headers"] = @"*";

    for (const auto& header : result.headers.entries) {
      auto key = [NSString stringWithUTF8String: trim(header.key).c_str()];
      auto value = [NSString stringWithUTF8String: trim(header.value.str()).c_str()];
      headers[key] = value;
    }

    NSData* data = nullptr;
    if (result.post.event_stream != nullptr) {
      *result.post.event_stream = [=](
        const char* name,
        const char* data,
        bool finished
      ) {
        if (![self waitingForTask: task]) {
          return false;
        }

        auto event_name = [NSString stringWithUTF8String: name];
        auto event_data = [NSString stringWithUTF8String: data];

        if (event_name.length > 0 || event_data.length > 0) {
          auto event =
              event_name.length > 0 && event_data.length > 0
                  ? [NSString stringWithFormat:@"event: %@\ndata: %@\n\n",
                                               event_name, event_data]
              : event_data.length > 0
                  ? [NSString stringWithFormat:@"data: %@\n\n", event_data]
                  : [NSString stringWithFormat:@"event: %@\n\n", event_name];

          [task didReceiveData: [event dataUsingEncoding:NSUTF8StringEncoding]];
        }

        if (finished) {
          [task didFinish];
          [self finalizeTask: task];
        }

        return true;
      };
      headers[@"content-type"] = @"text/event-stream";
      headers[@"cache-control"] = @"no-store";
    } else if (result.post.chunk_stream != nullptr) {
      *result.post.chunk_stream = [=](
        const char* chunk,
        size_t chunk_size,
        bool finished
      ) {
        if (![self waitingForTask: task]) {
          return false;
        }

        [task didReceiveData:[NSData dataWithBytes:chunk length:chunk_size]];

        if (finished) {
          [task didFinish];
          [self finalizeTask: task];
        }
        return true;
      };
      headers[@"transfer-encoding"] = @"chunked";
    } else {
      std::string json;
      const char* body;
      size_t size;
      if (result.post.body != nullptr) {
        body = result.post.body;
        size = result.post.length;
      } else {
        json = result.str();
        body = json.c_str();
        size = json.size();
        headers[@"content-type"] = @"application/json";
      }
      headers[@"content-length"] = @(size).stringValue;
      data = [NSData dataWithBytes: body length: size];
    }

    auto response = [[NSHTTPURLResponse alloc]
      initWithURL: task.request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: headers
    ];

    [task didReceiveResponse: response];
    if (data != nullptr) {
      [task didReceiveData: data];
      [task didFinish];
      [self finalizeTask: task];
    }

  #if !__has_feature(objc_arc)
    [response release];
  #endif
  });

  if (!invoked) {
    auto headers = [NSMutableDictionary dictionary];
    auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"},
        {"url", url}
      }}
    };

    auto msg = JSON::Object(json).str();
    auto str = [NSString stringWithUTF8String: msg.c_str()];
    auto data = [str dataUsingEncoding: NSUTF8StringEncoding];

    headers[@"access-control-allow-origin"] = @"*";
    headers[@"access-control-allow-headers"] = @"*";
    headers[@"content-length"] = [@(msg.size()) stringValue];

    auto response = [[NSHTTPURLResponse alloc]
       initWithURL: request.URL
        statusCode: 404
       HTTPVersion: @"HTTP/1.1"
      headerFields: headers
    ];

    [task didReceiveResponse: response];
    [task didReceiveData: data];
    [task didFinish];
    #if !__has_feature(objc_arc)
    [response release];
    #endif
  }
}
@end

@implementation SSCLocationPositionWatcher
+ (SSCLocationPositionWatcher*) positionWatcherWithIdentifier: (NSInteger) identifier
                                                   completion: (void (^)(CLLocation*)) completion {
  auto watcher= [SSCLocationPositionWatcher new];
  watcher.identifier = identifier;
  watcher.completion = [completion copy];
  return watcher;
}
@end

@implementation SSCLocationObserver
- (id) init {
  self = [super init];
  self.delegate = [[SSCLocationManagerDelegate alloc] initWithLocationObserver: self];
  self.isAuthorized = NO;
  self.locationWatchers = [NSMutableArray new];
  self.activationCompletions = [NSMutableArray new];
  self.locationRequestCompletions = [NSMutableArray new];

  self.locationManager = [CLLocationManager new];
  self.locationManager.delegate = self.delegate;
  self.locationManager.desiredAccuracy = CLAccuracyAuthorizationFullAccuracy;
  self.locationManager.pausesLocationUpdatesAutomatically = NO;

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
  self.locationManager.allowsBackgroundLocationUpdates = YES;
  self.locationManager.showsBackgroundLocationIndicator = YES;
#endif

  if ([CLLocationManager locationServicesEnabled]) {
    if (
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      self.locationManager.authorizationStatus == kCLAuthorizationStatusAuthorized ||
    #else
      self.locationManager.authorizationStatus == kCLAuthorizationStatusAuthorizedWhenInUse ||
    #endif
      self.locationManager.authorizationStatus == kCLAuthorizationStatusAuthorizedAlways
    ) {
      self.isAuthorized = YES;
    }
  }

  return self;
}

- (BOOL) attemptActivation {
  if ([CLLocationManager locationServicesEnabled] == NO) {
    return NO;
  }

  if (self.isAuthorized) {
    [self.locationManager requestLocation];
    return YES;
  }

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
  [self.locationManager requestWhenInUseAuthorization];
#else
  [self.locationManager requestAlwaysAuthorization];
#endif

  return YES;
}

- (BOOL) attemptActivationWithCompletion: (void (^)(BOOL)) completion {
  if (self.isAuthorized) {
    dispatch_async(dispatch_get_main_queue(), ^{
      completion(YES);
    });
    return YES;
  }

  if ([self attemptActivation]) {
    [self.activationCompletions addObject: [completion copy]];
    return YES;
  }

  return NO;
}

- (BOOL) getCurrentPositionWithCompletion: (void (^)(NSError*, CLLocation*)) completion {
  return [self attemptActivationWithCompletion: ^(BOOL isAuthorized) {
    static auto userConfig = SSC::getUserConfig();
    if (!isAuthorized) {
      auto reason = @("Location observer could not be activated");

      if (!self.locationManager) {
        reason = @("Location observer manager is not initialized");
      } else if (!self.locationManager.location) {
        reason = @("Location observer manager could not provide location");
      }

      auto error = [NSError
        errorWithDomain: @(userConfig["bundle_identifier"].c_str())
        code: -1
        userInfo: @{
          NSLocalizedDescriptionKey: reason
        }
      ];

      return completion(error, nullptr);
    }

    auto location = self.locationManager.location;
    if (location.timestamp.timeIntervalSince1970 > 0) {
      completion(nullptr, self.locationManager.location);
    } else {
      [self.locationRequestCompletions addObject: [completion copy]];
    }

    [self.locationManager requestLocation];
  }];
}

- (int) watchPositionForIdentifier: (NSInteger) identifier
                        completion: (void (^)(NSError*, CLLocation*)) completion {
  SSCLocationPositionWatcher* watcher = nullptr;
  BOOL exists = NO;

  for (SSCLocationPositionWatcher* existing in self.locationWatchers) {
    if (existing.identifier == identifier) {
      watcher = existing;
      exists = YES;
      break;
    }
  }

  if (!watcher) {
    watcher = [SSCLocationPositionWatcher
      positionWatcherWithIdentifier: identifier
                         completion: ^(CLLocation* location) {
      completion(nullptr, location);
    }];
  }

  auto performedActivation = [self attemptActivationWithCompletion: ^(BOOL isAuthorized) {
    static auto userConfig = SSC::getUserConfig();
    if (!isAuthorized) {
      auto error = [NSError
        errorWithDomain: @(userConfig["bundle_identifier"].c_str())
        code: -1
        userInfo: @{
          @"Error reason": @("Location observer could not be activated")
        }
      ];

      return completion(error, nullptr);
    }

    [self.locationManager startUpdatingLocation];

    if (CLLocationManager.headingAvailable) {
      [self.locationManager startUpdatingHeading];
    }

    [self.locationManager startMonitoringSignificantLocationChanges];
  }];

  if (!performedActivation) {
  #if !__has_feature(objc_arc)
    [watcher release];
    #endif
    return -1;
  }

  if (!exists) {
    [self.locationWatchers addObject: watcher];
  }

  return identifier;
}

- (BOOL) clearWatch: (NSInteger) identifier {
  for (SSCLocationPositionWatcher* watcher in self.locationWatchers) {
    if (watcher.identifier == identifier) {
      [self.locationWatchers removeObject: watcher];
    #if !__has_feature(objc_arc)
      [watcher release];
    #endif
      return YES;
    }
  }

  return NO;
}
@end

@implementation SSCLocationManagerDelegate
- (id) initWithLocationObserver: (SSCLocationObserver*) locationObserver {
  self = [super init];
  self.locationObserver = locationObserver;
  locationObserver.delegate = self;
  return self;
}

- (void) locationManager: (CLLocationManager*) locationManager
      didUpdateLocations: (NSArray<CLLocation*>*) locations {
  auto locationRequestCompletions = [NSArray arrayWithArray: self.locationObserver.locationRequestCompletions];
  for (id item in locationRequestCompletions) {
    auto completion = (void (^)(CLLocation*)) item;
    completion(locations.firstObject);
    [self.locationObserver.locationRequestCompletions removeObject: item];
  #if !__has_feature(objc_arc)
    [completion release];
  #endif
  }

  for (SSCLocationPositionWatcher* watcher in self.locationObserver.locationWatchers) {
    watcher.completion(locations.firstObject);
  }
}

- (void) locationManager: (CLLocationManager*) locationManager
        didFailWithError: (NSError*) error {
 // TODO(@jwerle): handle location manager error
  debug("locationManager:didFailWithError: %@", error);
}

- (void)            locationManager: (CLLocationManager*) locationManager
  didFinishDeferredUpdatesWithError: (NSError*) error {
  debug("locationManager:didFinishDeferredUpdatesWithError: %@", error);
 // TODO(@jwerle): handle deferred error
}

- (void) locationManagerDidPauseLocationUpdates: (CLLocationManager*) locationManager {
 // TODO(@jwerle): handle pause for updates
  debug("locationManagerDidPauseLocationUpdates");
}

- (void) locationManagerDidResumeLocationUpdates: (CLLocationManager*) locationManager {
  // TODO(@jwerle): handle resume for updates
  debug("locationManagerDidResumeLocationUpdates");
}

- (void) locationManager: (CLLocationManager*) locationManager
                didVisit: (CLVisit*) visit {
  auto locations = [NSArray arrayWithObject: locationManager.location];
  [self locationManager: locationManager didUpdateLocations: locations];
}

-       (void) locationManager: (CLLocationManager*) locationManager
  didChangeAuthorizationStatus: (CLAuthorizationStatus) status {
  // XXX(@jwerle): this is a legacy callback
  [self locationManagerDidChangeAuthorization: locationManager];
}

- (void) locationManagerDidChangeAuthorization: (CLLocationManager*) locationManager {
  auto activationCompletions = [NSArray arrayWithArray: self.locationObserver.activationCompletions];
  if (
  #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    locationManager.authorizationStatus == kCLAuthorizationStatusAuthorized ||
  #else
    locationManager.authorizationStatus == kCLAuthorizationStatusAuthorizedWhenInUse ||
  #endif
    locationManager.authorizationStatus == kCLAuthorizationStatusAuthorizedAlways
  ) {
    JSON::Object json = JSON::Object::Entries {
      {"name", "geolocation"},
      {"state", "granted"}
    };

    self.locationObserver.router->emit("permissionchange", json.str());
    self.locationObserver.isAuthorized = YES;
    for (id item in activationCompletions) {
      auto completion = (void (^)(BOOL)) item;
      completion(YES);
      [self.locationObserver.activationCompletions removeObject: item];
    #if !__has_feature(objc_arc)
      [completion release];
    #endif
    }
  } else {
    JSON::Object json = JSON::Object::Entries {
      {"name", "geolocation"},
      {"state", locationManager.authorizationStatus == kCLAuthorizationStatusNotDetermined
        ? "prompt"
        : "denied"
      }
    };

    self.locationObserver.router->emit("permissionchange", json.str());
    self.locationObserver.isAuthorized = NO;
    for (id item in activationCompletions) {
      auto completion = (void (^)(BOOL)) item;
      completion(NO);
      [self.locationObserver.activationCompletions removeObject: item];
    #if !__has_feature(objc_arc)
      [completion release];
    #endif
    }
  }
}
@end

@implementation SSCUserNotificationCenterDelegate
-  (void) userNotificationCenter: (UNUserNotificationCenter*) center
  didReceiveNotificationResponse: (UNNotificationResponse*) response
           withCompletionHandler: (void (^)(void)) completionHandler {
  completionHandler();
  Lock lock(notificationRouterMapMutex);
  auto id = String(response.notification.request.identifier.UTF8String);
  Router* router = notificationRouterMap.find(id) != notificationRouterMap.end()
    ? notificationRouterMap.at(id)
    : nullptr;

  if (router) {
    JSON::Object json = JSON::Object::Entries {
      {"id", id},
      {"action",
        [response.actionIdentifier isEqualToString: UNNotificationDefaultActionIdentifier]
          ? "default"
          : "dismiss"
      }
    };

    notificationRouterMap.erase(id);
    router->emit("notificationresponse", json.str());
  }
}

- (void) userNotificationCenter: (UNUserNotificationCenter*) center
        willPresentNotification: (UNNotification*) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler {
  UNNotificationPresentationOptions options = UNNotificationPresentationOptionList;

  if (notification.request.content.sound != nullptr) {
    options |= UNNotificationPresentationOptionSound;
  }

  if (notification.request.content.attachments != nullptr) {
    if (notification.request.content.attachments.count > 0) {
      options |= UNNotificationPresentationOptionBanner;
    }
  }

  completionHandler(options);

  Lock lock(notificationRouterMapMutex);
  auto __block id = String(notification.request.identifier.UTF8String);
  Router* __block router = notificationRouterMap.find(id) != notificationRouterMap.end()
    ? notificationRouterMap.at(id)
    : nullptr;

  if (router) {
    JSON::Object json = JSON::Object::Entries {
      {"id", id}
    };

    router->emit("notificationpresented", json.str());
    // look for dismissed notification
    auto timer = [NSTimer timerWithTimeInterval: 2 repeats: YES block: ^(NSTimer* timer) {
      auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
      [notificationCenter getDeliveredNotificationsWithCompletionHandler: ^(NSArray<UNNotification*> *notifications) {
        BOOL found = NO;

        for (UNNotification* notification in notifications) {
          if (String(notification.request.identifier.UTF8String) == id) {
            return;
          }
        }

        [timer invalidate];
        JSON::Object json = JSON::Object::Entries {
          {"id", id},
          {"action", "dismiss"}
        };

        router->emit("notificationresponse", json.str());

        Lock lock(notificationRouterMapMutex);
        if (notificationRouterMap.contains(id)) {
          notificationRouterMap.erase(id);
        }
      }];
    }];

    [NSRunLoop.mainRunLoop
      addTimer: timer
       forMode: NSDefaultRunLoopMode
    ];
  }
}
@end
#endif

namespace SSC::IPC {
  Bridge::Bridge (Core *core) : router() {
    static auto userConfig = SSC::getUserConfig();

    this->core = core;
    this->router.core = core;
    this->router.bridge = this;

    this->bluetooth.sendFunction = [this](
      const String& seq,
      const JSON::Any value,
      const SSC::Post post
    ) {
      this->router.send(seq, value.str(), post);
    };

    this->bluetooth.emitFunction = [this](
      const String& seq,
      const JSON::Any value
    ) {
      this->router.emit(seq, value.str());
    };

  #if !defined(__ANDROID__) && (defined(_WIN32) || defined(__linux__) || (defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR))
    if (isDebugEnabled() && userConfig["webview_watch"] == "true") {
      this->fileSystemWatcher = new FileSystemWatcher(getcwd());
      this->fileSystemWatcher->core = this->core;
      this->fileSystemWatcher->start([=, this](
        const auto& path,
        const auto& events,
        const auto& context
      ) mutable {
        auto json = JSON::Object::Entries {
          {"path", std::filesystem::relative(path, getcwd()).string()}
        };

        auto result = SSC::IPC::Result(json);
        this->router.emit("filedidchange", result.json().str());
      });
    }
  #endif
  }

  Bridge::~Bridge () {
  #if !defined(__ANDROID__) && (defined(_WIN32) || defined(__linux__) || (defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR))
    if (this->fileSystemWatcher) {
      this->fileSystemWatcher->stop();
      delete this->fileSystemWatcher;
    }
  #endif
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
    MessageBuffer buffer
  ) {
    Lock lock(this->mutex);
    auto key = std::to_string(index) + seq;
    this->buffers.insert_or_assign(key, buffer);
  }

  void Router::removeMappedBuffer (int index, const Message::Seq seq) {
    Lock lock(this->mutex);
    if (this->hasMappedBuffer(index, seq)) {
      auto key = std::to_string(index) + seq;
      this->buffers.erase(key);
    }
  }

  bool Bridge::route (const String& uri, const char *bytes, size_t size) {
    return this->route(uri, bytes, size, nullptr);
  }

  bool Bridge::route (
    const String& uri,
    const char* bytes,
    size_t size,
    Router::ResultCallback callback
  ) {
    if (callback != nullptr) {
      return this->router.invoke(uri, bytes, size, callback);
    } else {
      return this->router.invoke(uri, bytes, size);
    }
  }

  /*

    .
    ├── a-conflict-index
    │             └── index.html
    ├── a-conflict-index.html
    ├── an-index-file
    │             ├── a-html-file.html
    │             └── index.html
    ├── another-file.html
    └── index.html

    Subtleties:
    Direct file navigation always wins
    /foo/index.html have precedent over foo.html
    /foo redirects to /foo/ when there is a /foo/index.html

    '/' -> '/index.html'
    '/index.html' -> '/index.html'
    '/a-conflict-index' -> redirect to '/a-conflict-index/'
    '/another-file' -> '/another-file.html'
    '/another-file.html' -> '/another-file.html'
    '/an-index-file/' -> '/an-index-file/index.html'
    '/an-index-file' -> redirect to '/an-index-file/'
    '/an-index-file/a-html-file' -> '/an-index-file/a-html-file.html'
   */
  Router::WebViewURLPathResolution Router::resolveURLPathForWebView (String inputPath, const String& basePath) {
    namespace fs = std::filesystem;

    if (inputPath.starts_with("/")) {
      inputPath = inputPath.substr(1);
    }

    // Resolve the full path
    fs::path fullPath = (fs::path(basePath) / fs::path(inputPath)).make_preferred();

    // 1. Try the given path if it's a file
    if (fs::is_regular_file(fullPath)) {
      return Router::WebViewURLPathResolution{"/" + replace(fs::relative(fullPath, basePath).string(), "\\\\", "/")};
    }

    // 2. Try appending a `/` to the path and checking for an index.html
    fs::path indexPath = fullPath / fs::path("index.html");
    if (fs::is_regular_file(indexPath)) {
      if (fullPath.string().ends_with("\\") || fullPath.string().ends_with("/")) {
        return Router::WebViewURLPathResolution{
          .path = "/" + replace(fs::relative(indexPath, basePath).string(), "\\\\", "/"),
          .redirect = false
        };
      } else {
        return Router::WebViewURLPathResolution{
          .path = "/" + replace(fs::relative(fullPath, basePath).string(), "\\\\", "/") + "/",
          .redirect = true
        };
      }
    }

    // 3. Check if appending a .html file extension gives a valid file
    fs::path htmlPath = fullPath;
    htmlPath.replace_extension(".html");
    if (fs::is_regular_file(htmlPath)) {
      return Router::WebViewURLPathResolution{"/" + replace(fs::relative(htmlPath, basePath).string(), "\\\\", "/")};
    }

    // If no valid path is found, return empty string
    return Router::WebViewURLPathResolution{};
  };

  Router::WebViewURLComponents Router::parseURL(const SSC::String& url) {
    Router::WebViewURLComponents components;
    components.originalUrl = url;

    size_t queryPos = url.find('?');
    size_t fragmentPos = url.find('#');

    if (queryPos != SSC::String::npos) {
      components.path = url.substr(0, queryPos);
    } else if (fragmentPos != SSC::String::npos) {
      components.path = url.substr(0, fragmentPos);
    } else {
      components.path = url;
    }

    if (queryPos != SSC::String::npos) { // Extract the query string
      components.queryString = url.substr(queryPos + 1, fragmentPos != SSC::String::npos ? fragmentPos - queryPos - 1 : SSC::String::npos);
    }

    if (fragmentPos != SSC::String::npos) { // Extract the fragment
      components.fragment = url.substr(fragmentPos + 1);
    }

    return components;
  }

  static const Map getWebViewNavigatorMounts () {
    static const auto userConfig = getUserConfig();
  #if defined(_WIN32)
    static const auto HOME = Env::get("HOMEPATH", Env::get("USERPROFILE", Env::get("HOME")));
  #elif defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
    static const auto HOME = String(NSHomeDirectory().UTF8String);
  #else
    static const auto uid = getuid();
    static const auto pwuid = getpwuid(uid);
    static const auto HOME = pwuid != nullptr
      ? String(pwuid->pw_dir)
      : Env::get("HOME", getcwd());
  #endif

    static Map mounts;

    if (mounts.size() > 0) {
      return mounts;
    }

    Map mappings = {
      {"\\$HOST_HOME", HOME},
      {"~", HOME},

      {"\\$HOST_CONTAINER",
    #if defined(__APPLE__)
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        [NSSearchPathForDirectoriesInDomains(NSApplicationDirectory, NSUserDomainMask, YES) objectAtIndex: 0].UTF8String
      #else
        // `homeDirectoryForCurrentUser` resolves to sandboxed container
        // directory when in "sandbox" mode, otherwise the user's HOME directory
        NSFileManager.defaultManager.homeDirectoryForCurrentUser.absoluteString.UTF8String
      #endif
    #elif defined(__linux__)
        // TODO(@jwerle): figure out `$HOST_CONTAINER` for Linux
        getcwd(),
    #elif defined(_WIN32)
        // TODO(@jwerle): figure out `$HOST_CONTAINER` for Windows
        getcwd(),
    #endif
      },

      {"\\$HOST_PROCESS_WORKING_DIRECTORY",
    #if defined(__APPLE__)
        NSBundle.mainBundle.resourcePath.UTF8String
    #else
        getcwd(),
    #endif
      }
    };

    for (const auto& tuple : userConfig) {
      if (tuple.first.starts_with("webview_navigator_mounts_")) {
        auto key = replace(tuple.first, "webview_navigator_mounts_", "");

        if (key.starts_with("android") && !platform.android) continue;
        if (key.starts_with("ios") && !platform.ios) continue;
        if (key.starts_with("linux") && !platform.linux) continue;
        if (key.starts_with("mac") && !platform.mac) continue;
        if (key.starts_with("win") && !platform.win) continue;

        key = replace(key, "android_", "");
        key = replace(key, "ios_", "");
        key = replace(key, "linux_", "");
        key = replace(key, "mac_", "");
        key = replace(key, "win_", "");

        String path = key;

        for (const auto& map : mappings) {
          path = replace(path, map.first, map.second);
        }

        const auto& value = tuple.second;
        mounts.insert_or_assign(path, value);
      }
    }

    return mounts;
  }

  Router::WebViewNavigatorMount Router::resolveNavigatorMountForWebView (const String& path) {
    static const auto mounts = getWebViewNavigatorMounts();

    for (const auto& tuple : mounts) {
      if (path.starts_with(tuple.second)) {
        const auto relative = replace(path, tuple.second, "");
        const auto resolution = resolveURLPathForWebView(relative, tuple.first);
        if (resolution.path.size() > 0) {
          const auto resolved = Path(tuple.first) / resolution.path.substr(1);
          return WebViewNavigatorMount {
            resolution,
            resolved.string(),
            path
          };
        } else {
          const auto resolved = relative.starts_with("/")
            ? Path(tuple.first) / relative.substr(1)
            : Path(tuple.first) / relative;

          return WebViewNavigatorMount {
            resolution,
            resolved.string(),
            path
          };
        }
      }
    }

    return WebViewNavigatorMount {};
  }

  Router::Router () {
    static auto userConfig = SSC::getUserConfig();

  #if defined(__APPLE__)
    this->networkStatusObserver = [SSCIPCNetworkStatusObserver new];
    this->locationObserver = [SSCLocationObserver new];
    this->schemeHandler = [SSCIPCSchemeHandler new];

    [this->schemeHandler setRouter: this];
    [this->locationObserver setRouter: this];
    [this->networkStatusObserver setRouter: this];

  #elif defined(__linux__) && !defined(__ANDROID__)
    this->webkitWebContext = webkit_web_context_new();
  #endif

    initRouterTable(this);
    registerSchemeHandler(this);

    this->preserveCurrentTable();

  #if defined(__APPLE__)
    [this->networkStatusObserver start];

    if (userConfig["permissions_allow_notifications"] != "false") {
      auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];

      if (!notificationCenter.delegate) {
        notificationCenter.delegate = [SSCUserNotificationCenterDelegate new];
      }

      UNAuthorizationStatus __block currentAuthorizationStatus;
      [notificationCenter getNotificationSettingsWithCompletionHandler: ^(UNNotificationSettings *settings) {
        currentAuthorizationStatus = settings.authorizationStatus;
        this->notificationPollTimer = [NSTimer timerWithTimeInterval: 2 repeats: YES block: ^(NSTimer* timer) {
          // look for authorization status changes
          [notificationCenter getNotificationSettingsWithCompletionHandler: ^(UNNotificationSettings *settings) {
            if (currentAuthorizationStatus != settings.authorizationStatus) {
              JSON::Object json;
              currentAuthorizationStatus = settings.authorizationStatus;
              if (settings.authorizationStatus == UNAuthorizationStatusDenied) {
                json = JSON::Object::Entries {
                  {"name", "notifications"},
                  {"state", "denied"}
                };
              } else if (settings.authorizationStatus == UNAuthorizationStatusNotDetermined) {
                json = JSON::Object::Entries {
                  {"name", "notifications"},
                  {"state", "prompt"}
                };
              } else {
                json = JSON::Object::Entries {
                  {"name", "notifications"},
                  {"state", "granted"}
                };
              }

              this->emit("permissionchange", json.str());
            }
          }];
        }];

        [NSRunLoop.mainRunLoop
          addTimer: this->notificationPollTimer
            forMode: NSDefaultRunLoopMode
        ];
      }];
    }
  #endif
  }

  Router::~Router () {
  #if defined(__APPLE__)
    if (this->networkStatusObserver != nullptr) {
    #if !__has_feature(objc_arc)
      [this->networkStatusObserver release];
    #endif
    }

    if (this->locationObserver != nullptr) {
    #if !__has_feature(objc_arc)
      [this->locationObserver release];
    #endif
    }

    if (this->schemeHandler != nullptr) {
    #if !__has_feature(objc_arc)
      [this->schemeHandler release];
    #endif
    }

    if (this->notificationPollTimer) {
      [this->notificationPollTimer invalidate];
    #if !__has_feature(objc_arc)
      [this->notificationPollTimer release];
    #endif
    }

    this->notificationPollTimer = nullptr;
    this->networkStatusObserver = nullptr;
    this->locationObserver = nullptr;
    this->schemeHandler = nullptr;
  #endif
  }

  void Router::preserveCurrentTable () {
    Lock lock(mutex);
    this->preserved = this->table;
  }

  uint64_t Router::listen (const String& name, MessageCallback callback) {
    Lock lock(mutex);

    if (!this->listeners.contains(name)) {
      this->listeners[name] = std::vector<MessageCallbackListenerContext>();
    }

    auto& listeners = this->listeners.at(name);
    auto token = rand64();
    listeners.push_back(MessageCallbackListenerContext { token , callback });
    return token;
  }

  bool Router::unlisten (const String& name, uint64_t token) {
    Lock lock(mutex);

    if (!this->listeners.contains(name)) {
      return false;
    }

    auto& listeners = this->listeners.at(name);
    for (int i = 0; i < listeners.size(); ++i) {
      const auto& listener = listeners[i];
      if (listener.token == token) {
        listeners.erase(listeners.begin() + i);
        return true;
      }
    }

    return false;
  }

  void Router::map (const String& name, MessageCallback callback) {
    return this->map(name, true, callback);
  }

  void Router::map (const String& name, bool async, MessageCallback callback) {
    Lock lock(mutex);

    String data = name;
    // URI hostnames are not case sensitive. Convert to lowercase.
    std::transform(data.begin(), data.end(), data.begin(),
      [](unsigned char c) { return std::tolower(c); });
    if (callback != nullptr) {
      table.insert_or_assign(data, MessageCallbackContext { async, callback });
    }
  }

  void Router::unmap (const String& name) {
    Lock lock(mutex);

    String data = name;
    // URI hostnames are not case sensitive. Convert to lowercase.
    std::transform(data.begin(), data.end(), data.begin(),
      [](unsigned char c) { return std::tolower(c); });
    if (table.find(data) != table.end()) {
      table.erase(data);
    }
  }

  bool Router::invoke (const String& uri, const char *bytes, size_t size) {
    return this->invoke(uri, bytes, size, [this](auto result) {
      this->send(result.seq, result.str(), result.post);
    });
  }

  bool Router::invoke (const String& uri, ResultCallback callback) {
    return this->invoke(uri, nullptr, 0, callback);
  }

  bool Router::invoke (
    const String& uri,
    const char *bytes,
    size_t size,
    ResultCallback callback
  ) {
    auto message = Message(uri, true);
    return this->invoke(message, bytes, size, callback);
  }

  bool Router::invoke (
    const Message& message,
    const char *bytes,
    size_t size,
    ResultCallback callback
  ) {
    auto name = message.name;
    MessageCallbackContext ctx;

    // URI hostnames are not case sensitive. Convert to lowercase.
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
      return std::tolower(c);
    });

    do {
      // lookup router function in the preserved table,
      // then the public table, return if unable to determine a context
      if (this->preserved.find(name) != this->preserved.end()) {
        ctx = this->preserved.at(name);
      } else if (this->table.find(name) != this->table.end()) {
        ctx = this->table.at(name);
      } else {
        return false;
      }
    } while (0);

    if (ctx.callback != nullptr) {
      Message msg(message);
      // decorate message with buffer if buffer was previously
      // mapped with `ipc://buffer.map`, which we do on Linux
      if (this->hasMappedBuffer(msg.index, msg.seq)) {
        msg.buffer = this->getMappedBuffer(msg.index, msg.seq);
        this->removeMappedBuffer(msg.index, msg.seq);
      } else if (bytes != nullptr && size > 0) {
        // alloc and copy `bytes` into `msg.buffer.bytes - caller owns `bytes`
        // `msg.buffer.bytes` is free'd in CLEANUP_AFTER_INVOKE_CALLBACK
        msg.buffer.bytes = new char[size]{0};
        msg.buffer.size = size;
        memcpy(msg.buffer.bytes, bytes, size);
      }

      // named listeners
      do {
        auto listeners = this->listeners[name];
        for (const auto& listener : listeners) {
          listener.callback(msg, this, [](const auto& _) {});
        }
      } while (0);

      // wild card (*) listeners
      do {
        auto listeners = this->listeners["*"];
        for (const auto& listener : listeners) {
          listener.callback(msg, this, [](const auto& _) {});
        }
      } while (0);

      if (ctx.async) {
        auto dispatched = this->dispatch([ctx, msg, callback, this]() mutable {
          ctx.callback(msg, this, [msg, callback, this](const auto result) mutable {
            if (result.seq == "-1") {
              this->send(result.seq, result.str(), result.post);
            } else {
              callback(result);
            }

            CLEANUP_AFTER_INVOKE_CALLBACK(this, msg, result);
          });
        });

        if (!dispatched) {
          CLEANUP_AFTER_INVOKE_CALLBACK(this, msg, Result{});
        }

        return dispatched;
      } else {
        ctx.callback(msg, this, [msg, callback, this](const auto result) mutable {
          if (result.seq == "-1") {
            this->send(result.seq, result.str(), result.post);
          } else {
            callback(result);
          }
          CLEANUP_AFTER_INVOKE_CALLBACK(this, msg, result);
        });

        return true;
      }
    }

    return false;
  }

  bool Router::send (
    const Message::Seq& seq,
    const String data,
    const Post post
  ) {
    if (post.body || seq == "-1") {
      auto script = this->core->createPost(seq, data, post);
      return this->evaluateJavaScript(script);
    }

    auto value = encodeURIComponent(data);
    auto script = getResolveToRenderProcessJavaScript(
      seq.size() == 0 ? "-1" : seq,
      "0",
      value
    );

    return this->evaluateJavaScript(script);
  }

  bool Router::emit (
    const String& name,
    const String data
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

#if defined(__APPLE__)
@implementation SSCIPCNetworkStatusObserver
- (id) init {
  self = [super init];
  dispatch_queue_attr_t attrs = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_SERIAL,
    QOS_CLASS_UTILITY,
    DISPATCH_QUEUE_PRIORITY_DEFAULT
  );

  _monitorQueue = dispatch_queue_create(
    "co.socketsupply.queue.ipc.network-status-observer",
    attrs
  );

  // self.monitor = nw_path_monitor_create_with_type(nw_interface_type_wifi);
  _monitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(_monitor, _monitorQueue);
  nw_path_monitor_set_update_handler(_monitor, ^(nw_path_t path) {
    if (path == nullptr) {
      return;
    }

    nw_path_status_t status = nw_path_get_status(path);

    String name;
    String message;

    switch (status) {
      case nw_path_status_invalid: {
        name = "offline";
        message = "Network path is invalid";
        break;
      }
      case nw_path_status_satisfied: {
        name = "online";
        message = "Network is usable";
        break;
      }
      case nw_path_status_satisfiable: {
        name = "online";
        message = "Network may be usable";
        break;
      }
      case nw_path_status_unsatisfied: {
        name = "offline";
        message = "Network is not usable";
        break;
      }
    }

    auto json = JSON::Object::Entries {
      {"message", message}
    };

    auto router = self.router;
    self.router->dispatch([router, json, name] () {
      auto data = JSON::Object(json);
      router->emit(name, data.str());
    });
  });

  return self;
}

- (void) start {
  nw_path_monitor_start(_monitor);
}
@end
#endif
