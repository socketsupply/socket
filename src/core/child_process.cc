#include "core.hh"

#if !SSC_PLATFORM_IOS
namespace SSC {
  void Core::ChildProcess::shutdown () {
    Lock lock(this->mutex);
    for (const auto& entry : this->handles) {
      auto process = entry.second;
      process->kill();
      process->wait();
    }

    this->handles.clear();
  }

  void Core::ChildProcess::kill (
    const String seq,
    uint64_t id,
    int signal,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this] {
      Lock lock(mutex);

      if (!this->handles.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", "NotFoundError"},
            {"message", "A process with that id does not exist"}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto process = this->handles.at(id);

      #ifdef _WIN32
        process->kill();
      #else
        ::kill(-process->id, signal);
      #endif

      cb(seq, JSON::Object{}, Post{});
    });
  }

  void Core::ChildProcess::exec (
    const String seq,
    uint64_t id,
    const Vector<String> args,
    const ExecOptions options,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this] {
      Lock lock(mutex);

      if (this->handles.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", "A process with that id already exists"}
          }}
        };

        return cb(seq, json, Post{});
      }

      Process* process = nullptr;
      Core::Timers::ID timer;

      const auto command = args.size() > 0 ? args.at(0) : String("");
      const auto argv = join(args.size() > 1 ? Vector<String>{ args.begin() + 1, args.end() } : Vector<String>{}, " ");

      auto stdoutBuffer = new JSON::Array{};
      auto stderrBuffer = new JSON::Array{};

      const auto onStdout = [=](const String& output) mutable {
        if (!options.allowStdout || output.size() == 0) {
          return;
        }

        if (stdoutBuffer != nullptr) {
          stdoutBuffer->push(output);
        }
      };

      const auto onStderr = [=](const String& output) mutable {
        if (!options.allowStderr || output.size() == 0) {
          return;
        }

        if (stderrBuffer != nullptr) {
          stderrBuffer->push(output);
        }
      };

      const auto onExit = [=, this](const String& output) mutable {
        if (timer > 0) {
          this->core->clearTimeout(timer);
        }

        this->core->dispatchEventLoop([=, this] () mutable {
          Lock lock(mutex);
          if (this->handles.contains(id)) {
            auto process = this->handles.at(id);
            const auto pid = process->id;
            const auto code = process->wait();
            const auto json = JSON::Object::Entries {
              {"source", "child_process.exec"},
              {"data", JSON::Object::Entries {
                {"id", std::to_string(id)},
                {"pid", std::to_string(pid)},
                {"stdout", *stdoutBuffer},
                {"stderr", *stderrBuffer},
                {"code", code}
              }}
            };

            this->handles.erase(id);
            delete stdoutBuffer;
            delete stderrBuffer;
            delete process;

            stdoutBuffer = nullptr;
            stderrBuffer = nullptr;
            process = nullptr;

            cb(seq, json, Post{});
          }
        });
      };

      process = new Process(
        command,
        argv,
        options.cwd,
        onStdout,
        onStderr,
        onExit,
        false
      );

      do {
        Lock lock(mutex);
        this->handles.insert_or_assign(id, process);
      } while (0);

      const auto pid = process->open();
      if (options.timeout > 0) {
        timer = this->core->setTimeout(options.timeout, [=, this] () mutable {
          Lock lock(mutex);
          const auto json = JSON::Object::Entries {
            {"source", "child_process.exec"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"pid", std::to_string(pid)},
              {"stdout", *stdoutBuffer},
              {"stderr", *stderrBuffer},
              {"code", "ETIMEDOUT"}
            }}
          };

          this->handles.erase(id);

          this->core->dispatchEventLoop([=, this] {
            cb(seq, json, Post{});
          });

        #ifdef _WIN32
          process->kill();
        #else
          ::kill(-process->id, options.killSignal);
        #endif

          delete stdoutBuffer;
          delete stderrBuffer;
          delete process;

          stdoutBuffer = nullptr;
          stderrBuffer = nullptr;
          process = nullptr;
        });
      }
    });
  }

  void Core::ChildProcess::spawn (
    const String seq,
    uint64_t id,
    const Vector<String> args,
    const SpawnOptions options,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this] {
      Lock lock(mutex);

      if (this->handles.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", "A process with that id already exists"}
          }}
        };

        return cb(seq, json, Post{});
      }

      Process* process = nullptr;

      const auto command = args.size() > 0 ? args.at(0) : String("");
      const auto argv = join(args.size() > 1 ? Vector<String>{ args.begin() + 1, args.end() } : Vector<String>{}, " ");

      const auto onStdout = [=](const String& output) {
        if (!options.allowStdout || output.size() == 0) {
          return;
        }

        const auto bytes = new char[output.size()]{0};
        const auto headers = Headers {{
          {"content-type" ,"application/octet-stream"},
          {"content-length", (int) output.size()}
        }};

        memcpy(bytes, output.c_str(), output.size());

        Post post;
        post.id = rand64();
        post.body = bytes;
        post.length = (int) output.size();
        post.headers = headers.str();

        const auto json = JSON::Object::Entries {
          {"source", "child_process.spawn"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"source", "stdout"}
          }}
        };

        cb("-1", json, post);
      };

      const auto onStderr = [=](const String& output) {
        if (!options.allowStderr || output.size() == 0) {
          return;
        }

        const auto bytes = new char[output.size()]{0};
        const auto headers = Headers {{
          {"content-type" ,"application/octet-stream"},
          {"content-length", (int) output.size()}
        }};

        memcpy(bytes, output.c_str(), output.size());

        Post post;
        post.id = rand64();
        post.body = bytes;
        post.length = (int) output.size();
        post.headers = headers.str();

        const auto json = JSON::Object::Entries {
          {"source", "child_process.spawn"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"source", "stderr"}
          }}
        };

        cb("-1", json, post);
      };

      const auto onExit = [=, this](const String& output) {
        const auto code = output.size() > 0 ? std::stoi(output) : 0;
        const auto json = JSON::Object::Entries {
          {"source", "child_process.spawn"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"status", "exit"},
            {"code", code}
          }}
        };

        cb("-1", json, Post{});

        this->core->dispatchEventLoop([=, this] {
          Lock lock(mutex);
          if (this->handles.contains(id)) {
            auto process = this->handles.at(id);
            this->handles.erase(id);

            const auto code = process->wait();

            delete process;

            this->core->dispatchEventLoop([=, this] {
              const auto json = JSON::Object::Entries {
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
        });
      };

      process = new Process(
        command,
        argv,
        options.cwd,
        onStdout,
        onStderr,
        onExit,
        options.allowStdin
      );

      do {
        Lock lock(mutex);
        this->handles.insert_or_assign(id, process);
      } while (0);

      const auto pid = process->open();
      const auto json = JSON::Object::Entries {
        {"source", "child_process.spawn"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"pid", std::to_string(pid)}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void Core::ChildProcess::write (
    const String seq,
    uint64_t id,
    char* buffer,
    size_t size,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this] {
      Lock lock(mutex);

      if (!this->handles.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", "NotFoundError"},
            {"message", "A process with that id does not exist"}
          }}
        };

        return cb(seq, json, Post{});
      }

      bool didWrite = false;

      auto process = this->handles.at(id);

      if (!process->open_stdin) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", "NotSupportedError"},
            {"message", "Child process stdin is not opened"}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      try {
        didWrite = process->write(buffer, size);
      } catch (std::exception& e) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", "InternalError"},
            {"message", e.what()}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      if (!didWrite) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", process->lastWriteStatus != 0 ? "ErrnoError" : "InternalError"},
            {"message", process->lastWriteStatus != 0 ? strerror(process->lastWriteStatus) : "Failed to write to child process"}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      cb(seq, JSON::Object{}, Post{});
      return;
    });
  }
}
#endif
