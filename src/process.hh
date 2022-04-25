#ifndef OP_HPP_
#define OP_HPP_

#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

inline const std::vector<std::string>
splitc(const std::string& s, const char& c) {
  std::string buff;
  std::vector<std::string> vec;

  for (auto n : s) {
    if (n != c) {
      buff += n;
    } else if (n == c) {
      vec.push_back(buff);
      buff = "";
    }
  }

  vec.push_back(buff);

  return vec;
}

namespace Operator {
  using cb = std::function<void(std::string)>;

  // Additional parameters to Process constructors.
  struct Config {
    // Buffer size for reading stdout and stderr. Default is 131072 (128 kB).
    std::size_t buffer_size = 131072;
    // Set to true to inherit file descriptors from parent process. Default is false.
    // On Windows: has no effect unless read_stdout==nullptr, read_stderr==nullptr and open_stdin==false.
    bool inherit_file_descriptors = false;

    // On Windows only: controls how the process is started, mimics STARTUPINFO's wShowWindow.
    // See: https://docs.microsoft.com/en-us/windows/desktop/api/processthreadsapi/ns-processthreadsapi-startupinfoa
    // and https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-showwindow
    enum class ShowWindow {
      hide = 0,
      show_normal = 1,
      show_minimized = 2,
      maximize = 3,
      show_maximized = 3,
      show_no_activate = 4,
      show = 5,
      minimize = 6,
      show_min_no_active = 7,
      show_na = 8,
      restore = 9,
      show_default = 10,
      force_minimize = 11
    };
    // On Windows only: controls how the window is shown.
    ShowWindow show_window{ShowWindow::show_default};
  };

  // Platform independent class for creating processes.
  // Note on Windows: it seems not possible to specify which pipes to redirect.
  // Thus, at the moment, if read_stdout==nullptr, read_stderr==nullptr and open_stdin==false,
  // the stdout, stderr and stdin are sent to the parent process instead.
  class Process {
  public:
#ifdef _WIN32
    typedef unsigned long id_type; // Process id type
    typedef void *fd_type;         // File descriptor type
#else
    typedef pid_t id_type;
    typedef int fd_type;
    typedef std::string string_type;
#endif

  private:
    class Data {
    public:
      Data() noexcept;
      id_type id;
#ifdef _WIN32
      void *handle{nullptr};
#endif
      int exit_status{-1};
    };

  public:
    Process(
      const std::string &command,
      const std::string &argv,
      const std::string &path = std::string(""),
      cb read_stdout = nullptr,
      cb read_stderr = nullptr,
      bool open_stdin = true,
      const Config &config = {}) noexcept;

#ifndef _WIN32
    // Starts a process with the environment of the calling process.
    // Supported on Unix-like systems only.
    Process(
      const std::function<void()> &function,
      cb read_stdout = nullptr,
      cb read_stderr = nullptr,
      bool open_stdin = true,
      const Config &config = {}) noexcept;
#endif

    ~Process() noexcept {
      close_fds();
    };

    // Get the process id of the started process.
    id_type getPID() const noexcept {
      return data.id;
    }

    // Write to stdin.
    bool write(const char *bytes, size_t n);
    // Write to stdin. Convenience function using write(const char *, size_t).
    bool write(const std::string &str);
    // Close stdin. If the process takes parameters from stdin, use this to notify that all parameters have been sent.
    void close_stdin() noexcept;

    // Kill a given process id. Use kill(bool force) instead if possible. force=true is only supported on Unix-like systems.
    static void kill(id_type id) noexcept;

  private:
    Data data;
    bool closed;
    std::mutex close_mutex;
    cb read_stdout;
    cb read_stderr;
#ifndef _WIN32
    std::thread stdout_stderr_thread;
#else
    std::thread stdout_thread, stderr_thread;
#endif
    bool open_stdin;
    std::mutex stdin_mutex;

    Config config;

    std::unique_ptr<fd_type> stdout_fd, stderr_fd, stdin_fd;

    id_type open(const std::string &command, const std::string &path) noexcept;
#ifndef _WIN32
    id_type open(const std::function<void()> &function) noexcept;
#endif
    void read() noexcept;
    void close_fds() noexcept;
  };

  inline bool Process::write(const std::string &s) {
    return Process::write(s.c_str(), s.size());
  };

  inline Process::Process(
    const std::string &command,
    const std::string &argv,
    const std::string &path,
    cb read_stdout,
    cb read_stderr,
    bool open_stdin,
    const Config &config) noexcept
      : closed(true),
        open_stdin(true),
        read_stdout(std::move(read_stdout)),
        read_stderr(std::move(read_stderr)) {
    if (command.size() == 0) return;
    open(command + argv, path);
    read();
  }

} // namespace Operator

#endif // OP_HPP_
