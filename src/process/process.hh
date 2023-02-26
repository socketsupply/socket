#ifndef SSC_PROCESS_PROCESS_H
#define SSC_PROCESS_PROCESS_H

#ifndef WIFEXITED
#define WIFEXITED(w) ((w) & 0x7f)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(w) (((w) & 0xff00) >> 8)
#endif

#include "../common.hh"

namespace SSC {
  struct ExecOutput {
    SSC::String output;
    int exitCode = 0;
  };

  // Additional parameters to Process constructors.
  struct ProcessConfig {
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

  inline ExecOutput exec (SSC::String command) {
    command = command + " 2>&1";

    ExecOutput eo;
    FILE* pipe;
    size_t count;
    int exitCode = 0;
    const int bufsize = 128;
    std::array<char, 128> buffer;

    #ifdef _WIN32
      //
      // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen?view=msvc-160
      // _popen works fine in a console application... ok fine that's all we need it for... thanks.
      //
      pipe = _popen((const char*) command.c_str(), "rt");
    #else
      pipe = popen((const char*) command.c_str(), "r");
    #endif

    if (pipe == NULL) {
      std::cout << "error: unable to open the command" << std::endl;
      exit(1);
    }

    do {
      if ((count = fread(buffer.data(), 1, bufsize, pipe)) > 0) {
        eo.output.insert(eo.output.end(), std::begin(buffer), std::next(std::begin(buffer), count));
      }
    } while (count > 0);

    #ifdef _WIN32
      exitCode = _pclose(pipe);
    #else
      exitCode = pclose(pipe);
    #endif

    if (!WIFEXITED(exitCode) || exitCode != 0) {
      auto status = WEXITSTATUS(exitCode);
      if (status && exitCode) {
        exitCode = status;
      }
    }

    eo.exitCode = exitCode;

    return eo;
  }

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
    typedef SSC::String string_type;
  #endif

    SSC::String command;
    SSC::String argv;
    SSC::String path;
    std::atomic<bool> closed = true;
    std::atomic<int> status = -1;
    id_type id = 0;

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
      const SSC::String &command,
      const SSC::String &argv,
      const SSC::String &path = SSC::String(""),
      MessageCallback read_stdout = nullptr,
      MessageCallback read_stderr = nullptr,
      MessageCallback on_exit = nullptr,
      bool open_stdin = true,
      const ProcessConfig &config = {}) noexcept;

#ifndef _WIN32
    // Starts a process with the environment of the calling process.
    // Supported on Unix-like systems only.
    Process(
      const std::function<int()> &function,
      MessageCallback read_stdout = nullptr,
      MessageCallback read_stderr = nullptr,
      MessageCallback on_exit = nullptr,
      bool open_stdin = true,
      const ProcessConfig &config = {}) noexcept;
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
    bool write(const SSC::String &s) {
      return write(s.c_str(), s.size());
    }
    // Close stdin. If the process takes parameters from stdin, use this to
    // notify that all parameters have been sent.
    void close_stdin() noexcept;
    id_type open() noexcept {
      if (this->command.size() == 0) return 0;
      auto pid = open(this->command + this->argv, this->path);
      read();
      return pid;
    }

    // Kill a given process id. Use kill(bool force) instead if possible.
    // force=true is only supported on Unix-like systems.
    void kill(id_type id) noexcept;

    int wait () {
      do {
        std::this_thread::yield();
      } while (this->closed == false);

      return this->status;
    }

  private:
    Data data;
    std::mutex close_mutex;
    MessageCallback read_stdout;
    MessageCallback read_stderr;
    MessageCallback on_exit;
#ifndef _WIN32
    std::thread stdout_stderr_thread;
#else
    std::thread stdout_thread, stderr_thread;
#endif
    bool open_stdin;
    std::mutex stdin_mutex;
    std::mutex stdout_mutex;
    std::mutex stderr_mutex;

    ProcessConfig config;

    std::unique_ptr<fd_type> stdout_fd, stderr_fd, stdin_fd;

    id_type open(const SSC::String &command, const SSC::String &path) noexcept;
#ifndef _WIN32
    id_type open(const std::function<int()> &function) noexcept;
#endif
    void read() noexcept;
    void close_fds() noexcept;
  };

} // namespace SSC

#endif // SSC_HPP_
