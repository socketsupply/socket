#ifndef SSC_PROCESS_PROCESS_H
#define SSC_PROCESS_PROCESS_H

#include <iostream>

#include "../core/types.hh"
#include "../core/string.hh"
#include "../core/platform.hh"

#ifndef WIFEXITED
#define WIFEXITED(w) ((w) & 0x7f)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(w) (((w) & 0xff00) >> 8)
#endif

namespace SSC {
  struct ExecOutput {
    String output;
    int exitCode = 0;
  };

  // Additional parameters to Process constructors.
  struct ProcessConfig {
    // Buffer size for reading stdout and stderr. Default is 131072 (128 kB).
    size_t bufferSize = 131072;
    // Set to true to inherit file descriptors from parent process. Default is false.
    // On Windows: has no effect unless read_stdout==nullptr, read_stderr==nullptr and open_stdin==false.
    bool inheritFDs = false;

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

  inline ExecOutput exec (String command) {
    command = command + " 2>&1";

    ExecOutput eo;
    FILE* pipe;
    size_t count;
    int exitCode = 0;
    const int bufsize = 128;
    Array<char, 128> buffer;

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
    static constexpr auto PROCESS_WAIT_TIMEOUT = 256;

  #if SSC_PLATFORM_WINDOWS
    typedef unsigned long PID; // process id (pid) type
    typedef void *FD; // file descriptor type
  #else
    typedef pid_t PID;
    typedef int FD;
    typedef String string_type;
  #endif

    String command;
    String argv;
    String path;
    Atomic<bool> closed = true;
    Atomic<int> status = -1;
    Atomic<int> lastWriteStatus = 0;
    bool open_stdin;
    PID id = 0;

  #if SSC_PLATFORM_WINDOWS
    String shell = "";
  #else
    String shell = "/bin/sh";
  #endif

  private:

    class Data {
    public:
      Data() noexcept;
      PID id;
    #if SSC_PLATFORM_WINDOWS
      void *handle {nullptr};
    #endif
      int exit_status{-1};
    };

  public:
    Process(
      const String &command,
      const String &argv,
      const String &path = String(""),
      MessageCallback read_stdout = nullptr,
      MessageCallback read_stderr = nullptr,
      MessageCallback on_exit = nullptr,
      bool open_stdin = true,
      const ProcessConfig &config = {}) noexcept;

  #if !SSC_PLATFORM_WINDOWS
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

    ~Process () noexcept {
      close_fds();
    };

    // Get the process id of the started process.
    PID getPID () const noexcept {
      return data.id;
    }

    // Write to stdin.
    bool write (const char *bytes, size_t size);
    bool write (const SharedPointer<char*> bytes, size_t size) {
      return write(*bytes, size);
    }

    // Write to stdin. Convenience function using write(const char *, size_t).
    bool write (const String &string) {
      return write(string.c_str(), string.size());
    }

    // Close stdin. If the process takes parameters from stdin, use this to
    // notify that all parameters have been sent.
    void close_stdin () noexcept;
    PID open () noexcept {
      if (this->command.size() == 0) return 0;
      auto str = trim(this->command + " " + this->argv);
      auto pid = open(str, this->path);
      read();
      return pid;
    }

    // Kill a given process id. Use kill(bool force) instead if possible.
    // force=true is only supported on Unix-like systems.
    void kill (PID id) noexcept;
    void kill () noexcept {
      this->kill(this->getPID());
    }

    int wait ();

  private:
    Data data;
    std::mutex close_mutex;
    MessageCallback read_stdout;
    MessageCallback read_stderr;
    MessageCallback on_exit;
    Mutex stdin_mutex;
    Mutex stdout_mutex;
    Mutex stderr_mutex;
    ProcessConfig config;

  #if !SSC_PLATFORM_WINDOWS
    Thread stdout_stderr_thread;
  #else
    Thread stdout_thread, stderr_thread;
  #endif

    UniquePointer<FD> stdout_fd, stderr_fd, stdin_fd;

    void read () noexcept;
    void close_fds () noexcept;
    PID open (const String &command, const String &path) noexcept;
  #if !SSC_PLATFORM_WINDOWS
    PID open (const Function<int()> &function) noexcept;
  #endif
  };
}
#endif
