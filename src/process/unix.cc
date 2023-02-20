#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <set>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

#include "process.hh"

namespace SSC {

static StringStream initial;

Process::Data::Data() noexcept : id(-1) {}
Process::Process(
  const String &command,
  const String &argv,
  const String &path,
  MessageCallback read_stdout,
  MessageCallback read_stderr,
  MessageCallback on_exit,
  bool open_stdin,
  const ProcessConfig &config
) noexcept :
  open_stdin(true),
  read_stdout(std::move(read_stdout)),
  read_stderr(std::move(read_stderr)),
  on_exit(std::move(on_exit))
{
  this->command = command;
  this->argv = argv;
  this->path = path;
}

Process::Process(
  const std::function<int()> &function,
  MessageCallback read_stdout,
  MessageCallback read_stderr,
  MessageCallback on_exit,
  bool open_stdin, const ProcessConfig &config
) noexcept:
  read_stdout(std::move(read_stdout)),
  read_stderr(std::move(read_stderr)),
  on_exit(std::move(on_exit)),
  open_stdin(open_stdin),
  config(config)
{
  open(function);
  read();
}

Process::id_type Process::open(const std::function<int()> &function) noexcept {
  if (open_stdin) {
    stdin_fd = std::unique_ptr<fd_type>(new fd_type);
  }

  if (read_stdout) {
    stdout_fd = std::unique_ptr<fd_type>(new fd_type);
  }

  if (read_stderr) {
    stderr_fd = std::unique_ptr<fd_type>(new fd_type);
  }

  int stdin_p[2];
  int stdout_p[2];
  int stderr_p[2];

  if (stdin_fd && pipe(stdin_p) != 0) {
    return -1;
  }

  if (stdout_fd && pipe(stdout_p) != 0) {
    if (stdin_fd) {
      close(stdin_p[0]);
      close(stdin_p[1]);
    }
    return -1;
  }

  if (stderr_fd && pipe(stderr_p) != 0) {
    if (stdin_fd) {
      close(stdin_p[0]);
      close(stdin_p[1]);
    }

    if (stdout_fd) {
      close(stdout_p[0]);
      close(stdout_p[1]);
    }

    return -1;
  }

  id_type pid = fork();

  if (pid < 0) {
    if (stdin_fd) {
      close(stdin_p[0]);
      close(stdin_p[1]);
    }

    if (stdout_fd) {
      close(stdout_p[0]);
      close(stdout_p[1]);
    }

    if (stderr_fd) {
      close(stderr_p[0]);
      close(stderr_p[1]);
    }

    return pid;
  }

  closed = false;
  id = pid;

  if (pid > 0) {
    auto thread = std::thread([this] {
      int code = 0;
      waitpid(this->id, &code, 0);

      this->status = WEXITSTATUS(code);
      this->closed = true;

      if (this->on_exit != nullptr) {
        this->on_exit(std::to_string(status));
      }
    });

    thread.detach();
  } else if (pid == 0) {
    if (stdin_fd) {
      dup2(stdin_p[0], 0);
    }

    if (stdout_fd) {
      dup2(stdout_p[1], 1);
    }

    if (stderr_fd) {
      dup2(stderr_p[1], 2);
    }

    if (stdin_fd) {
      close(stdin_p[0]);
      close(stdin_p[1]);
    }

    if (stdout_fd) {
      close(stdout_p[0]);
      close(stdout_p[1]);
    }

    if (stderr_fd) {
      close(stderr_p[0]);
      close(stderr_p[1]);
    }

    setpgid(0, 0);

    if (function) {
      function();
    }

    _exit(EXIT_FAILURE);
  }

  if (stdin_fd) {
    close(stdin_p[0]);
  }

  if (stdout_fd) {
    close(stdout_p[1]);
  }

  if (stderr_fd) {
    close(stderr_p[1]);
  }

  if (stdin_fd) {
    *stdin_fd = stdin_p[1];
  }

  if (stdout_fd) {
    *stdout_fd = stdout_p[0];
  }

  if (stderr_fd) {
    *stderr_fd = stderr_p[0];
  }

  data.id = pid;
  return pid;
}

Process::id_type Process::open(const SSC::String &command, const SSC::String &path) noexcept {
  return open([&command, &path] {
    auto command_c_str = command.c_str();
    SSC::String cd_path_and_command;

    if (!path.empty()) {
      auto path_escaped = path;
      size_t pos = 0;

      // Based on https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxsxyb7
      while((pos = path_escaped.find('\'', pos)) != SSC::String::npos) {
        path_escaped.replace(pos, 1, "'\\''");
        pos += 4;
      }

      cd_path_and_command = "cd '" + path_escaped + "' && " + command; // To avoid resolving symbolic links
      command_c_str = cd_path_and_command.c_str();
    }

    return execl("/bin/sh", "/bin/sh", "-c", command_c_str, nullptr);
  });
}

void Process::read() noexcept {
  if (data.id <= 0 || (!stdout_fd && !stderr_fd)) {
    return;
  }

  stdout_stderr_thread = std::thread([this] {
    std::vector<pollfd> pollfds;
    std::bitset<2> fd_is_stdout;

    if (stdout_fd) {
      fd_is_stdout.set(pollfds.size());
      pollfds.emplace_back();
      pollfds.back().fd = fcntl(*stdout_fd, F_SETFL, fcntl(*stdout_fd, F_GETFL) | O_NONBLOCK) == 0 ? *stdout_fd : -1;
      pollfds.back().events = POLLIN;
    }

    if (stderr_fd) {
      pollfds.emplace_back();
      pollfds.back().fd = fcntl(*stderr_fd, F_SETFL, fcntl(*stderr_fd, F_GETFL) | O_NONBLOCK) == 0 ? *stderr_fd : -1;
      pollfds.back().events = POLLIN;
    }

    auto buffer = std::unique_ptr<char[]>(new char[config.buffer_size]);
    bool any_open = !pollfds.empty();
    SSC::StringStream ss;

    while (any_open && (poll(pollfds.data(), static_cast<nfds_t>(pollfds.size()), -1) > 0 || errno == EINTR)) {
      any_open = false;

      for (size_t i = 0; i < pollfds.size(); ++i) {
        if (!(pollfds[i].fd >= 0)) continue;

        if (pollfds[i].revents & POLLIN) {
          memset(buffer.get(), 0, config.buffer_size);
          const ssize_t n = ::read(pollfds[i].fd, buffer.get(), config.buffer_size);

          if (n > 0) {
            if (fd_is_stdout[i]) {
              std::lock_guard<std::mutex> lock(stdout_mutex);
              auto b = SSC::String(buffer.get());
              auto parts = splitc(b, '\n');

              if (parts.size() > 1) {
                for (int i = 0; i < parts.size() - 1; i++) {
                  ss << parts[i];

                  SSC::String s(ss.str());
                  read_stdout(s);

                  ss.str(SSC::String());
                  ss.clear();
                  ss.copyfmt(initial);
                }
                ss << parts[parts.size() - 1];
              } else {
                ss << b;
              }
            } else {
              std::lock_guard<std::mutex> lock(stderr_mutex);
              read_stderr(SSC::String(buffer.get()));
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
}

void Process::close_fds() noexcept {
  if (stdout_stderr_thread.joinable()) {
    stdout_stderr_thread.join();
  }

  if (stdin_fd) {
    close_stdin();
  }

  if (stdout_fd) {
    if (data.id > 0) {
      close(*stdout_fd);
    }

    stdout_fd.reset();
  }

  if (stderr_fd) {
    if (data.id > 0) {
      close(*stderr_fd);
    }

    stderr_fd.reset();
  }
}

bool Process::write(const char *bytes, size_t n) {
  std::lock_guard<std::mutex> lock(stdin_mutex);

  if (stdin_fd) {
    SSC::String b(bytes);

    while (true && (b.size() > 0)) {
      int bytesWritten = ::write(*stdin_fd, b.c_str(), b.size());

      if (bytesWritten >= b.size()) {
        break;
      }

      b = b.substr(bytesWritten, b.size());
    }

    int bytesWritten = ::write(*stdin_fd, "\n", 1);
  }

  return false;
}

void Process::close_stdin() noexcept {
  std::lock_guard<std::mutex> lock(stdin_mutex);

  if (stdin_fd) {
    if (data.id > 0) {
      close(*stdin_fd);
    }

    stdin_fd.reset();
  }
}

void Process::kill(id_type id) noexcept {
  if (id <= 0) {
    return;
  }

  this->closed = true;
  auto r = ::kill(-id, SIGINT);

  if (r != 0) {
    r = ::kill(-id, SIGTERM);

    if (r != 0) {
      r = ::kill(-id, SIGKILL);

      if (r != 0) {
        // @TODO: print warning
      }
    }
  }
}

} // namespace SSC
