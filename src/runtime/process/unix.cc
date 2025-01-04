#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <set>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

#include "../debug.hh"
#include "../string.hh"
#include "../process.hh"

using ssc::runtime::string::splitc;

extern char **environ;

namespace ssc::runtime::process {
  static StringStream initial;

  Process::Data::Data () noexcept
    : id(-1)
  {}

  Process::Process (
    const String &command,
    const String &argv,
    const Vector<String> &env,
    const String &path,
    MessageCallback readStdout,
    MessageCallback readStderr,
    MessageCallback onExit,
    bool openStdin,
    const ProcessConfig &config
  ) noexcept
    : openStdin(true),
      readStdout(std::move(readStdout)),
      readStderr(std::move(readStderr)),
      onExit(std::move(onExit)),
      env(env),
      command(command),
      argv(argv),
      path(path)
  {}

  Process::Process (
    const String &command,
    const String &argv,
    const String &path,
    MessageCallback readStdout,
    MessageCallback readStderr,
    MessageCallback onExit,
    bool openStdin,
    const ProcessConfig &config
  ) noexcept
    : openStdin(true),
      readStdout(std::move(readStdout)),
      readStderr(std::move(readStderr)),
      onExit(std::move(onExit)),
      command(command),
      argv(argv),
      path(path)
  {}

  Process::Process (
    const Function<int()> &function,
    MessageCallback readStdout,
    MessageCallback readStderr,
    MessageCallback onExit,
    bool openStdin,
    const ProcessConfig &config
  ) noexcept
    : readStdout(std::move(readStdout)),
      readStderr(std::move(readStderr)),
      onExit(std::move(onExit)),
      openStdin(openStdin),
      config(config)
  {
    #if !SOCKET_RUNTIME_PLATFORM_IOS
      open(function);
      read();
    #endif
  }

  Process::PID Process::open (const Function<int()> &function) noexcept {
    #if SOCKET_RUNTIME_PLATFORM_IOS
      return -1; // -EPERM
    #else

    if (openStdin) {
      stdinFD = UniquePointer<FD>(new FD);
    }

    if (readStdout) {
      stdoutFD = UniquePointer<FD>(new FD);
    }

    if (readStderr) {
      stderrFD = UniquePointer<FD>(new FD);
    }

    int stdin_p[2];
    int stdout_p[2];
    int stderr_p[2];

    if (stdinFD && pipe(stdin_p) != 0) {
      return -1;
    }

    if (stdoutFD && pipe(stdout_p) != 0) {
      if (stdinFD) {
        close(stdin_p[0]);
        close(stdin_p[1]);
      }
      return -1;
    }

    if (stderrFD && pipe(stderr_p) != 0) {
      if (stdinFD) {
        close(stdin_p[0]);
        close(stdin_p[1]);
      }

      if (stdoutFD) {
        close(stdout_p[0]);
        close(stdout_p[1]);
      }

      return -1;
    }

    PID pid = fork();

    if (pid < 0) {
      if (stdinFD) {
        close(stdin_p[0]);
        close(stdin_p[1]);
      }

      if (stdoutFD) {
        close(stdout_p[0]);
        close(stdout_p[1]);
      }

      if (stderrFD) {
        close(stderr_p[0]);
        close(stderr_p[1]);
      }

      return pid;
    }

    closed = false;
    id = pid;

    if (pid > 0) {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      setpgid(pid, getpgid(0));
    #endif

      auto thread = Thread([this] {
        int code = 0;
        waitpid(this->id, &code, 0);

        this->status = WEXITSTATUS(code);
        this->closed = true;

        if (this->onExit != nullptr) {
          this->onExit(std::to_string(status));
        }
      });

      thread.detach();
    } else if (pid == 0) {
      if (stdinFD) {
        dup2(stdin_p[0], 0);
      }

      if (stdoutFD) {
        dup2(stdout_p[1], 1);
      }

      if (stderrFD) {
        dup2(stderr_p[1], 2);
      }

      if (stdinFD) {
        close(stdin_p[0]);
        close(stdin_p[1]);
      }

      if (stdoutFD) {
        close(stdout_p[0]);
        close(stdout_p[1]);
      }

      if (stderrFD) {
        close(stderr_p[0]);
        close(stderr_p[1]);
      }

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      setpgid(0, 0);
    #endif

      if (function) {
        function();
      }

      _exit(EXIT_FAILURE);
    }

    if (stdinFD) {
      close(stdin_p[0]);
    }

    if (stdoutFD) {
      close(stdout_p[1]);
    }

    if (stderrFD) {
      close(stderr_p[1]);
    }

    if (stdinFD) {
      *stdinFD = stdin_p[1];
    }

    if (stdoutFD) {
      *stdoutFD = stdout_p[0];
    }

    if (stderrFD) {
      *stderrFD = stderr_p[0];
    }

    data.id = pid;
    return pid;
  #endif
  }

  Process::PID Process::open (const String &command, const String &path) noexcept {
    #if SOCKET_RUNTIME_PLATFORM_IOS
      return -1; // -EPERM
    #else

     std::vector<char*> newEnv;

     for (char** env = environ; *env != nullptr; ++env) {
        newEnv.push_back(strdup(*env));
      }

      for (const auto& str : this->env) {
        newEnv.push_back(const_cast<char*>(str.c_str()));
      }

      newEnv.push_back(nullptr);

      return open([&command, &path, &newEnv, this] {
        auto command_c_str = command.c_str();
        String cd_path_and_command;

        if (!path.empty()) {
          auto path_escaped = path;
          size_t pos = 0;

          while ((pos = path_escaped.find('\'', pos)) != String::npos) {
            path_escaped.replace(pos, 1, "'\\''");
            pos += 4;
          }

          cd_path_and_command = "cd '" + path_escaped + "' && " + command; // To avoid resolving symbolic links
          command_c_str = cd_path_and_command.c_str();
        }

        #if SOCKET_RUNTIME_PLATFORM_APPLE
          setpgid(0, 0);
        #endif

        if (this->shell.size() > 0) {
          return execle(this->shell.c_str(), this->shell.c_str(), "-c", command_c_str, (char*)nullptr, newEnv.data());
        } else {
          return execle("/bin/sh", "/bin/sh", "-c", command_c_str, (char*)nullptr, newEnv.data());
        }
      });
    #endif
  }

  int Process::wait () {
  #if SOCKET_RUNTIME_PLATFORM_IOS
    return -1; // -EPERM
  #else
    do {
      msleep(Process::PROCESS_WAIT_TIMEOUT);
    } while (this->closed == false);

    return this->status;
  #endif
  }

  void Process::read() noexcept {
  #if !SOCKET_RUNTIME_PLATFORM_IOS
    if (data.id <= 0 || (!stdoutFD && !stderrFD)) {
      return;
    }

    stdoutAndStderrThread = Thread([this] {
      Vector<pollfd> pollfds;
      std::bitset<2> fd_is_stdout;

      if (stdoutFD) {
        fd_is_stdout.set(pollfds.size());
        pollfds.emplace_back();
        pollfds.back().fd = fcntl(*stdoutFD, F_SETFL, fcntl(*stdoutFD, F_GETFL) | O_NONBLOCK) == 0 ? *stdoutFD : -1;
        pollfds.back().events = POLLIN;
      }

      if (stderrFD) {
        pollfds.emplace_back();
        pollfds.back().fd = fcntl(*stderrFD, F_SETFL, fcntl(*stderrFD, F_GETFL) | O_NONBLOCK) == 0 ? *stderrFD : -1;
        pollfds.back().events = POLLIN;
      }

      auto buffer = UniquePointer<unsigned char[]>(new unsigned char[config.bufferSize]);
      bool any_open = !pollfds.empty();
      StringStream ss;

      while (any_open && (poll(pollfds.data(), static_cast<nfds_t>(pollfds.size()), -1) > 0 || errno == EINTR)) {
        any_open = false;

        for (size_t i = 0; i < pollfds.size(); ++i) {
          if (!(pollfds[i].fd >= 0)) continue;
          if (pollfds[i].revents & POLLIN) {
            memset(buffer.get(), 0, config.bufferSize);
            const ssize_t n = ::read(pollfds[i].fd, buffer.get(), config.bufferSize);

            if (n > 0) {
              if (fd_is_stdout[i]) {
                Lock lock(stdoutMutex);
                auto b = String(reinterpret_cast<char*>(buffer.get()));
                auto parts = splitc(b, '\n');

                if (parts.size() > 1) {
                  for (int i = 0; i < parts.size() - 1; i++) {
                    ss << parts[i];

                    String s(ss.str());
                    readStdout(s);

                    ss.str(String());
                    ss.clear();
                    ss.copyfmt(initial);
                  }
                  ss << parts[parts.size() - 1];
                } else {
                  ss << b;
                }
              } else {
                Lock lock(stderrMutex);
                readStderr(String(reinterpret_cast<char*>(buffer.get())));
              }
            } else if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
              pollfds[i].fd = -1;
              continue;
            }
          }

          if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            pollfds[i].fd = -1;
            continue;
          }

          any_open = true;
        }
      }
    });
  #endif
  }

  void Process::closeFDs () noexcept {
  #if !SOCKET_RUNTIME_PLATFORM_IOS
    if (stdoutAndStderrThread.joinable()) {
      stdoutAndStderrThread.join();
    }

    if (stdinFD) {
      closeStdin();
    }

    if (stdoutFD) {
      if (data.id > 0) {
        close(*stdoutFD);
      }

      stdoutFD.reset();
    }

    if (stderrFD) {
      if (data.id > 0) {
        close(*stderrFD);
      }

      stderrFD.reset();
    }
  #endif
  }

  bool Process::write (const char *bytes, size_t n) {
  #if SOCKET_RUNTIME_PLATFORM_IOS
    return false;
  #else
    Lock lock(stdinMutex);

    this->lastWriteStatus = 0;

    if (stdinFD) {
      String b(bytes);

      while (true && (b.size() > 0)) {
        int bytesWritten = ::write(*stdinFD, b.c_str(), b.size());

        if (bytesWritten == -1) {
          this->lastWriteStatus = errno;
          return false;
        }

        if (bytesWritten >= b.size()) {
          break;
        }

        b = b.substr(bytesWritten, b.size());
      }

      int bytesWritten = ::write(*stdinFD, "\n", 1);
      if (bytesWritten == -1) {
        this->lastWriteStatus = errno;
        return false;
      }

      return true;
    }

    return false;
  #endif
  }

  void Process::closeStdin () noexcept {
  #if !SOCKET_RUNTIME_PLATFORM_IOS
    Lock lock(stdinMutex);

    if (stdinFD) {
      if (data.id > 0) {
        close(*stdinFD);
      }

      stdinFD.reset();
    }
  #endif
  }

  void Process::kill (PID id) noexcept {
  #if !SOCKET_RUNTIME_PLATFORM_IOS
    if (id <= 0 || ::kill(-id, 0) != 0) {
      return;
    }

    auto r = ::kill(-id, SIGTERM);

    if (r != 0 && ::kill(-id, 0) == 0) {
      r = ::kill(-id, SIGINT);

      if (r != 0 && ::kill(-id, 0) == 0 ) {
        r = ::kill(-id, SIGKILL);

        if (r != 0 && ::kill(-id, 0) == 0) {
          debug("Process: Failed to kill process %d", id);
        }
      }
    }
  #endif
  }
}
