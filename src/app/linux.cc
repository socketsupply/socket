#include "app.hh"
#include "../core/core.hh"

namespace SSC {
  static Map bufferQueue;

  class Bridge {
    public:
      struct CallbackContext {
        Callback cb;
        std::string seq;
        Window *window;
        void *data;
      };

      App *app;
      Core *core;

      Bridge (IApp *app) {
        this->core = new Core();
        this->app = app;
      }

      bool route (std::string msg, char *buf, size_t bufsize);
      void send (Parse cmd, std::string seq, std::string msg, Post post);
      bool invoke (Parse cmd, char *buf, size_t bufsize, Callback cb);
      bool invoke (Parse cmd, Callback cb);
  };

  std::atomic<bool> App::isReady {false};

  App::App (int instanceId) : bridge(this) {
    auto webkitContext = webkit_web_context_get_default();
    gtk_init_check(0, nullptr);
    // TODO enforce single instance is set
    webkit_web_context_register_uri_scheme(
      webkitContext,
      "ipc",
      [](WebKitURISchemeRequest *request, gpointer arg) {
        auto *app = static_cast<App*>(arg);
        auto uri = std::string(webkit_uri_scheme_request_get_uri(request));
        auto msg = std::string(uri);

        Parse cmd(msg);

        auto invoked = app->bridge.invoke(cmd, [=](auto seq, auto result, auto post) {
          auto size = post.body != nullptr ? post.length : result.size();
          auto body = post.body != nullptr ? post.body : result.c_str();

          // `post.body` is free'd with `freeFunction`
          post.bodyNeedsFree = false;
          auto freeFunction = post.body != nullptr ? free : nullptr;
          auto stream = g_memory_input_stream_new_from_data(body, size, freeFunction);
          auto response = webkit_uri_scheme_response_new(stream, size);

          webkit_uri_scheme_response_set_content_type(
            response,
            "application/octet-stream"
          );

          webkit_uri_scheme_request_finish_with_response(request, response);

          g_object_unref(stream);
        });

        if (!invoked) {
          auto windowFactory = reinterpret_cast<WindowFactory<Window, App> *>(app->getWindowFactory());

          if (windowFactory) {
            auto window = windowFactory->getWindow(cmd.index);

            if (window && window->onMessage) {
              window->onMessage(msg);
            }
          }

          auto msg = SSC::format(R"MSG({
            "err": {
              "message": "Not found",
              "type": "NotFoundError",
              "url": "$S"
            }
          })MSG", uri);

          auto stream = g_memory_input_stream_new_from_data(msg.c_str(), msg.size(), 0);
          auto response = webkit_uri_scheme_response_new(stream, msg.size());

          webkit_uri_scheme_response_set_content_type(
            response,
            "application/octet-stream"
          );

          webkit_uri_scheme_request_finish_with_response(request, response);
          g_object_unref(stream);
        }
      },
      this,
      0
    );
  }

  int App::run () {
    auto cwd = getCwd("");
    uv_chdir(cwd.c_str());
    gtk_main();
    return shouldExit ? 1 : 0;
  }

  void App::kill () {
    shouldExit = true;
    gtk_main_quit();
  }

  void App::restart () {
  }

  std::string App::getCwd(const std::string &s) {
    auto canonical = fs::canonical("/proc/self/exe");
    return std::string(fs::path(canonical).parent_path());
  }

  void App::dispatch(std::function<void()> f) {
    g_idle_add_full(
      G_PRIORITY_HIGH_IDLE,
      (GSourceFunc)([](void* f) -> int {
        (*static_cast<std::function<void()>*>(f))();
        return G_SOURCE_REMOVE;
      }),
      new std::function<void()>(f),
      [](void* f) {
        delete static_cast<std::function<void()>*>(f);
      }
    );
  }

  bool Bridge::invoke (Parse cmd, Callback cb) {
    return this->invoke(cmd, nullptr, 0, cb);
  }

  bool Bridge::invoke (Parse cmd, char *buf, size_t bufsize, Callback cb) {
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

    if (cmd.name == "getPlatformOS" || cmd.name == "os.platform") {
      auto msg = SSC::format(R"JSON({
        "data": "$S"
      })JSON", SSC::platform.os);

      cb(seq, msg, Post{});
      return true;
    }

    if (cmd.name == "getPlatformType" || cmd.name == "os.type") {
      auto msg = SSC::format(R"JSON({
        "data": "$S"
      })JSON", SSC::platform.os);

      cb(seq, msg, Post{});
      return true;
    }

    if (cmd.name == "getPlatformArch" || cmd.name == "os.arch") {
      auto msg = SSC::format(R"JSON({
        "data": "$S"
      })JSON", SSC::platform.arch);

      cb(seq, msg, Post{});
      return true;
    }

    if (cmd.name == "getNetworkInterfaces" || cmd.name == "os.networkInterfaces") {
      this->app->dispatch([=, this] {
        auto msg = this->core->getNetworkInterfaces();
        cb(seq, msg, Post{});
      });
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
      auto ctx = new Bridge::CallbackContext { cb, seq, window, (void *) this };

      webkit_web_view_run_javascript(
        WEBKIT_WEB_VIEW(window->webview),
        value.c_str(),
        nullptr,
        [](GObject *object, GAsyncResult *res, gpointer data) {
          GError *error = nullptr;
          auto ctx = reinterpret_cast<Bridge::CallbackContext *>(data);
          auto result = webkit_web_view_run_javascript_finish(
            WEBKIT_WEB_VIEW(ctx->window->webview),
            res,
            &error
          );

          if (!result) {
            auto msg = SSC::format(
              R"MSG({"err": { "code": "$S", "message": "$S" } })MSG",
              std::to_string(error->code),
              std::string(error->message)
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
              std::string msg = "";

              if (exception) {
                auto message = jsc_exception_get_message(exception);
                msg = SSC::format(
                  R"MSG({"err": { "message": "$S" } })MSG",
                  std::string(message)
                );
              } else if (string) {
                msg = std::string(string);
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

    if (cmd.name == "dnsLookup" || cmd.name == "dns.lookup") {
      auto hostname = cmd.get("hostname");
      auto strFamily = cmd.get("family");
      int family = 0;

      if (strFamily.size() > 0) {
        try {
          family = std::stoi(strFamily);
        } catch (...) {
        }
      }

      this->app->dispatch([=, this] {
        this->core->dnsLookup(seq, hostname, family, cb);
      });
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
      auto str = std::string();
      str.assign(buf, bufsize);
      bufferQueue[bufferKey] = str;
      return true;
    }

    if (cmd.name == "event") {
      this->app->dispatch([=, this] {
        auto event = decodeURIComponent(cmd.get("value"));
        auto data = decodeURIComponent(cmd.get("data"));
        auto seq = cmd.get("seq");

        this->core->handleEvent(seq, event, data, cb);
      });
      return true;
    }

    if (cmd.name == "getFSConstants" || cmd.name == "fs.constants") {
      cb(seq, this->core->getFSConstants(), Post{});
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
      std::string err = "";

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
      std::string err = "";
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
      std::string err;

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

    if (cmd.name == "bufferSize") {
      if (cmd.get("id").size() == 0) {
        auto err = SSC::format(R"MSG({
          "source": "bufferSize",
          "err": {
            "type": "InternalError",
            "message": ".id is required"
          }
        })MSG");

        cb(seq, err, Post{});
        return true;
      }

      auto buffer = std::stoi(cmd.get("buffer", "0"));
      auto size = std::stoi(cmd.get("size", "0"));
      auto id  = std::stoull(cmd.get("id"));

      this->app->dispatch([=, this] {
        this->core->bufferSize(seq, id, size, buffer, cb);
      });

      return true;
    }

    return false;
  }

  bool Bridge::route (std::string msg, char *buf, size_t bufsize) {
    Parse cmd(msg);

    return this->invoke(cmd, buf, bufsize, [cmd, this](auto seq, auto msg, auto post) {
      this->send(cmd, seq, msg, post);
    });
  }

  void Bridge::send (Parse cmd, std::string seq, std::string msg, Post post) {
    if (cmd.index == -1) {
      // @TODO(jwerle): print warning
      return;
    }

    if (app == nullptr) {
      // @TODO(jwerle): print warning
      return;
    }

    auto windowFactory = reinterpret_cast<WindowFactory<Window, App> *>(app->getWindowFactory());
    if (windowFactory == nullptr) {
      // @TODO(jwerle): print warning
      return;
    }

    auto window = windowFactory->getWindow(cmd.index);

    if (window == nullptr) {
      // @TODO(jwerle): print warning
      return;
    }

    if (post.body || seq == "-1") {
      auto script = this->core->createPost(seq, msg, post);
      window->eval(script);
      return;
    }

    if (seq != "-1" && seq.size() > 0) {
      msg = SSC::resolveToRenderProcess(seq, "0", encodeURIComponent(msg));
    }

    window->eval(msg);
  }
}
