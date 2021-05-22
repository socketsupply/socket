#include <uv.h>
#include <sstream>
#include <iostream>
#include "webview.h"

static uv_loop_t *loop = uv_default_loop();
static uv_process_options_t options;

using msg_cb_t = std::function<void(const std::string)>;

class Process {
  void receiveData(const std::string);
  void receiveError(const std::string);
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
    void kill();
    void spawn(const char* file, const char* arg, const char* cwd);
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

void Process::kill () {
  delete stdin;
  delete stdout;
  delete stderr;
  delete parent;
  uv_stop(loop);
}

void Process::spawn (const char* file, const char* arg, const char* cwd) {
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
  options.cwd = cwd;
  options.stdio_count = 3;
  options.stdio = stdio;

  int status = uv_spawn(loop, this->parent, &options);

  if (status < 0) {
    return;
  }

  this->resumeStdout();
  this->resumeStderr();
  
  uv_run(loop, UV_RUN_DEFAULT);
}

void Process::receiveData (const std::string s) {
  if (this->onData != nullptr) this->onData(s);
}

void Process::receiveError (const std::string s) {
  if (this->onError != nullptr) this->onError(s);
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
      std::string s(buf->base);
      reinterpret_cast<Process*>(stream->data)->receiveData(s);
      delete[] buf->base;

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
      const std::string s(buf->base);
      reinterpret_cast<Process*>(stream->data)->receiveError(s);

      delete[] buf->base;
    }
  );
}

//
// On linux, this seems to cause an error, but only *sometimes*...
// operator: src/unix/stream.c:743: uv__write_req_update: Assertion `n <= stream->write_queue_size' failed.
//
// running gdb, i see a stack trace where just before the assert.c happens,
// there is a call to uv_write2, however, after throttling the calls to write,
// i stop seeing the error. Does write_queue_size have some kind of
// back pressure?
//

void Process::write (const std::string str) {
  uv_write_t* req = new uv_write_t {};

  auto buf = uv_buf_init((char*) str.c_str(), str.size());
  req->data = &buf;

  uv_write(req, getStdin(), &buf, 1, +[](uv_write_t* req, int status) {
    // auto& data = reinterpret_cast<char*>(req->data);
    delete req;

    if (status != 0) {
      // TODO better write error handling
      std::cout
        << "uv_write error: "
        << uv_err_name(status)
        << std::endl;
    }
  });
}
