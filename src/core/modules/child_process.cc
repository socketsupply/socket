#include "../headers.hh"
#include "../codec.hh"
#include "../core.hh"
#include "child_process.hh"
#include "timers.hh"

namespace SSC {
  void CoreChildProcess::shutdown () {
  #if !SOCKET_RUNTIME_PLATFORM_IOS
    Lock lock(this->mutex);
    for (const auto& entry : this->handles) {
      auto process = entry.second;
      process->kill();
      process->wait();
    }
  #endif

    this->handles.clear();
  }

  void CoreChildProcess::kill (
    const String& seq,
    ID id,
    int signal,
    const CoreModule::Callback& callback
  ) {
  #if SOCKET_RUNTIME_PLATFORM_IOS
    const auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotSupportedError"},
        {"message", "kill() is not supported"}
      }}
    };
    return callback(seq, json, Post{});
  #else
    this->core->dispatchEventLoop([=, this] {
      Lock lock(this->mutex);

      if (!this->handles.contains(id)) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", "NotFoundError"},
            {"message", "A process with that id does not exist"}
          }}
        };

        return this->core->dispatchEventLoop([=, this] () {
          callback(seq, json, Post{});
        });
      }

      auto process = this->handles.at(id);

      #if SOCKET_RUNTIME_PLATFORM_WINDOWS
        process->kill();
      #else
        ::kill(-process->id, signal);
      #endif

      return this->core->dispatchEventLoop([=, this] () {
        callback(seq, JSON::Object{}, Post{});
      });
    });
  #endif
  }

  void CoreChildProcess::exec (
    const String& seq,
    ID id,
    const Vector<String> args,
    const ExecOptions options,
    const CoreModule::Callback& callback
  ) {
  #if SOCKET_RUNTIME_PLATFORM_IOS
    const auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotSupportedError"},
        {"message", "exec() is not supported"}
      }}
    };
    return callback(seq, json, Post{});
  #else
    this->core->dispatchEventLoop([=, this] {
      Lock lock(this->mutex);

      if (this->handles.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", "A process with that id already exists"}
          }}
        };

        return this->core->dispatchEventLoop([=, this] () {
          callback(seq, json, Post{});
        });
      }

      SharedPointer<Process> process = nullptr;
      CoreTimers::ID timer;

      const auto command = args.size() > 0 ? args.at(0) : String("");
      const auto argv = join(args.size() > 1 ? Vector<String>{ args.begin() + 1, args.end() } : Vector<String>{}, " ");

      auto stdoutBuffer = new StringStream;
      auto stderrBuffer = new StringStream;

      const auto onStdout = [=](const String& output) mutable {
        if (!options.allowStdout || output.size() == 0) {
          return;
        }

        if (stdoutBuffer != nullptr) {
          *stdoutBuffer << String(output);
        }
      };

      const auto onStderr = [=](const String& output) mutable {
        if (!options.allowStderr || output.size() == 0) {
          return;
        }

        if (stderrBuffer != nullptr) {
          *stderrBuffer << String(output);
        }
      };

      const auto onExit = [=, this](const String& output) mutable {
        if (timer > 0) {
          this->core->clearTimeout(timer);
        }

        this->core->dispatchEventLoop([=, this] () mutable {
          Lock lock(this->mutex);
          if (this->handles.contains(id)) {
            auto process = this->handles.at(id);
            const auto pid = process->id;
            const auto code = process->wait();
            const auto json = JSON::Object::Entries {
              {"source", "child_process.exec"},
              {"data", JSON::Object::Entries {
                {"id", std::to_string(id)},
                {"pid", std::to_string(pid)},
                {"stdout", encodeURIComponent(stdoutBuffer->str())},
                {"stderr", encodeURIComponent(stderrBuffer->str())},
                {"code", code}
              }}
            };

            delete stdoutBuffer;
            delete stderrBuffer;

            stdoutBuffer = nullptr;
            stderrBuffer = nullptr;

            return this->core->dispatchEventLoop([=, this] () {
              callback(seq, json, Post{});
            });

            this->handles.erase(id);
          }
        });
      };

      process.reset(new Process(
        command,
        argv,
        options.env,
        options.cwd,
        onStdout,
        onStderr,
        onExit,
        false
      ));

      this->handles.insert_or_assign(id, process);

      const auto pid = process->open();

      if (options.timeout > 0) {
        timer = this->core->setTimeout(options.timeout, [=, this] () mutable {
          Lock lock(this->mutex);
          const auto json = JSON::Object::Entries {
            {"source", "child_process.exec"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"pid", std::to_string(pid)},
              {"stdout", encodeURIComponent(stdoutBuffer->str())},
              {"stderr", encodeURIComponent(stderrBuffer->str())},
              {"code", "ETIMEDOUT"}
            }}
          };

          this->core->dispatchEventLoop([=, this] {
            callback(seq, json, Post{});
          });

        #if SOCKET_RUNTIME_PLATFORM_WINDOWS
          process->kill();
        #else
          ::kill(-process->id, options.killSignal);
        #endif

          delete stdoutBuffer;
          delete stderrBuffer;

          stdoutBuffer = nullptr;
          stderrBuffer = nullptr;

          this->handles.erase(id);
        });
      }
    });
  #endif
  }

  void CoreChildProcess::spawn (
    const String& seq,
    ID id,
    const Vector<String> args,
    const SpawnOptions options,
    const CoreModule::Callback& callback
  ) {
  #if SOCKET_RUNTIME_PLATFORM_IOS
    const auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotSupportedError"},
        {"message", "spawn() is not supported"}
      }}
    };
    return callback(seq, json, Post{});
  #else
    this->core->dispatchEventLoop([=, this] {
      Lock lock(this->mutex);

      if (this->handles.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", "A process with that id already exists"}
          }}
        };

        return this->core->dispatchEventLoop([=, this] () {
          callback(seq, json, Post{});
        });
      }

      SharedPointer<Process> process = nullptr;

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
        post.body.reset(bytes);
        post.length = (int) output.size();
        post.headers = headers.str();

        const auto json = JSON::Object::Entries {
          {"source", "child_process.spawn"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"source", "stdout"}
          }}
        };

        callback("-1", json, post);
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
        post.body.reset(bytes);
        post.length = (int) output.size();
        post.headers = headers.str();

        const auto json = JSON::Object::Entries {
          {"source", "child_process.spawn"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"source", "stderr"}
          }}
        };

        callback("-1", json, post);
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

        callback("-1", json, Post{});

        this->core->dispatchEventLoop([=, this] {
          SharedPointer<Process> process = nullptr;
          do {
            Lock lock(this->mutex);
            if (!this->handles.contains(id)) {
              return;
            }

            process = this->handles.at(id);
          } while (0);

          const auto code = process->wait();

          this->core->dispatchEventLoop([=, this] {
            const auto json = JSON::Object::Entries {
              {"source", "child_process.spawn"},
              {"data", JSON::Object::Entries {
                {"id", std::to_string(id)},
                {"status", "close"},
                {"code", code}
              }}
            };

            callback("-1", json, Post{});

            Lock lock(this->mutex);
            this->handles.erase(id);
          });
        });
      };

      process.reset(new Process(
        command,
        argv,
        options.env,
        options.cwd,
        onStdout,
        onStderr,
        onExit,
        options.allowStdin
      ));

      this->handles.insert_or_assign(id, process);

      const auto pid = process->open();
      const auto json = JSON::Object::Entries {
        {"source", "child_process.spawn"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"pid", std::to_string(pid)}
        }}
      };

      return this->core->dispatchEventLoop([=, this] () {
        callback(seq, json, Post{});
      });
    });
  #endif
  }

  void CoreChildProcess::write (
    const String& seq,
    ID id,
    SharedPointer<char[]> buffer,
    size_t size,
    const CoreModule::Callback& callback
  ) {
  #if SOCKET_RUNTIME_PLATFORM_IOS
    const auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotSupportedError"},
        {"message", "write() is not supported"}
      }}
    };
    return callback(seq, json, Post{});
  #else
    this->core->dispatchEventLoop([=, this] {
      Lock lock(this->mutex);

      if (!this->handles.contains(id)) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", "NotFoundError"},
            {"message", "A process with that id does not exist"}
          }}
        };

        return callback(seq, json, Post{});
      }

      bool didWrite = false;

      auto process = this->handles.at(id);

      if (!process->openStdin) {
        auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"type", "NotSupportedError"},
            {"message", "Child process stdin is not opened"}
          }}
        };

        callback(seq, json, Post{});
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

        callback(seq, json, Post{});
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

        callback(seq, json, Post{});
        return;
      }

      callback(seq, JSON::Object{}, Post{});
      return;
    });
  #endif
  }
}
