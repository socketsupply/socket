#include "core.hh"

namespace SSC {
  // 
  // TODO(@heapwolf): clean up all threads on process exit
  //
  void Core::ChildProcess::kill (const String seq, uint64_t id, int signal, Module::Callback cb) {
    this->core->dispatchEventLoop([=, this] {
      if (!this->processes.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", "NotFoundError"},
            {"message", "A process with that id does not exist"}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto p = this->processes.at(id);

      #ifdef _WIN32
        p->kill();
      #else
        ::kill(p->id, signal);
      #endif

      cb(seq, JSON::Object{}, Post{});
    });
  }

  void Core::ChildProcess::spawn (const String seq, uint64_t id, const String cwd, Vector<String> args, Module::Callback cb) {
    this->core->dispatchEventLoop([=, this] {
      auto command = args.at(0);

      if (this->processes.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", "A process with that id already exists"}
          }}
        };

        return cb(seq, json, Post{});
      }

      const auto argv = join(args.size() > 1 ? Vector<String>{ args.begin() + 1, args.end() } : Vector<String>{}, " ");

      Process* p = new Process(
        command,
        argv,
        cwd,
        [=](SSC::String const &out) {
          Post post;

          auto bytes = new char[out.size()]{0};
          memcpy(bytes, out.c_str(), out.size());

          auto headers = Headers {{
            {"content-type" ,"application/octet-stream"},
            {"content-length", (int) out.size()}
          }};

          post.id = rand64();
          post.body = bytes;
          post.length = (int) out.size();
          post.headers = headers.str();

          auto json = JSON::Object::Entries {
            {"source", "child_process.spawn"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"source", "stdout" }
            }}
          };

          cb("-1", json, post);
        },
        [=](SSC::String const &out) {
          Post post;

          auto bytes = new char[out.size()]{0};
          memcpy(bytes, out.c_str(), out.size());

          auto headers = Headers {{
            {"content-type" ,"application/octet-stream"},
            {"content-length", (int) out.size()}
          }};

          post.id = rand64();
          post.body = bytes;
          post.length = (int) out.size();
          post.headers = headers.str();

          auto json = JSON::Object::Entries {
            {"source", "child_process.spawn"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"source", "stderr" }
            }}
          };

          cb("-1", json, post);
        },
        [=, this](SSC::String const &code) {
          this->processes.erase(id);

          auto json = JSON::Object::Entries {
            {"source", "child_process.spawn"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"status", "exit"},
              {"code", std::stoi(code)}
            }}
          };

          cb("-1", json, Post{});

          this->core->dispatchEventLoop([=, this] {
            auto code = p->wait();
            delete p;

            auto json = JSON::Object::Entries {
              {"source", "child_process.spawn"},
              {"data", JSON::Object::Entries {
                {"id", std::to_string(id)},
                {"status", "close"},
                {"code", code}
              }}
            };

            cb("-1", json, Post{});
          });
        }
      );

      this->processes.insert_or_assign(id, p);

      auto pid = p->open();
      
      auto json = JSON::Object::Entries {
        {"source", "child_process.spawn"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"pid", pid}
        }}
      };

      cb(seq, json, Post{});
    });
  }
}
