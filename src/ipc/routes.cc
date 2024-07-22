#include "../app/app.hh"
#include "../cli/cli.hh"
#include "../core/json.hh"
#include "../extension/extension.hh"
#include "../window/window.hh"
#include "ipc.hh"

extern int LLAMA_BUILD_NUMBER;

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

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    auto bundleIdentifier = userConfig["meta_bundle_identifier"];
    auto SOCKET_RUNTIME_OS_LOG_BUNDLE = os_log_create(bundleIdentifier.c_str(), "socket.runtime");
  #endif

  /**
   * AI
   */
  router->map("ai.llm.create", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "path", "prompt"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    SSC::LLMOptions options;
    options.path = message.get("path");
    options.prompt = message.get("prompt");
    options.antiprompt = message.get("antiprompt");

    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "id", std::stoull);

    if (message.has("n_batch")) REQUIRE_AND_GET_MESSAGE_VALUE(options.n_batch, "n_batch", std::stoi);
    if (message.has("n_ctx")) REQUIRE_AND_GET_MESSAGE_VALUE(options.n_ctx, "n_ctx", std::stoi);
    if (message.has("n_gpu_layers")) REQUIRE_AND_GET_MESSAGE_VALUE(options.n_gpu_layers, "n_gpu_layers", std::stoi);
    if (message.has("n_keep")) REQUIRE_AND_GET_MESSAGE_VALUE(options.n_keep, "n_keep", std::stoi);
    if (message.has("n_threads")) REQUIRE_AND_GET_MESSAGE_VALUE(options.n_threads, "n_threads", std::stoi);
    if (message.has("n_predict")) REQUIRE_AND_GET_MESSAGE_VALUE(options.n_predict, "n_predict", std::stoi);
    if (message.has("grp_attn_n")) REQUIRE_AND_GET_MESSAGE_VALUE(options.grp_attn_n, "grp_attn_n", std::stoi);
    if (message.has("grp_attn_w")) REQUIRE_AND_GET_MESSAGE_VALUE(options.grp_attn_w, "grp_attn_w", std::stoi);
    if (message.has("max_tokens")) REQUIRE_AND_GET_MESSAGE_VALUE(options.max_tokens, "max_tokens", std::stoi);
    if (message.has("seed")) REQUIRE_AND_GET_MESSAGE_VALUE(options.seed, "seed", std::stoi);
    if (message.has("temp")) REQUIRE_AND_GET_MESSAGE_VALUE(options.temp, "temp", std::stof);
    if (message.has("top_k")) REQUIRE_AND_GET_MESSAGE_VALUE(options.top_k, "top_k", std::stoi);
    if (message.has("top_p")) REQUIRE_AND_GET_MESSAGE_VALUE(options.top_p, "top_p", std::stof);
    if (message.has("min_p")) REQUIRE_AND_GET_MESSAGE_VALUE(options.min_p, "min_p", std::stof);
    if (message.has("tfs_z")) REQUIRE_AND_GET_MESSAGE_VALUE(options.tfs_z, "tfs_z", std::stof);
    if (message.has("conversation")) options.conversation = message.get("conversation") == "true";
    if (message.has("chatml")) options.chatml = message.get("chatml") == "true";
    if (message.has("instruct")) options.instruct = message.get("instruct") == "true";

    router->bridge->core->ai.createLLM(message.seq, modelId, options, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("ai.llm.destroy", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});
    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "id", std::stoull);
    router->bridge->core->ai.destroyLLM(message.seq, modelId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("ai.llm.stop", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});
    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "id", std::stoull);
    router->bridge->core->ai.stopLLM(message.seq, modelId, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("ai.llm.chat", [](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id", "message"});

    uint64_t modelId = 0;
    REQUIRE_AND_GET_MESSAGE_VALUE(modelId, "id", std::stoull);

    auto value = message.get("message");
    router->bridge->core->ai.chatLLM(message.seq, modelId, value, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Attemps to exit the application
   * @param value The exit code
   */
  router->map("application.exit", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    const auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    int exitCode;
    REQUIRE_AND_GET_MESSAGE_VALUE(exitCode, "value", std::stoi);

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (app->wasLaunchedFromCli) {
      debug("__EXIT_SIGNAL__=%d", exitCode);
      CLI::notify();
    }
  #endif

    const auto window = app->windowManager.getWindow(0);

    if (window == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    window->exit(exitCode);

    reply(Result::Data { message, JSON::Object {} });
  });

  /**
   * Get the screen size available to the application
   */
  router->map("application.getScreenSize", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    const auto window = app->windowManager.getWindow(0);

    if (window == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    const auto screenSize = window->getScreenSize();
    const JSON::Object json = JSON::Object::Entries {
      { "width", screenSize.width },
      { "height", screenSize.height }
    };

    reply(Result::Data { message, json });
  });

  /**
   * Get all active application windows
   * @param value - A list of window indexes to filter on
   */
  router->map("application.getWindows", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    const auto window = app->windowManager.getWindow(0);

    if (window == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    const auto requested = split(message.value, ',');
    Vector<int> indices;

    if (requested.size() == 0) {
      for (const auto& window : app->windowManager.windows) {
        if (window != nullptr) {
          indices.push_back(window->index);
        }
      }
    } else {
      for (const auto& value : requested) {
        try {
          indices.push_back(std::stoi(value));
        } catch (...) {
          return reply(Result::Err { message, "Invalid index given" });
        }
      }
    }

    const auto json  = app->windowManager.json(indices);
    reply(Result::Data { message, json });
  });

  /**
   * Set the application tray menu
   * @param value - The DSL for the system tray menu
   */
  router->map("application.setTrayMenu", [=](auto message, auto router, auto reply) {
  #if SOCKET_RUNTIME_PLATFORM_DESKTOP
    const auto app = App::sharedApplication();
    const auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    app->dispatch([=]() {
      const auto window = app->windowManager.getWindow(0);

      if (window == nullptr) {
        return reply(Result::Err { message, "Application is invalid state" });
      }

      window->setTrayMenu(message.value);
      reply(Result::Data { message, JSON::Object {} });
    });
  #else
    reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"type", "NotSupportedError"},
        {"message", "Application Tray Menu is not supported"}
      }
    });
  #endif
  });

  /**
   * Set the application system menu
   * @param value - The DSL for the system tray menu
   */
  router->map("application.setSystemMenu", [=](auto message, auto router, auto reply) {
  #if SOCKET_RUNTIME_PLATFORM_DESKTOP
    const auto app = App::sharedApplication();
    const auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    app->dispatch([=]() {
      const auto window = app->windowManager.getWindow(0);

      if (window == nullptr) {
        return reply(Result::Err { message, "Application is invalid state" });
      }

      window->setSystemMenu(message.value);
      reply(Result::Data { message, JSON::Object {} });
    });
  #else
    reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"type", "NotSupportedError"},
        {"message", "Application System Menu is not supported"}
      }
    });
  #endif
  });

  /**
   * Set the application system menu item enabled state
   * @param value - The DSL for the system tray menu
   * @param enabled - true or false
   * @param indexMain
   * @param indexSub
   */
  router->map("application.setSystemMenuItemEnabled", [=](auto message, auto router, auto reply) {
  #if SOCKET_RUNTIME_PLATFORM_DESKTOP
    const auto app = App::sharedApplication();
    const auto err = validateMessageParameters(message, {"value", "enabled", "indexMain", "indexSub"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    app->dispatch([=]() {
      const auto window = app->windowManager.getWindow(0);

      if (window == nullptr) {
        return reply(Result::Err { message, "Application is invalid state" });
      }

      const auto enabled = message.get("enabled") == "true";
      int indexMain;
      int indexSub;

      REQUIRE_AND_GET_MESSAGE_VALUE(indexMain, "indexMain", std::stoi);
      REQUIRE_AND_GET_MESSAGE_VALUE(indexSub, "indexSub", std::stoi);

      window->setSystemMenuItemEnabled(enabled, indexMain, indexSub);

      reply(Result::Data { message, JSON::Object {} });
    });
  #else
    reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"type", "NotSupportedError"},
        {"message", "Application System Menu is not supported"}
      }
    });
  #endif
  });

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

    auto bytes = message.buffer.bytes.get();
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
   * Kills an already spawned child process.
   *
   * @param id
   * @param signal
   */
  router->map("child_process.kill", [=](auto message, auto router, auto reply) {
  #if SOCKET_RUNTIME_PLATFORM_IOS
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

    router->bridge->core->childProcess.kill(
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
    #if SOCKET_RUNTIME_PLATFORM_IOS
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

      SSC::Vector<SSC::String> env{};

      if (message.has("env")) {
        env = split(message.get("env"), 0x0001);
      }

      const auto options = Core::ChildProcess::SpawnOptions {
        .cwd = message.get("cwd", getcwd()),
        .env = env,
        .allowStdin = message.get("stdin") != "false",
        .allowStdout = message.get("stdout") != "false",
        .allowStderr = message.get("stderr") != "false"
      };

      router->bridge->core->childProcess.spawn(
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
    #if SOCKET_RUNTIME_PLATFORM_IOS
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

      SSC::Vector<SSC::String> env{};

      if (message.has("env")) {
        env = split(message.get("env"), 0x0001);
      }

      const auto options = Core::ChildProcess::ExecOptions {
        .cwd = message.get("cwd", getcwd()),
        .env = env,
        .allowStdout = message.get("stdout") != "false",
        .allowStderr = message.get("stderr") != "false",
        .timeout = timeout,
        .killSignal = killSignal
      };

      router->bridge->core->childProcess.exec(
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
    #if SOCKET_RUNTIME_PLATFORM_IOS
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

      router->bridge->core->childProcess.write(
        message.seq,
        id,
        message.buffer.bytes,
        message.buffer.size,
        RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
      );
    #endif
  });

  /**
   * Query diagnostics information about the runtime core.
   */
  router->map("diagnostics.query", [=](auto message, auto router, auto reply) {
    router->bridge->core->diagnostics.query(
      message.seq,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
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

    router->bridge->core->dns.lookup(
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
      #if SOCKET_RUNTIME_PLATFORM_WINDOWS
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
      #if SOCKET_RUNTIME_PLATFORM_WINDOWS
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

    router->bridge->core->fs.access(
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
    router->bridge->core->fs.constants(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
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

    router->bridge->core->fs.chmod(
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

    router->bridge->core->fs.chown(
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

    router->bridge->core->fs.lchown(
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

    router->bridge->core->fs.close(message.seq, id, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
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

    router->bridge->core->fs.closedir(message.seq, id, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
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

    router->bridge->core->fs.closeOpenDescriptor(
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
    router->bridge->core->fs.closeOpenDescriptor(
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

    router->bridge->core->fs.copyFile(
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

    router->bridge->core->fs.link(
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

    router->bridge->core->fs.symlink(
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

    router->bridge->core->fs.fstat(message.seq, id, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
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

    router->bridge->core->fs.fsync(
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

    router->bridge->core->fs.ftruncate(
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
    router->bridge->core->fs.getOpenDescriptors(
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

    router->bridge->core->fs.lstat(
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

    router->bridge->core->fs.mkdir(
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

    router->bridge->core->fs.open(
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

    router->bridge->core->fs.opendir(
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

    router->bridge->core->fs.read(
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

    router->bridge->core->fs.readdir(
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

    router->bridge->core->fs.readlink(
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

    router->bridge->core->fs.realpath(
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

    router->bridge->core->fs.retainOpenDescriptor(
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

    router->bridge->core->fs.rename(
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

    router->bridge->core->fs.rmdir(
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

    router->bridge->core->fs.stat(
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

    router->bridge->core->fs.watch(
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

    router->bridge->core->fs.unlink(
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

    router->bridge->core->fs.watch(
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

    router->bridge->core->fs.write(
      message.seq,
      id,
      message.buffer.bytes,
      message.buffer.size,
      offset,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  router->map("geolocation.getCurrentPosition", [=](auto message, auto router, auto reply) {
    router->bridge->core->geolocation.getCurrentPosition(
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

    router->bridge->core->geolocation.watchPosition(
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
    router->bridge->core->geolocation.clearWatch(
      message.seq,
      id,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );

    reply(Result { message.seq, message, JSON::Object{} });
  });

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
   * A private API for starting the Runtime `Core::Conduit`, if it isn't running.
   */
  router->map("internal.conduit.start", [=](auto message, auto router, auto reply) {
    router->bridge->core->conduit.start([=]() {
      if (router->bridge->core->conduit.isActive()) {
        reply(Result::Data {
          message,
          JSON::Object::Entries {
            {"isActive", true},
            {"port", router->bridge->core->conduit.port.load()}
          }
        });
      } else {
        const auto err = JSON::Object::Entries {{ "message", "Failed to start Conduit"}};
        reply(Result::Err { message, err });
      }
    });
  });

  /**
   * A private API for stopping the Runtime `Core::Conduit`, if it is running.
   */
  router->map("internal.conduit.stop", [=](auto message, auto router, auto reply) {
    router->bridge->core->conduit.stop();
    reply(Result { message.seq, message, JSON::Object{} });
  });

  /**
   * A private API for getting the status of the Runtime `Core::Conduit.
   */
  router->map("internal.conduit.status", [=](auto message, auto router, auto reply) {
    reply(Result::Data {
      message,
      JSON::Object::Entries {
        {"isActive", router->bridge->core->conduit.isActive()},
        {"port", router->bridge->core->conduit.port.load()}
      }
    });
  });

  /**
   * Log `value to stdout` with platform dependent logger.
   * @param value
   */
  router->map("log", [=](auto message, auto router, auto reply) {
    auto value = message.value.c_str();
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      NSLog(@"%s", value);
      os_log_with_type(SOCKET_RUNTIME_OS_LOG_BUNDLE, OS_LOG_TYPE_INFO, "%{public}s", value);
    #elif SOCKET_RUNTIME_PLATFORM_ANDROID
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

    const auto options = Core::Notifications::ShowOptions {
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


    const auto notification = Core::Notifications::Notification { message.get("id") };
    router->bridge->core->notifications.close(notification);

    reply(Result { message.seq, message, JSON::Object::Entries {
      {"id", notification.identifier}
    }});
  });

  router->map("notification.list", [=](auto message, auto router, auto reply) {
    router->bridge->core->notifications.list([=](const auto notifications) {
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

    router->bridge->core->os.bufferSize(
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
    router->bridge->core->os.constants(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Returns a mapping of network interfaces.
   */
  router->map("os.networkInterfaces", [=](auto message, auto router, auto reply) {
    router->bridge->core->os.networkInterfaces(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  /**
   * Returns an array of CPUs available to the process.
   */
  router->map("os.cpus", [=](auto message, auto router, auto reply) {
    router->bridge->core->os.cpus(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.rusage", [=](auto message, auto router, auto reply) {
    router->bridge->core->os.rusage(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.uptime", [=](auto message, auto router, auto reply) {
    router->bridge->core->os.uptime(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.uname", [=](auto message, auto router, auto reply) {
    router->bridge->core->os.uname(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.hrtime", [=](auto message, auto router, auto reply) {
    router->bridge->core->os.hrtime(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.availableMemory", [=](auto message, auto router, auto reply) {
    router->bridge->core->os.availableMemory(message.seq, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
  });

  router->map("os.paths", [=](auto message, auto router, auto reply) {
    const auto json = FileResource::getWellKnownPaths().json();
    return reply(Result::Data { message, json });
  });

  router->map("permissions.query", [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"name"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    auto name = message.get("name");

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (name == "geolocation") {
      if (router->bridge->core->geolocation.locationObserver.isAuthorized) {
        auto data = JSON::Object::Entries {{"state", "granted"}};
        return reply(Result::Data { message, data });
      } else if (router->bridge->core->geolocation.locationObserver.locationManager) {
        auto authorizationStatus = (
          router->bridge->core->geolocation.locationObserver.locationManager.authorizationStatus
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
  #if SOCKET_RUNTIME_PLATFORM_APPLE
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
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      auto performedActivation = [router->bridge->core->geolocation.locationObserver attemptActivationWithCompletion: ^(BOOL isAuthorized) {
        if (!isAuthorized) {
          auto reason = @("Location observer could not be activated");

          if (!router->bridge->core->geolocation.locationObserver.locationManager) {
            reason = @("Location observer manager is not initialized");
          } else if (!router->bridge->core->geolocation.locationObserver.locationManager.location) {
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
        } else if (router->bridge->core->geolocation.locationObserver.locationManager.authorizationStatus == kCLAuthorizationStatusNotDetermined) {
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
    #if SOCKET_RUNTIME_PLATFORM_APPLE
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
    const auto app = App::sharedApplication();
    const auto window = app->windowManager.getWindowForBridge(router->bridge);

    if (err.type != JSON::Type::Null) {
      return reply(Result { message.seq, message, err });
    }

    if (message.value == "readystatechange") {
      const auto err = validateMessageParameters(message, {"state"});

      if (err.type != JSON::Type::Null) {
        return reply(Result { message.seq, message, err });
      }

      const auto state = message.get("state");

      if (state == "loading") {
        window->readyState = Window::ReadyState::Loading;
      } else if (state == "interactive") {
        window->readyState = Window::ReadyState::Interactive;
      } else if (state == "complete") {
        window->readyState = Window::ReadyState::Complete;
      }

      window->onReadyStateChange(window->readyState);
    }

    const auto frameType = message.get("runtime-frame-type");
    const auto frameSource = message.get("runtime-frame-source");
    auto userConfig = router->bridge->userConfig;

    if (frameType == "top-level" && frameSource != "serviceworker") {
      if (message.value == "load") {
        const auto href = message.get("location.href");
        if (href.size() > 0) {
          router->bridge->navigator.location.set(href);
          router->bridge->navigator.location.workers.clear();
          auto tmp = href;
          tmp = replace(tmp, "socket://", "");
          tmp = replace(tmp, "https://", "");
          tmp = replace(tmp, userConfig["meta_bundle_identifier"], "");
          const auto parsed = URL::Components::parse(tmp);
          router->bridge->navigator.location.pathname = parsed.pathname;
          router->bridge->navigator.location.query = parsed.query;
        }
      }

      if (router->bridge == router->bridge->navigator.serviceWorker.bridge) {
        if (router->bridge->userConfig["webview_service_worker_mode"] == "hybrid") {
          if (router->bridge->navigator.location.href.size() > 0 && message.value == "beforeruntimeinit") {
            router->bridge->navigator.serviceWorker.reset();
            router->bridge->navigator.serviceWorker.isReady = false;
          } else if (message.value == "runtimeinit") {
            router->bridge->navigator.serviceWorker.isReady = true;
          }
        }
      }
    }

    if (message.value == "load" && frameType == "worker") {
      const auto workerLocation = message.get("runtime-worker-location");
      const auto href = message.get("location.href");
      if (href.size() > 0 && workerLocation.size() > 0) {
        router->bridge->navigator.location.workers[href] = workerLocation;
      }
    }

    router->bridge->core->platform.event(
      message.seq,
      message.value,
      message.get("data"),
      frameType,
      frameSource,
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

    router->bridge->core->platform.notify(
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

    router->bridge->core->platform.revealFile(
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
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"value"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    if (applicationProtocol.size() > 0 && message.value.starts_with(applicationProtocol + ":")) {
      SSC::JSON::Object json = SSC::JSON::Object::Entries {
        { "url", message.value }
      };

      const auto window = app->windowManager.getWindowForBridge(router->bridge);

      if (window) {
        window->handleApplicationURL(message.value);
      }

      reply(Result {
        message.seq,
        message,
        SSC::JSON::Object::Entries {
          {"data", json}
        }
      });
      return;
    }

    router->bridge->core->platform.openExternal(
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
        {"uv", JSON::Object::Entries {
          {"version", uv_version_string()}
        }},
        {"llama", JSON::Object::Entries {
          {"version", String("0.0.") + std::to_string(LLAMA_BUILD_NUMBER)}
        }},
        {"host-operating-system",
        #if SOCKET_RUNTIME_PLATFORM_APPLE
          #if SOCKET_RUNTIME_PLATFORM_IOS_SIMULATOR
             "iphonesimulator"
          #elif SOCKET_RUNTIME_PLATFORM_IOS
            "iphoneos"
          #else
             "macosx"
          #endif
        #elif SOCKET_RUNTIME_PLATFORM_ANDROID
             (router->bridge->isAndroidEmulator ? "android-emulator" : "android")
        #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
             "win32"
        #elif SOCKET_RUNTIME_PLATFORM_LINUX
             "linux"
        #elif SOCKET_RUNTIME_PLATFORM_UNIX
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
  router->map("post", false, [=](auto message, auto router, auto reply) {
    auto err = validateMessageParameters(message, {"id"});

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    uint64_t id;
    REQUIRE_AND_GET_MESSAGE_VALUE(id, "id", std::stoull);

    if (!router->bridge->core->hasPost(id)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotFoundError"},
        {"message", "A 'Post' was not found for the given 'id' in parameters"}
      }});
    }

    auto result = Result { message.seq, message };
    result.post = router->bridge->core->getPost(id);
    if (result.post.headers.size() > 0) {
      const auto lines = split(trim(result.post.headers), '\n');
      for (const auto& line : lines) {
        const auto pair = split(trim(line), ':');
        const auto key = trim(pair[0]);
        const auto value = trim(pair[1]);
        result.headers.set(key, value);
      }
    }
    reply(result);
    router->bridge->core->removePost(id);
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

    if (data.size() > 0 && router->bridge->navigator.serviceWorker.protocols.hasHandler(scheme)) {
      router->bridge->navigator.serviceWorker.protocols.setHandlerData(scheme, { data });
    } else {
      router->bridge->navigator.serviceWorker.protocols.registerHandler(scheme, { data });
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

    if (!router->bridge->navigator.serviceWorker.protocols.hasHandler(scheme)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Protocol handler scheme is not registered."}
      }});
    }

    router->bridge->navigator.serviceWorker.protocols.unregisterHandler(scheme);

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

    if (!router->bridge->navigator.serviceWorker.protocols.hasHandler(scheme)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Protocol handler scheme is not registered."}
      }});
    }

    const auto data = router->bridge->navigator.serviceWorker.protocols.getHandlerData(scheme);

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

    if (!router->bridge->navigator.serviceWorker.protocols.hasHandler(scheme)) {
      return reply(Result::Err { message, JSON::Object::Entries {
        {"message", "Protocol handler scheme is not registered."}
      }});
    }

    router->bridge->navigator.serviceWorker.protocols.setHandlerData(scheme, { data });

    reply(Result { message.seq, message });
  });

  /**
   * Prints incoming message value to stdout.
   * @param value
   */
  router->map("stdout", [=](auto message, auto router, auto reply) {
    if (message.value.size() > 0) {
      #if SOCKET_RUNTIME_PLATFORM_APPLE
        int seq = ++router->bridge->core->logSeq;
        auto msg = String(std::to_string(seq) + "::::" + message.value.c_str());
        os_log_with_type(SOCKET_RUNTIME_OS_LOG_BUNDLE, OS_LOG_TYPE_INFO, "%{public}s", msg.c_str());

        if (Env::get("SSC_LOG_SOCKET").size() > 0) {
          Core::UDP::SendOptions options;
          options.size = 2;
          options.bytes = SharedPointer<char[]>(new char[3]{ '+', 'N', '\0' });
          options.address = "0.0.0.0";
          options.port = std::stoi(Env::get("SSC_LOG_SOCKET"));
          options.ephemeral = true;
          router->bridge->core->udp.send("-1", 0, options, [](auto seq, auto json, auto post) {});
        }
      #endif
      IO::write(message.value, false);
    } else if (message.buffer.bytes != nullptr && message.buffer.size > 0) {
      IO::write(String(message.buffer.bytes.get(), message.buffer.size), false);
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
      #if SOCKET_RUNTIME_PLATFORM_APPLE
        int seq = ++router->bridge->core->logSeq;
        auto msg = String(std::to_string(seq) + "::::" + message.value.c_str());
        os_log_with_type(SOCKET_RUNTIME_OS_LOG_BUNDLE, OS_LOG_TYPE_ERROR, "%{public}s", msg.c_str());

        if (Env::get("SSC_LOG_SOCKET").size() > 0) {
          Core::UDP::SendOptions options;
          options.size = 2;
          options.bytes = SharedPointer<char[]>(new char[3]{ '+', 'N', '\0' });
          options.address = "0.0.0.0";
          options.port = std::stoi(Env::get("SSC_LOG_SOCKET"));
          options.ephemeral = true;
          router->bridge->core->udp.send("-1", 0, options, [](auto seq, auto json, auto post) {});
        }
      #endif
      IO::write(message.value, true);
    } else if (message.buffer.bytes != nullptr && message.buffer.size > 0) {
      IO::write(String(message.buffer.bytes.get(), message.buffer.size), true);
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

    const auto registration = router->bridge->navigator.serviceWorker.registerServiceWorker(options);
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
    router->bridge->navigator.serviceWorker.reset();
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
    router->bridge->navigator.serviceWorker.unregisterServiceWorker(scope);

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

    for (const auto& entry : router->bridge->navigator.serviceWorker.registrations) {
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
    for (const auto& entry : router->bridge->navigator.serviceWorker.registrations) {
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

    router->bridge->navigator.serviceWorker.skipWaiting(id);

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
      router->bridge->navigator.location.workers[workerURL] = scriptURL;
    }

    router->bridge->navigator.serviceWorker.updateState(id, message.get("state"));
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

    for (auto& entry : router->bridge->navigator.serviceWorker.registrations) {
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

    for (auto& entry : router->bridge->navigator.serviceWorker.registrations) {
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

    for (auto& entry : router->bridge->navigator.serviceWorker.registrations) {
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

    for (auto& entry : router->bridge->navigator.serviceWorker.registrations) {
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

    for (auto& entry : router->bridge->navigator.serviceWorker.registrations) {
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
    const Core::Timers::ID id = router->bridge->core->timers.setTimeout(timeout, [=]() {
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
    router->bridge->core->timers.clearTimeout(id);

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

    router->bridge->core->udp.bind(
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

    router->bridge->core->udp.close(message.seq, id, RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply));
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

    router->bridge->core->udp.connect(
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

    router->bridge->core->udp.disconnect(
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

    router->bridge->core->udp.getPeerName(
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

    router->bridge->core->udp.getSockName(
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

    router->bridge->core->udp.getState(
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

    router->bridge->core->udp.readStart(
      message.seq,
      id,
      [&, id, router, message, reply](auto seq, auto json, auto post) {
        if (seq == "-1" && router->bridge->core->conduit.has(id)) {
          auto data = json["data"];

          CoreConduit::Options options = {
            { "port", data["port"].str() },
            { "address", data["address"].template as<JSON::String>().data }
          };

          auto client = router->bridge->core->conduit.get(id);
          client->emit(options, post.body, post.length);
          return;
        }

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

    router->bridge->core->udp.readStop(
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
    options.ephemeral = message.get("ephemeral") == "true";
    options.address = message.get("address", "0.0.0.0");
    options.bytes = message.buffer.bytes;

    router->bridge->core->udp.send(
      message.seq,
      id,
      options,
      RESULT_CALLBACK_FROM_CORE_CALLBACK(message, reply)
    );
  });

  /**
   * Show the file system picker dialog
   * @param allowMultiple
   * @param allowFiles
   * @param allowDirs
   * @param type
   * @param contentTypeSpecs
   * @param defaultName
   * @param defaultPath
   * @param title
   */
  router->map("window.showFileSystemPicker", [=](auto message, auto router, auto reply) {
    const auto allowMultiple = message.get("allowMultiple") == "true";
    const auto allowFiles = message.get("allowFiles") == "true";
    const auto allowDirs = message.get("allowDirs") == "true";
    const auto isSave = message.get("type") == "save";

    const auto contentTypeSpecs = message.get("contentTypeSpecs");
    const auto defaultName = message.get("defaultName");
    const auto defaultPath = message.get("defaultPath");
    const auto title = message.get("title", isSave ? "Save" : "Open");
    const auto app = App::sharedApplication();
    const auto window = app->windowManager.getWindowForBridge(router->bridge);

    app->dispatch([=]() {
      Dialog* dialog = nullptr;

      if (window) {
        dialog = &window->dialog;
      } else {
        dialog = new Dialog();
      }

      const auto options = Dialog::FileSystemPickerOptions {
        .prefersDarkMode = message.get("prefersDarkMode") == "true",
        .directories = allowDirs,
        .multiple = allowMultiple,
        .files = allowFiles,
        .contentTypes = contentTypeSpecs,
        .defaultName = defaultName,
        .defaultPath = defaultPath,
        .title = title
      };

      const auto callback = [=](Vector<String> results) {
        JSON::Array paths;

        if (results.size() == 0) {
          const auto err = JSON::Object::Entries {{"type", "AbortError"}};
          return reply(Result::Err { message, err });
        }

        for (const auto& result : results) {
          paths.push(result);
        }

        const auto data = JSON::Object::Entries {
          {"paths", paths}
        };

        reply(Result::Data { message, data });
      };

      if (isSave) {
        if (!dialog->showSaveFilePicker(options, callback)) {
          const auto err = JSON::Object::Entries {{"type", "AbortError"}};
          reply(Result::Err { message, err });
        }
      } else {
        const auto result = (
          allowFiles && !allowDirs
            ? dialog->showOpenFilePicker(options, callback)
            : dialog->showDirectoryPicker(options, callback)
        );

        if (!result) {
          const auto err = JSON::Object::Entries {{"type", "AbortError"}};
          reply(Result::Err { message, err });
        }
      }

      if (!window) {
        delete dialog;
      }
    });
  });

  /**
   * Closes a target window
   * @param targetWindowIndex
   */
  router->map("window.close", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);

    if (!window) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    reply(Result::Data { message, window->json() });

    app->core->setTimeout(16, [=] () {
      app->windowManager.destroyWindow(targetWindowIndex);
    });
  });

  /**
   * Creates a new window
   * @param url
   * @param title
   * @param shouldExitApplicationOnClose
   * @param headless
   * @param radius
   * @param margin
   * @param height
   * @param width
   * @param minWidth
   * @param minHeight
   * @param maxWidth
   * @param maxHeight
   * @param resizable
   * @param frameless
   * @param closable
   * @param maximizable
   * @param minimizable
   * @param aspectRatio
   * @param titlebarStyle
   * @param windowControlOffsets
   * @param backgroundColorLight
   * @param backgroundColorDark
   * @param utility
   * @param userScript
   * @param userConfig
   * @param targetWindowIndex
   */
  router->map("window.create", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    if (
      targetWindowIndex >= SOCKET_RUNTIME_MAX_WINDOWS &&
      message.get("headless") != "true" &&
      message.get("debug") != "true"
    ) {
      static const auto maxWindows = std::to_string(SOCKET_RUNTIME_MAX_WINDOWS);
      return reply(Result::Err {
        message,
        "Cannot create widow with an index beyond " + maxWindows
      });
    }

    app->dispatch([=]() {
      if (
        app->windowManager.getWindow(targetWindowIndex) != nullptr &&
        app->windowManager.getWindowStatus(targetWindowIndex) != WindowManager::WindowStatus::WINDOW_NONE
      ) {
        return reply(Result::Err {
          message,
          "Window with index " + message.get("targetWindowIndex") + " already exists"
        });
      }

      const auto window = app->windowManager.getWindow(0);
      const auto screen = window->getScreenSize();
      auto options = Window::Options {};

      options.shouldExitApplicationOnClose = message.get("shouldExitApplicationOnClose") == "true" ? true : false;
      options.headless = app->userConfig["build_headless"] == "true";

      if (message.get("headless") == "true") {
        options.headless = true;
      } else if (message.get("headless") == "false") {
        options.headless = false;
      }

      try {
        if (message.has("radius")) {
          options.radius = std::stof(message.get("radius"));
        }
      } catch (...) {}

      try {
        if (message.has("margin")) {
          options.margin = std::stof(message.get("margin"));
        }
      } catch (...) {}

      options.width = message.get("width").size()
        ? window->getSizeInPixels(message.get("width"), screen.width)
        : 0;

      options.height = message.get("height").size()
        ? window->getSizeInPixels(message.get("height"), screen.height)
        : 0;

      options.minWidth = message.get("minWidth").size()
        ? window->getSizeInPixels(message.get("minWidth"), screen.width)
        : 0;

      options.minHeight = message.get("minHeight").size()
        ? window->getSizeInPixels(message.get("minHeight"), screen.height)
        : 0;

      options.maxWidth = message.get("maxWidth").size()
        ? window->getSizeInPixels(message.get("maxWidth"), screen.width)
        : screen.width;

      options.maxHeight = message.get("maxHeight").size()
        ? window->getSizeInPixels(message.get("maxHeight"), screen.height)
        : screen.height;

      options.resizable = message.get("resizable") == "true" ? true : false;
      options.frameless = message.get("frameless") == "true" ? true : false;
      options.closable = message.get("closable") == "true" ? true : false;
      options.maximizable = message.get("maximizable") == "true" ? true : false;
      options.minimizable = message.get("minimizable") == "true" ? true : false;
      options.aspectRatio = message.get("aspectRatio");
      options.titlebarStyle = message.get("titlebarStyle");
      options.windowControlOffsets = message.get("windowControlOffsets");
      options.backgroundColorLight = message.get("backgroundColorLight");
      options.backgroundColorDark = message.get("backgroundColorDark");
      options.utility = message.get("utility") == "true" ? true : false;
      options.debug = message.get("debug") == "true" ? true : false;
      options.userScript = message.get("userScript");
      options.index = targetWindowIndex;
      options.RUNTIME_PRIMORDIAL_OVERRIDES = message.get("__runtime_primordial_overrides__");
      options.userConfig = INI::parse(message.get("config"));

      if (options.index >= SOCKET_RUNTIME_MAX_WINDOWS) {
        options.features.useGlobalCommonJS = false;
      }

      auto createdWindow = app->windowManager.createWindow(options);

      if (message.has("title")) {
        createdWindow->setTitle(message.get("title"));
      }

      if (message.has("url")) {
        createdWindow->navigate(message.get("url"));
      }

      if (!options.headless) {
        createdWindow->show();
      }

      reply(Result::Data { message, createdWindow->json() });
    });
  });

  /**
   * Gets the background color of a target window window
   * @param targetWindowIndex
   */
  router->map("window.getBackgroundColor", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    app->dispatch([=]() {
      const auto window = app->windowManager.getWindow(targetWindowIndex);
      const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

      if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
        return reply(Result::Err {
          message,
          JSON::Object::Entries {
            {"message", "Target window not found"},
            {"type", "NotFoundError"}
          }
        });
      }

      reply(Result::Data { message, window->getBackgroundColor() });
    });
  });

  /**
   * Gets the title of a target window
   * @param targetWindowIndex
   */
  router->map("window.getTitle", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    app->dispatch([=]() {
      const auto window = app->windowManager.getWindow(targetWindowIndex);
      const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

      if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
        return reply(Result::Err {
          message,
          JSON::Object::Entries {
            {"message", "Target window not found"},
            {"type", "NotFoundError"}
          }
        });
      }

      reply(Result::Data { message, window->getTitle() });
    });
  });

  /**
   * Hides a target window
   * @param targetWindowIndex
   */
  router->map("window.hide", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
      window->hide();
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Maximize a target window
   * @param targetWindowIndex
   */
  router->map("window.maximize", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
    #if SOCKET_RUNTIME_PLATFORM_DESKTOP
      window->maximize();
    #else
      const auto screen = window->getScreenSize();
      window->setSize(screen.height, screen.width);
      window->show();
    #endif
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Minimize a target window
   * @param targetWindowIndex
   */
  router->map("window.minimize", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
    #if SOCKET_RUNTIME_PLATFORM_DESKTOP
      window->minimize();
    #else
      window->hide();
    #endif
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Navigate a targetbnnb
   * @param targetWindowIndex
   */
  router->map("window.navigate", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex", "url"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    const auto requestedURL = message.get("url");
    const auto allowed = window->bridge.navigator.isNavigationRequestAllowed(
      window->bridge.navigator.location.href,
      requestedURL
    );

    if (!allowed) {
      return reply(Result::Err { message, "Navigation to URL is not allowed" });
    }

    app->dispatch([=]() {
      window->bridge.navigator.location.set(requestedURL);
      window->bridge.navigate(requestedURL);
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Restore a target window
   * @param targetWindowIndex
   */
  router->map("window.restore", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
    #if SOCKET_RUNTIME_PLATFORM_DESKTOP
      window->restore();
    #else
      window->show();
    #endif
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Send an event to another window
   * @param event
   * @param value
   * @param targetWindowIndex
   */
  router->map("window.send", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    const auto event = message.get("event");
    const auto value = message.get("value");
    const auto targetWindowIndex = message.get("targetWindowIndex").size() >= 0 ? std::stoi(message.get("targetWindowIndex")) : -1;

    if (targetWindowIndex < 0) {
      return reply(Result::Err { message, "Invalid target window index" });
    }

    const auto targetWindow = app->windowManager.getWindow(targetWindowIndex);

    if (!targetWindow) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
      debug("eval: %s", getEmitToRenderProcessJavaScript(event, value).c_str());
      targetWindow->eval(getEmitToRenderProcessJavaScript(event, value));
      reply(Result { message.seq, message });
    });
  });

  /**
   * Sets the background color
   * @param targetWindowIndex
   * @param red
   * @param green
   * @param blue
   * @param alpha
   *
   */
  router->map("window.setBackgroundColor", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;
    int red = 0;
    int green = 0;
    int blue = 0;
    float alpha = 1;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    if (message.has("red")) {
      REQUIRE_AND_GET_MESSAGE_VALUE(red, "red", std::stoi);
    }

    if (message.has("green")) {
      REQUIRE_AND_GET_MESSAGE_VALUE(green, "green", std::stoi);
    }

    if (message.has("blue")) {
      REQUIRE_AND_GET_MESSAGE_VALUE(blue, "blue", std::stoi);
    }

    if (message.has("alpha")) {
      REQUIRE_AND_GET_MESSAGE_VALUE(alpha, "alpha", std::stof);
    }

    if (alpha > 1 || alpha < 0) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Invalid 'alpha' parameter given"},
          {"type", "RangeError"}
        }
      });
    }

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
      window->setBackgroundColor(red, green, blue, alpha);
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Creates and displays a context menu at the current mouse position (desktop only)
   * @param value
   */
  router->map("window.setContextMenu", [=](auto message, auto router, auto reply) {
  #if SOCKET_RUNTIME_PLATFORM_DESKTOP
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"index", "value"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    const auto window = app->windowManager.getWindow(message.index);
    const auto windowStatus = app->windowManager.getWindowStatus(message.index);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
      window->setContextMenu(message.seq, message.value);
      reply(Result::Data { message, JSON::Object {} });
    });
  #else
    reply(Result::Err {
      message,
      JSON::Object::Entries {
        {"type", "NotSupportedError"},
        {"message", "Setting a window context menu is not supported"}
      }
    });
  #endif
  });

  /**
   * Sets the position  of a target window
   * @param targetWindowIndex
   * @param height
   * @param width
   */
  router->map("window.setPosition", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex", "x", "y"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    const auto screen = window->getScreenSize();
    const auto x = Window::getSizeInPixels(message.get("x"), screen.width);
    const auto y = Window::getSizeInPixels(message.get("y"), screen.height);

    app->dispatch([=]() {
      window->setPosition(x, y);
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Sets the size of a target window (desktop only)
   * @param targetWindowIndex
   * @param height
   * @param width
   */
  router->map("window.setSize", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex", "height", "width"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    const auto screen = window->getScreenSize();
    const auto width = window->getSizeInPixels(message.get("width"), screen.width);
    const auto height = window->getSizeInPixels(message.get("height"), screen.height);

    app->dispatch([=]() {
      window->setSize(width, height, 0);
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Sets the title of a target windo
   * @param targetWindowIndex
   * @param value
   */
  router->map("window.setTitle", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex", "value"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
      window->setTitle(message.value);
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Shows a target window
   * @param targetWindowIndex
   */
  router->map("window.show", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=]() {
      window->show();
      reply(Result::Data { message, window->json() });
    });
  });

  /**
   * Shows the target window web inspector (desktop only)
   * @param targetWindowIndex
   */
  router->map("window.showInspector", [=](auto message, auto router, auto reply) {
    const auto app = App::sharedApplication();
    auto err = validateMessageParameters(message, {"targetWindowIndex"});

    if (app == nullptr) {
      return reply(Result::Err { message, "Application is invalid state" });
    }

    if (err.type != JSON::Type::Null) {
      return reply(Result::Err { message, err });
    }

    int targetWindowIndex;

    REQUIRE_AND_GET_MESSAGE_VALUE(targetWindowIndex, "targetWindowIndex", std::stoi);

    const auto window = app->windowManager.getWindow(targetWindowIndex);
    const auto windowStatus = app->windowManager.getWindowStatus(targetWindowIndex);

    if (!window || windowStatus == WindowManager::WindowStatus::WINDOW_NONE) {
      return reply(Result::Err {
        message,
        JSON::Object::Entries {
          {"message", "Target window not found"},
          {"type", "NotFoundError"}
        }
      });
    }

    app->dispatch([=] () {
      window->showInspector();
      reply(Result::Data { message, window->json() });
    });
  });
}

namespace SSC::IPC {
  void Router::mapRoutes () {
    mapIPCRoutes(this);
  }
}
