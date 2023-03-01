#include "process.hh"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <tlhelp32.h>
#include <limits.h>

namespace SSC {

SSC::String FormatError(DWORD error, SSC::String source) {
  SSC::StringStream message;
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  FormatMessage(
  FORMAT_MESSAGE_ALLOCATE_BUFFER | 
  FORMAT_MESSAGE_FROM_SYSTEM |
  FORMAT_MESSAGE_IGNORE_INSERTS,
  NULL,
  error,
  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
  (LPTSTR) &lpMsgBuf,
  0, NULL );

  message << "Error " << error << " in " << source << ": " <<  (LPTSTR)lpMsgBuf;
  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);

  return message.str();
}

const static SSC::StringStream initial;

Process::Data::Data() noexcept : id(0) {}

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

// Simple HANDLE wrapper to close it automatically from the destructor.
class Handle {
  public:
    Handle() noexcept : handle(INVALID_HANDLE_VALUE) {}

    ~Handle() noexcept {
      close();
    }

    void close() noexcept {
      if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
      }
    }

    HANDLE detach() noexcept {
      HANDLE old_handle = handle;
      handle = INVALID_HANDLE_VALUE;
      return old_handle;
    }

    operator HANDLE() const noexcept { return handle; }
    HANDLE *operator&() noexcept { return &handle; }

  private:
    HANDLE handle;
};

//Based on the discussion thread: https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxq1wsj
std::mutex create_process_mutex;

//Based on the example at https://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx.
Process::id_type Process::open(const SSC::String &command, const SSC::String &path) noexcept {
  if (open_stdin) {
    stdin_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));
  }

  if (read_stdout) {
    stdout_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));
  }

  if (read_stderr) {
    stderr_fd = std::unique_ptr<fd_type>(new fd_type(nullptr));
  }

  Handle stdin_rd_p;
  Handle stdin_wr_p;
  Handle stdout_rd_p;
  Handle stdout_wr_p;
  Handle stderr_rd_p;
  Handle stderr_wr_p;

  SECURITY_ATTRIBUTES security_attributes;

  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.bInheritHandle = TRUE;
  security_attributes.lpSecurityDescriptor = nullptr;

  std::lock_guard<std::mutex> lock(create_process_mutex);

  if (stdin_fd) {
    if (!CreatePipe(&stdin_rd_p, &stdin_wr_p, &security_attributes, 0) ||
       !SetHandleInformation(stdin_wr_p, HANDLE_FLAG_INHERIT, 0))
      return 0;
  }

  if (stdout_fd) {
    if (!CreatePipe(&stdout_rd_p, &stdout_wr_p, &security_attributes, 0) ||
       !SetHandleInformation(stdout_rd_p, HANDLE_FLAG_INHERIT, 0)) {
      return 0;
    }
  }

  if (stderr_fd) {
    if (!CreatePipe(&stderr_rd_p, &stderr_wr_p, &security_attributes, 0) ||
       !SetHandleInformation(stderr_rd_p, HANDLE_FLAG_INHERIT, 0)) {
      return 0;
    }
  }

  PROCESS_INFORMATION process_info;
  STARTUPINFO startup_info;

  ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));

  ZeroMemory(&startup_info, sizeof(STARTUPINFO));
  startup_info.cb = sizeof(STARTUPINFO);
  startup_info.hStdInput = stdin_rd_p;
  startup_info.hStdOutput = stdout_wr_p;
  startup_info.hStdError = stderr_wr_p;

  if (stdin_fd || stdout_fd || stderr_fd)
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

  if (config.show_window != ProcessConfig::ShowWindow::show_default) {
    startup_info.dwFlags |= STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = static_cast<WORD>(config.show_window);
  }

  auto process_command = command;
#ifdef MSYS_PROCESS_USE_SH
  size_t pos = 0;
  while((pos = process_command.find('\\', pos)) != string_type::npos) {
    process_command.replace(pos, 1, "\\\\\\\\");
    pos += 4;
  }
  pos = 0;
  while((pos = process_command.find('\"', pos)) != string_type::npos) {
    process_command.replace(pos, 1, "\\\"");
    pos += 2;
  }
  process_command.insert(0, "sh -c \"");
  process_command += "\"";
#endif

  BOOL bSuccess = CreateProcess(
    nullptr,
    process_command.empty() ? nullptr : &process_command[0],
    nullptr,
    nullptr,
    stdin_fd || stdout_fd || stderr_fd || config.inherit_file_descriptors, // Cannot be false when stdout, stderr or stdin is used
    stdin_fd || stdout_fd || stderr_fd ? CREATE_NO_WINDOW : 0,             // CREATE_NO_WINDOW cannot be used when stdout or stderr is redirected to parent process
    nullptr,
    path.empty() ? nullptr : path.c_str(),
    &startup_info,
    &process_info
  );

  if (!bSuccess) {
    auto msg = SSC::String("Unable to execute: " + process_command);
    MessageBoxA(nullptr, &msg[0], "Alert", MB_OK | MB_ICONSTOP);
    return 0;
  } else {
    CloseHandle(process_info.hThread);
  }

  if (stdin_fd) {
    *stdin_fd = stdin_wr_p.detach();
  }

  if (stdout_fd) {
    *stdout_fd = stdout_rd_p.detach();
  }

  if (stderr_fd) {
    *stderr_fd = stderr_rd_p.detach();
  }

  auto processHandle = process_info.hProcess;
  auto t = std::thread([&](HANDLE _processHandle) {
    DWORD exitCode = 0;
    try {
      WaitForSingleObject(_processHandle, INFINITE);

      if (GetExitCodeProcess(_processHandle, &exitCode) == 0) {
        std::cerr << FormatError(GetLastError(), "SSC::Process::open() GetExitCodeProcess()") << std::endl;
        exitCode = -1;
      }

      if (this->closed)
      {
        std::cout << "Process killed. " << exitCode << std::endl;
        return;
      }

      this->status = (exitCode <= UINT_MAX ? exitCode : WEXITSTATUS(exitCode));
      this->closed = true;
      if (this->on_exit != nullptr)
        this->on_exit(std::to_string(this->status));
    } catch (std::exception e) {
      std::cerr << "SSC::Process thread exception: " << e.what() << std::endl;
      this->closed = true;
    }
  }, processHandle);

  t.detach();

  closed = false;
  id = process_info.dwProcessId;

  data.id = process_info.dwProcessId;
  data.handle = process_info.hProcess;

  return process_info.dwProcessId;
}

void Process::read() noexcept {
  if (data.id == 0) {
    return;
  }

  if (stdout_fd) {
    stdout_thread = std::thread([this]() {
      DWORD n;

      std::unique_ptr<char[]> buffer(new char[config.buffer_size]);
      SSC::StringStream ss;

      for (;;) {
        memset(buffer.get(), 0, config.buffer_size);
        BOOL bSuccess = ReadFile(*stdout_fd, static_cast<CHAR *>(buffer.get()), static_cast<DWORD>(config.buffer_size), &n, nullptr);

        if (!bSuccess || n == 0) {
          break;
        }

        auto b = SSC::String(buffer.get());
        auto parts = splitc(b, '\n');

        if (parts.size() > 1) {
          std::lock_guard<std::mutex> lock(stdout_mutex);

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
      }
    });
  }

  if (stderr_fd) {
    stderr_thread = std::thread([this]() {
      DWORD n;
      std::unique_ptr<char[]> buffer(new char[config.buffer_size]);

      for (;;) {
        BOOL bSuccess = ReadFile(*stderr_fd, static_cast<CHAR *>(buffer.get()), static_cast<DWORD>(config.buffer_size), &n, nullptr);
        if (!bSuccess || n == 0) break;
        std::lock_guard<std::mutex> lock(stderr_mutex);
        read_stderr(SSC::String(buffer.get()));
      }
    });
  }
}

void Process::close_fds() noexcept {
  if (stdout_thread.joinable()) {
    stdout_thread.join();
  }

  if (stderr_thread.joinable()) {
    stderr_thread.join();
  }

  if (stdin_fd) {
    close_stdin();
  }

  if (stdout_fd) {
    if (*stdout_fd != nullptr) {
      CloseHandle(*stdout_fd);
    }

    stdout_fd.reset();
  }

  if (stderr_fd) {
    if (*stderr_fd != nullptr) {
      CloseHandle(*stderr_fd);
    }

    stderr_fd.reset();
  }
}

bool Process::write(const char *bytes, size_t n) {
  if (!open_stdin) {
    throw std::invalid_argument("Can't write to an unopened stdin pipe. Please set open_stdin=true when constructing the process.");
  }

  std::lock_guard<std::mutex> lock(stdin_mutex);
  if (stdin_fd) {
    SSC::String b(bytes);

    while (true && (b.size() > 0)) {
      DWORD bytesWritten;
      DWORD size = static_cast<DWORD>(b.size());
      BOOL bSuccess = WriteFile(*stdin_fd, b.c_str(), size, &bytesWritten, nullptr);

      if (bytesWritten >= size || bSuccess) {
        break;
      }

      b = b.substr(bytesWritten / 2, b.size());
    }

    DWORD bytesWritten;
    BOOL bSuccess = WriteFile(*stdin_fd, L"\n", static_cast<DWORD>(2), &bytesWritten, nullptr);

    if (!bSuccess || bytesWritten == 0) {
      return false;
    } else {
      return true;
    }
  }

  return false;
}

void Process::close_stdin() noexcept {
  std::lock_guard<std::mutex> lock(stdin_mutex);

  if (stdin_fd) {
    if (*stdin_fd != nullptr) {
      CloseHandle(*stdin_fd);
    }

    stdin_fd.reset();
  }
}

//Based on http://stackoverflow.com/a/1173396
void Process::kill(id_type id) noexcept {
  if (id == 0) {
    return;
  }

  this->closed = true;

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (snapshot) {
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(process));
    process.dwSize = sizeof(process);

    if (Process32First(snapshot, &process)) {
      do {
        if (process.th32ParentProcessID == id) {
          HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, process.th32ProcessID);

          if (process_handle) {
            TerminateProcess(process_handle, 2);
            CloseHandle(process_handle);
          }
        }
      } while (Process32Next(snapshot, &process));
    }

    CloseHandle(snapshot);
  }

  HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, id);

  if (process_handle) {
    TerminateProcess(process_handle, 2);
  }
}
} // namespace SSC
