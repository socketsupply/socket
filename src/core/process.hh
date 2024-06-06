#ifndef SOCKET_RUNTIME_CORE_PROCESS_H
#define SOCKET_RUNTIME_CORE_PROCESS_H

#include "../platform/platform.hh"

#if !SOCKET_RUNTIME_PLATFORM_IOS
#include <iostream>
#endif

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
    // On Windows: has no effect unless readStdout==nullptr, readStderr==nullptr and openStdin==false.
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

#if !SOCKET_RUNTIME_PLATFORM_IOS
  inline ExecOutput exec (String command) {
    command = command + " 2>&1";

    ExecOutput eo;
    FILE* pipe;
    size_t count;
    int exitCode = 0;
    const int bufsize = 128;
    Array<char, 128> buffer;

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
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

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
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
#endif

  // Platform independent class for creating processes.
  // Note on Windows: it seems not possible to specify which pipes to redirect.
  // Thus, at the moment, if readStdout==nullptr, readStderr==nullptr and openStdin==false,
  // the stdout, stderr and stdin are sent to the parent process instead.
  class Process {
  public:
    static constexpr auto PROCESS_WAIT_TIMEOUT = 256;

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    typedef unsigned long PID; // process id (pid) type
    typedef void *FD; // file descriptor type
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    typedef int PID;
    typedef int FD;
  #else
    typedef pid_t PID;
    typedef int FD;
  #endif

    String command;
    String argv;
    String path;
    Vector<String> env;
    Atomic<bool> closed = true;
    Atomic<int> status = -1;
    Atomic<int> lastWriteStatus = 0;
    bool detached = false;
    bool openStdin;
    PID id = 0;

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    String shell = "";
  #else
    String shell = "/bin/sh";
  #endif

  private:

    class Data {
      public:
        PID id;
        int exitStatus = -1;
      #if SOCKET_RUNTIME_PLATFORM_WINDOWS
        void* handle = nullptr;
      #endif

        Data() noexcept;
    };

  public:
    Process(
      const String &command,
      const String &argv,
      const SSC::Vector<SSC::String> &env,
      const String &path = String(""),
      MessageCallback readStdout = nullptr,
      MessageCallback readStderr = nullptr,
      MessageCallback onExit = nullptr,
      bool openStdin = true,
      const ProcessConfig &config = {}
    ) noexcept;

    Process(
      const String &command,
      const String &argv,
      const String &path = String(""),
      MessageCallback readStdout = nullptr,
      MessageCallback readStderr = nullptr,
      MessageCallback onExit = nullptr,
      bool openStdin = true,
      const ProcessConfig &config = {}
    ) noexcept;

    #if !SOCKET_RUNTIME_PLATFORM_WINDOWS
      // Starts a process with the environment of the calling process.
      // Supported on Unix-like systems only.
      Process (
        const std::function<int()> &function,
        MessageCallback readStdout = nullptr,
        MessageCallback readStderr = nullptr,
        MessageCallback onExit = nullptr,
        bool openStdin = true,
        const ProcessConfig &config = {}
      ) noexcept;
    #endif

    ~Process () noexcept {
      closeFDs();
    };

    // Get the process id of the started process.
    PID getPID () const noexcept {
      return data.id;
    }

    // Write to stdin.
    bool write (const char *bytes, size_t size);
    bool write (const SharedPointer<char[]> bytes, size_t size) {
      return write(bytes.get(), size);
    }

    // Write to stdin. Convenience function using write(const char *, size_t).
    bool write (const String &string) {
      return write(string.c_str(), string.size());
    }

    // Close stdin. If the process takes parameters from stdin, use this to
    // notify that all parameters have been sent.
    void closeStdin () noexcept;
    PID open () noexcept {
      if (this->command.size() == 0) return 0;
      auto str = trim(this->command + " " + this->argv);
      auto pid = open(str, this->path);
      read();
      return pid;
    }

    void kill (PID id) noexcept;
    void kill () noexcept {
      this->kill(this->getPID());
    }

    int wait ();

  private:
    Data data;
    std::mutex closeMutex;
    MessageCallback readStdout;
    MessageCallback readStderr;
    MessageCallback onExit;
    Mutex stdinMutex;
    Mutex stdoutMutex;
    Mutex stderrMutex;
    ProcessConfig config;

  #if !SOCKET_RUNTIME_PLATFORM_WINDOWS
    Thread stdoutAndStderrThread;
  #else
    Thread stdoutThread, stderrThread;
  #endif

    UniquePointer<FD> stdoutFD, stderrFD, stdinFD;

    void read () noexcept;
    void closeFDs () noexcept;
    PID open (const String &command, const String &path) noexcept;
  #if !SOCKET_RUNTIME_PLATFORM_WINDOWS
    PID open (const Function<int()> &function) noexcept;
  #endif
  };
}
#endif
