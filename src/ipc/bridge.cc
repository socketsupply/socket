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
  router->map("ping", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = "pong";
    reply(result);
  });

  router->map("event", [](auto message, auto router, auto reply) {
    auto value = message.value;
    auto data = message.get("data");
    auto seq = message.seq;
    router->core->handleEvent(seq, value, data, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json, post });
    });
  });

  router->map("bufferSize", [](auto message, auto router, auto reply) {
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

    router->core->bufferSize(message.seq, id, size, buffer, [=](auto seq, auto json, auto post) {
      auto result = Result { message.seq, message };
      reply(result)
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
  });

  router->map("os.arch", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.data = SSC::platform.arch;
    reply(result);
  });

  router->map("os.networkInterfaces", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.json = router->core->getNetworkInterfaces();
    reply(result);
  });

  router->map("dns.lookup", [](auto message, auto router, auto reply) {
    auto hostname = message.get("hostname");
    auto family = std::stoi(message.get("family", "0"));
    auto seq = message.seq;

    // TODO: support these options
    // auto hints = std::stoi(message.get("hints"));
    // auto all = bool(std::stoi(message.get("all")));
    // auto verbatim = bool(std::stoi(message.get("verbatim")));
    router->core->dnsLookup(seq, hostname, family, [=](auto seq, auto json, auto post) {
      reply(Result { seq, message, json, post });
    });
  });

  router->map("fs.constants", [](auto message, auto router, auto reply) {
    auto result = Result { message.seq, message };
    result.json = router->core->getFSConstants();
    reply(result);
  });
}

namespace SSC::IPC {
  Bridge::Bridge (Core *core) : router(core) {
    this->core = core;
  }

  bool Bridge::route (const String msg, char *bytes, size_t size) {
    return this->router.invoke(msg, bytes, size);
  }

  Router::Router () {
    initFunctionsTable(this);
    registerSchemeHandler(this);
    fprintf(stderr, "INIT ROUTER\n");
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

  bool Router::invoke (const String name, char *bytes, size_t size) {
    auto message = Message { name, bytes, size };
    return this->invoke(message);
  }

  bool Router::invoke (const String name, char *bytes, size_t size, ResultCallback callback) {
    auto message = Message { name, bytes, size };
    return this->invoke(message);
  }

  bool Router::invoke (const Message& message) {
    return this->invoke(message, [this, message](auto result) {
      this->send(message.seq, result.str(), result.post);
    });
  }

  bool Router::invoke (const Message& message, ResultCallback callback) {
    if (this->table.find(message.name) == this->table.end()) {
      return false;
    }

    auto fn = this->table.at(message.name);

    if (fn!= nullptr) {
      auto router = this;
      return this->dispatch([=] {
        fn(message, router, callback);
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
    const Post& post
  ) {
    if (post.body || seq == "-1") {
      auto script = this->core->createPost(seq, data, post);
      this->evaluateJavaScript(script);
      return true;
    }

    // this had a sequence, we need to try to resolve it.
    if (seq != "-1" && seq.size() > 0) {
      auto value = SSC::encodeURIComponent(data);
      auto script = SSC::getResolveToRenderProcessJavaScript(seq, "0", value);
      this->evaluateJavaScript(script);
      return true;
    }

    if (data.size() > 0) {
      this->evaluateJavaScript(data);
      return true;
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
  static Map bufferQueue;

  struct CallbackContext {
    Callback cb;
    SSC::String seq;
    Window *window;
    void *data;
  };
  static bool XXX (IPC::Message cmd, char *buf, size_t bufsize, Callback cb) {
    auto seq = cmd.get("seq");

    if (cmd.name == "post" || cmd.name == "data") {
      auto id = cmd.get("id");

      if (id.size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "id", "$S",
            "type": "InternalError",
            "message": "'id' is required"
          }
        })MSG", id);

        cb(seq, err, Post{});
        return true;
      }

      auto pid = std::stoull(id);

      if (!this->core->hasPost(pid)) {
        auto err = SSC::format(R"MSG({
          "err": {
            "id", "$S",
            "type": "InternalError",
            "message": "Invalid 'id' for post"
          }
        })MSG", id);

        cb(seq, err, Post{});
        return true;
      }

      auto post = this->core->getPost(pid);
      cb(seq, "{}", post);
      this->core->removePost(pid);
      return true;
    }

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

    if (cmd.name == "buffer.queue" && buf != nullptr) {
      if (seq.size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "Missing 'seq' for buffer.queue"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      auto bufferKey = std::to_string(cmd.index) + seq;
      auto str = SSC::String();
      str.assign(buf, bufsize);
      bufferQueue[bufferKey] = str;
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

    if (cmd.name == "udpClose" || cmd.name == "udp.close") {
      uint64_t peerId = 0ll;
      SSC::String err = "";

      if (cmd.get("id").size() == 0) {
        err = ".id is required";
      } else {
        try {
          peerId = std::stoull(cmd.get("id"));
        } catch (...) {
          err = "property .id is invalid";
        }
      }

      this->app->dispatch([=, this] {
        this->core->close(seq, peerId, cb);
      });
      return true;
    }

    if (cmd.name == "udpBind" || cmd.name == "udp.bind") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": ".id is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      if (cmd.get("port").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": "'port' is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto ip = cmd.get("address");
        auto reuseAddr = cmd.get("reuseAddr") == "true";
        int port;
        uint64_t peerId;

        if (ip.size() == 0) {
          ip = "0.0.0.0";
        }

        port = std::stoi(cmd.get("port"));
        peerId = std::stoull(cmd.get("id"));

        this->core->udpBind(seq, peerId, ip, port, reuseAddr, cb);
      });
      return true;
    }

    if (cmd.name == "udpConnect" || cmd.name == "udp.connect") {
      auto strId = cmd.get("id");
      SSC::String err = "";
      uint64_t peerId = 0ll;
      int port = 0;
      auto strPort = cmd.get("port");
      auto ip = cmd.get("address");

      if (strId.size() == 0) {
        err = "invalid peerId";
      } else {
        try {
          peerId = std::stoull(cmd.get("id"));
        } catch (...) {
          err = "invalid peerId";
        }
      }

      if (strPort.size() == 0) {
        err = "invalid port";
      } else {
        try {
          port = std::stoi(strPort);
        } catch (...) {
          err = "invalid port";
        }
      }

      if (port == 0) {
        err = "Can not bind to port 0";
      }

      if (err.size() > 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "message": "$S"
          }
        })MSG", err);
        cb(seq, msg, Post{});
        return true;
      }

      if (ip.size() == 0) {
        ip = "0.0.0.0";
      }

      this->app->dispatch([=, this] {
        this->core->udpConnect(seq, peerId, ip, port, cb);
      });

      return true;
    }

    if (cmd.name == "udpDisconnect" || cmd.name == "udp.disconnect") {
      auto strId = cmd.get("id");

      if (strId.size() == 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "message": "expected .peerId"
          }
        })MSG");
        cb(seq, msg, Post{});
        return true;
      }

      auto peerId = std::stoull(strId);

      this->app->dispatch([=, this] {
        this->core->udpDisconnect(seq, peerId, cb);
      });
      return true;
    }

    if (cmd.name == "udpGetPeerName" || cmd.name == "udp.getPeerName") {
      auto strId = cmd.get("id");

      if (strId.size() == 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "message": "expected .peerId"
          }
        })MSG");
        cb(seq, msg, Post{});
        return true;
      }

      auto peerId = std::stoull(strId);

      this->app->dispatch([=, this] {
        this->core->udpGetPeerName(seq, peerId, cb);
      });
      return true;
    }

    if (cmd.name == "udpGetSockName" || cmd.name == "udp.getSockName") {
      auto strId = cmd.get("id");

      if (strId.size() == 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "message": "expected .peerId"
          }
        })MSG");
        cb(seq, msg, Post{});
        return true;
      }

      auto peerId = std::stoull(strId);

      this->app->dispatch([=, this] {
        this->core->udpGetSockName(seq, peerId, cb);
      });
      return true;
    }

    if (cmd.name == "udpGetState" || cmd.name == "udp.getState") {
      auto strId = cmd.get("id");

      if (strId.size() == 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "message": "expected .peerId"
          }
        })MSG");
        cb(seq, msg, Post{});
        return true;
      }

      auto peerId = std::stoull(strId);

      this->app->dispatch([=, this] {
        this->core->udpGetState(seq, peerId, cb);
      });
      return true;
    }

    if (cmd.name == "udpReadStart" || cmd.name == "udp.readStart") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": ".id is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      auto peerId = std::stoull(cmd.get("id"));
      this->app->dispatch([=, this] {
        this->core->udpReadStart(seq, peerId, cb);
      });
      return true;
    }

    if (cmd.name == "udpReadStop" || cmd.name == "udp.readStop") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "err": {
            "type": "InternalError",
            "message": ".id is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      auto peerId = std::stoull(cmd.get("id"));
      this->app->dispatch([=, this] {
        this->core->udpReadStop(seq, peerId, cb);
      });
      return true;
    }

    if (cmd.name == "udpSend" || cmd.name == "udp.send") {
      int offset = 0;
      int port = 0;
      uint64_t peerId;
      SSC::String err;

      auto ephemeral = cmd.get("ephemeral") == "true";
      auto strOffset = cmd.get("offset");
      auto strPort = cmd.get("port");
      auto ip = cmd.get("address");

      if (strOffset.size() > 0) {
        try {
          offset = std::stoi(strOffset);
        } catch (...) {
          err = "invalid offset";
        }
      }

      try {
        port = std::stoi(strPort);
      } catch (...) {
        err = "invalid port";
      }

      if (ip.size() == 0) {
        ip = "0.0.0.0";
      }

      try {
        peerId = std::stoull(cmd.get("id"));
      } catch (...) {
        err = "invalid id";
      }

      if (err.size() > 0) {
        auto msg = SSC::format(R"MSG({
          "err": {
            "message": "$S"
          }
        })MSG", err);
        cb(seq, err, Post{});
        return true;
      }

      this->app->dispatch([=, this] {
        auto bufferKey = std::to_string(cmd.index) + seq;
        auto buffer = bufferQueue[bufferKey];
        auto data = buffer.data();
        auto size = buffer.size();

        bufferQueue.erase(bufferQueue.find(bufferKey));
        this->core->udpSend(seq, peerId, data, size, port, ip, ephemeral, cb);
      });
      return true;
    }

*/
