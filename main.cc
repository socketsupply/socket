#include "./webview.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>
#include <uv.h>

static uv_loop_t *loop = uv_default_loop();
static uv_process_options_t options;

using msg_cb_t = std::function<void(const std::string)>;

class Process {
  void receiveData(uv_stream_t*, ssize_t, const uv_buf_t*);
  void receiveError(uv_stream_t*, ssize_t, const uv_buf_t*);
  void resumeStdout();
  void resumeStdin();
  void resumeStderr();
  uv_stream_t* getStdin();
  uv_stream_t* getStdout();
  uv_stream_t* getStderr();
  uv_pipe_t* stdin;
  uv_pipe_t* stdout;
  uv_pipe_t* stderr;
  uv_process_t* parent;
  std::thread run;

  public:
    void spawn(const char* file, const char* arg);
    void write(const std::string str);
    msg_cb_t onData;
    msg_cb_t onError;

    Process () noexcept :
      parent { nullptr },
      stdin { nullptr },
      stdout { nullptr },
      stderr { nullptr },
      onData { nullptr },
      onError { nullptr } {
    }
};

void Process::spawn (const char* file, const char* arg) {
  parent = new uv_process_t {};

  stdin = new uv_pipe_t {};
  stdin->data = this;
  uv_pipe_init(loop, stdin, 0);

  stdout = new uv_pipe_t {};
  stdout->data = this;
  uv_pipe_init(loop, stdout, 0);

  stderr = new uv_pipe_t {};
  stderr->data = this;
  uv_pipe_init(loop, stderr, 0);

  uv_stdio_container_t stdio[3];
  auto flags = uv_stdio_flags(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);

  stdio[0].data.stream = getStdin();
  stdio[0].flags = flags;
  stdio[1].data.stream = getStdout();
  stdio[1].flags = flags;
  stdio[2].data.stream = getStderr();
  stdio[2].flags = flags;

  char* args[3];
  args[0] = (char*) file;
  args[1] = (char*) arg;
  args[2] = NULL;

  options.file = file;
  options.args = args;
   options.stdio_count = 3;
  options.stdio = stdio;

  int status = uv_spawn(loop, this->parent, &options);

  if (status < 0) {
    // TODO cleanup
    return;
  }

  this->resumeStdout();
  this->resumeStderr();
  uv_run(loop, UV_RUN_DEFAULT);
}

void Process::receiveData (uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
  if (this->onData != nullptr) this->onData(buf->base);
}

void Process::receiveError (uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
  if (this->onError != nullptr) this->onError(buf->base);
}

uv_stream_t* Process::getStdin() {
  return reinterpret_cast<uv_stream_t*>(stdin);
}

uv_stream_t* Process::getStdout() {
  return reinterpret_cast<uv_stream_t*>(stdout);
}

uv_stream_t* Process::getStderr() {
  return reinterpret_cast<uv_stream_t*>(stderr);
}

void Process::resumeStdout () {
  uv_read_start(
    getStdout(),
    [] (uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);        
    },
    [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
      reinterpret_cast<Process*>(stream->data)->receiveData(stream, nread, buf);
    }
  );
}

void Process::resumeStderr () {
  uv_read_start(
    getStderr(),
    [] (uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
      *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);        
    },
    [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
      reinterpret_cast<Process*>(stream->data)->receiveError(stream, nread, buf);
    }
  );
}

void Process::write (const std::string str) {
  auto buf = uv_buf_init((char*) str.c_str(), str.size());

  uv_write_t* req = new uv_write_t {};
  req->data = &buf;

  uv_write(req, getStdin(), &buf, 1, [](uv_write_t* req, int status) {
    delete req;
  });
}

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
#else
#include <unistd.h>

int main()
#endif
{
  static auto process = std::make_unique<Process>();
  static auto win = std::make_unique<webview::webview>(true, nullptr);

  win->set_title("Operator");
  win->set_size(750, 520, WEBVIEW_HINT_NONE);

  win->bind("log", [](std::string s) -> std::string {
    std::cout << s << std::endl;
    return s;
  });

  win->bind("invokeIPC", [&](std::string s) -> std::string {
    process->write(s); // continue on to the child process.
    return "true"; // we can return stuff from C++ land.
  });

  process->onData = [] (const std::string str) {
    // std::cout << str << std::endl;
    win->emit("data", str);
  };

  char* buffer = getcwd(NULL, 0);

  if (buffer == NULL) {
    exit(1);
  }

  std::stringstream ss;
  ss << "file://";
  ss << buffer;
  ss << "/render.html";

  win->navigate(ss.str());

  std::thread main([&]() {
    process->spawn("node", "main.js");
  });

  win->run();
  main.join();

  free(buffer);
  return 0;
}
