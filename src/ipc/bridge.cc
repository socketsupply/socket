#include "../core/core.hh"
#include "ipc.hh"

#define IPC_CONTENT_TYPE "application/octet-stream"

using namespace SSC;
using namespace SSC::IPC;

static void registerSchemeHandler (Router *router) {
#if defined(__linux__)
  static std::atomic<bool> registered = false;
  if (registered) return;
  registered = true;
  auto ctx = webkit_web_context_get_default();
  webkit_web_context_register_uri_scheme(ctx, "ipc", [](auto request, auto ptr){
    auto uri = String(webkit_uri_scheme_request_get_uri(request));
    auto router = reinterpret_cast<Router *>(ptr);
    auto message = Message{uri};
    auto invoked = router->invoke(message, [=](auto result) {
      auto json = result.str();
      auto size = result.post.body != nullptr ? result.post.length : json.size();
      auto body = result.post.body != nullptr ? result.post.body : json.c_str();

      auto freeFunction = result.post.body != nullptr ? free : nullptr;
      auto stream = g_memory_input_stream_new_from_data(body, size, freeFunction);
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
      auto stream = g_memory_input_stream_new_from_data(msg.c_str(), msg.size(), 0);
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
  router->map("buffer.map", [](auto message, auto router, auto reply) {
    if (!message.has("seq")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'seq' in parameters"}
      }});
    }

    if (message.body.bytes != nullptr) {
      auto key = std::to_string(message.index) + message.seq;
      auto str = String();
      str.assign(message.body.bytes, message.body.size);
      router->buffers[key] = str;
      delete message.body.bytes;
      message.body.bytes = str.data();
    }
  });

  router->map("dns.lookup", [](auto message, auto router, auto reply) {
    auto hostname = message.get("hostname");
    auto family = std::stoi(message.get("family", "0"));
    auto seq = message.seq;

    // TODO: support these options
    // auto hints = std::stoi(message.get("hints"));
    // auto all = bool(std::stoi(message.get("all")));
    // auto verbatim = bool(std::stoi(message.get("verbatim")));
    router->core->dns.lookup(seq, hostname, family, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json, post });
    });
  });

  router->map("fs.constants", [](auto message, auto router, auto reply) {
    router->core->fs.constants(message.seq, [=](auto seq, auto json, auto post) {
      auto result = Result { message.seq, message };
      result.value = json;
      reply(result);
    });
  });

  router->map("os.bufferSize", [](auto message, auto router, auto reply) {
    if (message.get("id").size() == 0) {
      auto result = Result { message.seq, message };
      result.err = JSON::Object::Entries {
        {"type", "InternalError"},
        {"message", ".id is required"}
      };
      reply(result);
      return;
    }

    auto buffer = std::stoi(message.get("buffer", "0"));
    auto size = std::stoi(message.get("size", "0"));
    auto id  = std::stoull(message.get("id"));

    router->core->os.bufferSize(message.seq, id, size, buffer, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json , post});
    });
  });

  router->map("os.platform", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = SSC::platform.os;
    reply(result);
  });

  router->map("os.type", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = SSC::platform.os;
    reply(result);
  });

  router->map("os.arch", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = SSC::platform.arch;
    reply(result);
  });

  router->map("os.networkInterfaces", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.value = router->core->getNetworkInterfaces();
    reply(result);
  });

  router->map("ping", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = "pong";
    reply(result);
  });

  router->map("platform.event", [](auto message, auto router, auto reply) {
    auto value = message.value;
    auto data = message.get("data");
    auto seq = message.seq;
    router->core->platform.event(seq, value, data, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json, post });
    });
  });

  router->map("post", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    if (!router->core->hasPost(id)) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"message", "Invalid 'id' for post"}
      }});
    }

    result.post = router->core->getPost(id);
    reply(result);
    //router->core->removePost(id);
  });

  router->map("udp.bind", [](auto message, auto router, auto reply) {
    Core::UDP::BindOptions options;
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    if (!message.has("port")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'port' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { options.port = std::stoi(message.get("port")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'port' given in parameters"}
      }});
    }

    options.reuseAddr = message.get("reuseAddr") == "true";
    options.address = message.get("address", "0.0.0.0");

    router->core->udp.bind(message.seq, id, options, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });

  router->map("udp.close", [](auto message, auto router, auto reply) {
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.close(message.seq, id, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });

  router->map("udp.connect", [](auto message, auto router, auto reply) {
    Core::UDP::ConnectOptions options;
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    if (!message.has("port")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'port' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { options.port = std::stoi(message.get("port")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'port' given in parameters"}
      }});
    }

    options.address = message.get("address", "0.0.0.0");

    router->core->udp.connect(message.seq, id, options, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });

  router->map("udp.disconnect", [](auto message, auto router, auto reply) {
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.disconnect(message.seq, id, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });

  router->map("udp.getPeerName", [](auto message, auto router, auto reply) {
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.getPeerName(message.seq, id, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });

  router->map("udp.getSockName", [](auto message, auto router, auto reply) {
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.getSockName(message.seq, id, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });

  router->map("udp.getState", [](auto message, auto router, auto reply) {
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.getState(message.seq, id, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });

  router->map("udp.readStart", [](auto message, auto router, auto reply) {
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.readStart(message.seq, id,
      [=](auto seq, auto json, auto post) {
        reply(Result { seq, message, json, post });
      },
      [=](auto nread, auto buf, auto addr) {
        if (nread == UV_EOF) {
          auto json = JSON::Object::Entries {
            {"source", "udp.receive"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"EOF", true}
            }}
          };

          reply(Result { "-1", message, json });
        } else if (nread > 0) {
          int port;
          char addressbuf[17];
          parseAddress((struct sockaddr *) addr, &port, addressbuf);
          String address(addressbuf);

          auto headers = SSC::format(R"MSG(
            content-type: application/octet-stream
            content-length: $i
          )MSG", (int) nread);

          auto result = Result { "-1", message };
          result.post.id = rand64();
          result.post.body = buf->base;
          result.post.length = (int) nread;
          result.post.headers = headers;
          result.post.bodyNeedsFree = true;

          result.value = JSON::Object::Entries {
            {"source", "udp.receive"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"port", port},
              {"bytes", std::to_string(result.post.length)},
              {"address", address}
            }}
          };

          reply(result);
        }
      }
    );
  });

  router->map("udp.readStop", [](auto message, auto router, auto reply) {
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    router->core->udp.readStop(message.seq, id, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });

  router->map("udp.send", [](auto message, auto router, auto reply) {
    Core::UDP::SendOptions options;
    uint64_t id;

    if (!message.has("id")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'id' in parameters"}
      }});
    }

    if (!message.has("port")) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Expecting 'port' in parameters"}
      }});
    }

    try { id = std::stoull(message.get("id")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'id' given in parameters"}
      }});
    }

    try { options.port = std::stoi(message.get("port")); } catch (...) {
      return reply(Result::Err { message.seq, message, JSON::Object::Entries {
        {"message", "Invalid 'port' given in parameters"}
      }});
    }

    auto bufferKey = std::to_string(message.index) + message.seq;
    auto buffer = router->buffers[bufferKey];

    options.size = buffer.size();
    options.bytes = buffer.data();
    options.address = message.get("address", "0.0.0.0");
    options.ephemeral = message.get("ephemeral") == "true";

    router->buffers.erase(router->buffers.find(bufferKey));

    router->core->udp.send(message.seq, id, options, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json });
    });
  });
}

namespace SSC::IPC {
  Bridge::Bridge (Core *core) : router(core) {
    this->core = core;
  }

  bool Bridge::route (const String& msg, char *bytes, size_t size) {
    return this->router.invoke(msg, bytes, size);
  }

  Router::Router () {
    initFunctionsTable(this);
    registerSchemeHandler(this);
  }

  Router::Router (Core *core) : Router() {
    this->core = core;
  }

  void Router::map (const String& name, MessageCallback callback) {
    if (callback != nullptr) {
      table.insert_or_assign(name, callback);
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

  bool Router::invoke (const Message message) {
    return this->invoke(message, [this](auto result) {
      this->send(result.seq, result.str(), result.post);
    });
  }

  bool Router::invoke (const Message message, ResultCallback callback) {
    if (this->table.find(message.name) == this->table.end()) {
      return false;
    }

    auto fn = this->table.at(message.name);

    if (fn!= nullptr) {
      return this->dispatch([=, this] {
        fn(message, this, callback);
      });
    }

    return false;
  }

  bool Router::send (const Message::Seq& seq, const String& data) {
    return this->send(seq, data, Post{});
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
      auto value = SSC::encodeURIComponent(data);
      auto script = SSC::getResolveToRenderProcessJavaScript(seq, "0", value);
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
    auto value = SSC::encodeURIComponent(data);
    auto script = SSC::getEmitToRenderProcessJavaScript(name, value);
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

  /*
  struct CallbackContext {
    Callback cb;
    SSC::String seq;
    Window *window;
    void *data;
  };
  static bool XXX (IPC::Message cmd, char *buf, size_t bufsize, Callback cb) {
    auto seq = cmd.get("seq");

    if (cmd.name == "window.eval" && cmd.index >= 0) {
      auto windowFactory = reinterpret_cast<WindowFactory<Window, App> *>(app->getWindowFactory());
      if (windowFactory == nullptr) {
        // @TODO(jwerle): print warning
        return false;
      }

      auto window = windowFactory->getWindow(cmd.index);

      if (window == nullptr) {
        return false;
      }

      auto value = decodeURIComponent(cmd.get("value"));
      auto ctx = new CallbackContext { cb, seq, window, (void *) this };

      webkit_web_view_run_javascript(
        WEBKIT_WEB_VIEW(window->webview),
        value.c_str(),
        nullptr,
        [](GObject *object, GAsyncResult *res, gpointer data) {
          GError *error = nullptr;
          auto ctx = reinterpret_cast<CallbackContext *>(data);
          auto result = webkit_web_view_run_javascript_finish(
            WEBKIT_WEB_VIEW(ctx->window->webview),
            res,
            &error
          );

          if (!result) {
            auto msg = SSC::format(
              R"MSG({"err": { "code": "$S", "message": "$S" } })MSG",
              std::to_string(error->code),
              SSC::String(error->message)
            );

            g_error_free(error);
            ctx->cb(ctx->seq, msg, Post{});
            return;
          } else {
            auto value = webkit_javascript_result_get_js_value(result);

            if (
              jsc_value_is_null(value) ||
              jsc_value_is_array(value) ||
              jsc_value_is_object(value) ||
              jsc_value_is_number(value) ||
              jsc_value_is_string(value) ||
              jsc_value_is_function(value) ||
              jsc_value_is_undefined(value) ||
              jsc_value_is_constructor(value)
            ) {
              auto context = jsc_value_get_context(value);
              auto string = jsc_value_to_string(value);
              auto exception = jsc_context_get_exception(context);
              SSC::String msg = "";

              if (exception) {
                auto message = jsc_exception_get_message(exception);
                msg = SSC::format(
                  R"MSG({"err": { "message": "$S" } })MSG",
                  SSC::String(message)
                );
              } else if (string) {
                msg = SSC::String(string);
              }

              ctx->cb(ctx->seq, msg, Post{});
              if (string) {
                g_free(string);
              }
            } else {
              auto msg = SSC::format(
                R"MSG({"err": { "message": "Error: An unknown JavaScript evaluation error has occurred" } })MSG"
               );

              ctx->cb(ctx->seq, msg, Post{});
            }
          }

          webkit_javascript_result_unref(result);
        },
        ctx
      );

      return true;
    }

    if (cmd.name == "fsAccess" || cmd.name == "fs.access") {
      if (cmd.get("path").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'path' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      if (cmd.get("mode").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'mode' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto path = decodeURIComponent(cmd.get("path"));
        auto mode = std::stoi(cmd.get("mode"));

        this->core->fsAccess(seq, path, mode, cb);
      });
      return true;
    }

    if (cmd.name == "fsChmod" || cmd.name == "fs.chmod") {
      if (cmd.get("path").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'path' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      if (cmd.get("mode").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'mode' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto path = decodeURIComponent(cmd.get("path"));
        auto mode = std::stoi(cmd.get("mode"));

        this->core->fsChmod(seq, path, mode, cb);
      });
      return true;
    }

    if (cmd.name == "fsClose" || cmd.name == "fs.close") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'id' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto id = std::stoull(cmd.get("id"));
        this->core->fsClose(seq, id, cb);
      });
      return true;
    }

    if (cmd.name == "fsClosedir" || cmd.name == "fs.closedir") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'id' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto id = std::stoull(cmd.get("id"));
        this->core->fsClosedir(seq, id, cb);
      });
      return true;
    }

    if (cmd.name == "fsGetOpenDescriptors" || cmd.name == "fs.getOpenDescriptors") {
      this->app->dispatch([=, this] {
        this->core->fsGetOpenDescriptors(seq, cb);
      });
      return true;
    }

    if (cmd.name == "fsCloseOpenDescriptor" || cmd.name == "fs.closeOpenDescriptor") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'id' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto id = std::stoull(cmd.get("id"));
        this->core->fsCloseOpenDescriptor(seq, id, cb);
      });
      return true;
    }

    if (cmd.name == "fsCloseOpenDescriptors" || cmd.name == "fs.closeOpenDescriptors") {
      this->app->dispatch([=, this] {
        auto preserveRetained = cmd.get("retain") != "false";
        this->core->fsCloseOpenDescriptors(seq, preserveRetained, cb);
      });
      return true;
    }

    if (cmd.name == "fsCopyFile" || cmd.name == "fs.copyFile") {
      this->app->dispatch([=, this] {
        auto src = cmd.get("src");
        auto dest = cmd.get("dest");
        auto mode = std::stoi(cmd.get("dest"));

        this->core->fsCopyFile(seq, src, dest, mode, cb);
      });
      return true;
    }

    if (cmd.name == "fsOpen" || cmd.name == "fs.open") {
      this->app->dispatch([=, this] {
        auto seq = cmd.get("seq");
        auto path = decodeURIComponent(cmd.get("path"));
        auto flags = std::stoi(cmd.get("flags"));
        auto mode = std::stoi(cmd.get("mode"));
        auto id = std::stoull(cmd.get("id"));

        this->core->fsOpen(seq, id, path, flags, mode, cb);
      });

      return true;
    }

    if (cmd.name == "fsOpendir" || cmd.name == "fs.opendir") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'id' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      if (cmd.get("path").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'path' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto path = decodeURIComponent(cmd.get("path"));
        auto id = std::stoull(cmd.get("id"));

        this->core->fsOpendir(seq, id, path,  cb);
      });
      return true;
    }

    if (cmd.name == "fsRead" || cmd.name == "fs.read") {
      this->app->dispatch([=, this] {
        auto id = std::stoull(cmd.get("id"));
        auto size = std::stoi(cmd.get("size"));
        auto offset = std::stoi(cmd.get("offset"));

        this->core->fsRead(seq, id, size, offset, cb);
      });
      return true;
    }

    if (cmd.name == "fsRetainOpenDescriptor" || cmd.name == "fs.retainOpenDescriptor") {
      this->app->dispatch([=, this] {
        auto id = std::stoull(cmd.get("id"));

        this->core->fsRetainOpenDescriptor(seq, id, cb);
      });
      return true;
    }

    if (cmd.name == "fsReaddir" || cmd.name == "fs.readdir") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'id' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto id = std::stoull(cmd.get("id"));
        auto entries = std::stoi(cmd.get("entries", "256"));

        this->core->fsReaddir(seq, id, entries, cb);
      });
      return true;
    }

    if (cmd.name == "fsWrite" || cmd.name == "fs.write") {
      auto bufferKey = std::to_string(cmd.index) + seq;
      if (bufferQueue.count(bufferKey)) {
          auto id = std::stoull(cmd.get("id"));
          auto offset = std::stoi(cmd.get("offset"));
          auto buffer = bufferQueue[bufferKey];
          bufferQueue.erase(bufferQueue.find(bufferKey));

        this->app->dispatch([=, this] {
          this->core->fsWrite(seq, id, buffer, offset, cb);
        });
      }

      return true;
    }
*/
