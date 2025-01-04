#include "extension.hh"

using ssc::runtime::process::exec;
using ssc::runtime::string::trim;

const sapi_process_exec_t* sapi_process_exec (
  sapi_context_t* ctx,
  const char* command
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_exec is not supported on this platform");
  return nullptr;
#else

  if (ctx == nullptr) return nullptr;
  if (!ctx->isAllowed("process_exec")) {
    sapi_debug(ctx, "'process_exec' is not allowed.");
    return nullptr;
  }

  auto process = exec(command);
  process.output = trim(process.output);
  return ctx->memory.alloc<sapi_process_exec_t>(ctx, process);
#endif
}

int sapi_process_exec_get_exit_code (
  const sapi_process_exec_t* process
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_exec_get_exit_code is not supported on this platform");
  return -1;
#else
  return process != nullptr ? process->exitCode : -1;
#endif
}

const char* sapi_process_exec_get_output (
  const sapi_process_exec_t* process
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_exec_get_output is not supported on this platform");
  return nullptr;
#else
  return process != nullptr ? process->output.c_str() : nullptr;
#endif
}

sapi_process_spawn_t* sapi_process_spawn (
  sapi_context_t* ctx,
  const char* command,
  const char* argv,
  const char* path,
  sapi_process_spawn_stderr_callback_t onstdout,
  sapi_process_spawn_stderr_callback_t onstderr,
  sapi_process_spawn_exit_callback_t onexit
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_spawn is not supported on this platform");
  return nullptr;
#else
  auto process = ctx->memory.alloc<sapi_process_spawn_t>(
    ctx,
    command,
    argv,
    ssc::runtime::Vector<ssc::runtime::String>{},
    path,
    onstdout,
    onstderr,
    onexit
  );
  process->open();
  return process;
#endif
}

int sapi_process_spawn_get_exit_code (
  const sapi_process_spawn_t* process
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_spawn_get_exit_code is not supported on this platform");
  return -1;
#else
  return process != nullptr ? process->status.load() : -1;
#endif
}

unsigned long sapi_process_spawn_get_pid (
  const sapi_process_spawn_t* process
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_spawn_get_pid is not supported on this platform");
  return 0;
#else
  return process != nullptr ? process->id : 0;
#endif
}

sapi_context_t* sapi_process_spawn_get_context (
  const sapi_process_spawn_t* process
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_spawn_get_context is not supported on this platform");
  return nullptr;
#else
  return process != nullptr ? process->context : nullptr;
#endif
}

int sapi_process_spawn_wait (
  sapi_process_spawn_t* process
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_spawn_wait is not supported on this platform");
  return -1;
#else
  return process != nullptr ? process->wait() : -1;
#endif
}

bool sapi_process_spawn_write (
  sapi_process_spawn_t* process,
  const char* data,
  const size_t size
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_spawn_write is not supported on this platform");
  return false;
#else
  if (!process || process->closed) return false;
  process->write(data, size);
  return true;
#endif
}

bool sapi_process_spawn_close_stdin (
  sapi_process_spawn_t* process
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_spawn_close_stdin is not supported on this platform");
  return false;
#else
  if (!process || process->closed) return false;
  process->closeStdin();
  return true;
#endif
}

bool sapi_process_spawn_kill (
  sapi_process_spawn_t* process,
  int code
) {
#if SOCKET_RUNTIME_PLATFORM_IOS
  debug("sapi_process_spawn_kill is not supported on this platform");
  return false;
#else
  if (!process || process->closed) return false;
  process->kill(code);
  return true;
#endif
}
