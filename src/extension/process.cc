#include "extension.hh"

const sapi_process_exec_t* sapi_process_exec (
  sapi_context_t* ctx,
  const char* command
) {
  if (ctx == nullptr) return nullptr;
  if (!ctx->isAllowed("process_exec")) {
    sapi_debug(ctx, "'process_exec' is not allowed.");
    return nullptr;
  }
  auto process = SSC::exec(command);
  process.output = SSC::trim(process.output);
  return ctx->memory.alloc<sapi_process_exec_t>(ctx, process);
}

const int sapi_process_exec_get_exit_code (
  const sapi_process_exec_t* process
) {
  return process != nullptr ? process->exitCode : -1;
}

const char* sapi_process_exec_get_output (
  const sapi_process_exec_t* process
) {
  return process != nullptr ? process->output.c_str() : nullptr;
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
  auto process = ctx->memory.alloc<sapi_process_spawn_t>(
    ctx,
    command,
    argv,
    path,
    onstdout,
    onstderr,
    onexit
  );
  process->open();
  return process;
}

const int sapi_process_spawn_get_exit_code (
  const sapi_process_spawn_t* process
) {
  return process != nullptr ? process->status.load() : -1;
}

const unsigned long sapi_process_spawn_get_pid (
  const sapi_process_spawn_t* process
) {
  return process != nullptr ? process->id : 0;
}

sapi_context_t* sapi_process_spawn_get_context (
  const sapi_process_spawn_t* process
) {
  return process != nullptr ? process->context : nullptr;
}

const int sapi_process_spawn_wait (
  sapi_process_spawn_t* process
) {
  return process != nullptr ? process->wait() : -1;
}

const bool sapi_process_spawn_write (
  sapi_process_spawn_t* process,
  const char* data,
  const size_t size
) {
  if (!process || process->closed) return false;
  process->write(data, size);
  return true;
}

const bool sapi_process_spawn_close_stdin (
  sapi_process_spawn_t* process
) {
  if (!process || process->closed) return false;
  process->close_stdin();
  return true;
}

const bool sapi_process_kill (
  sapi_process_spawn_t* process,
  const int code
) {
  if (!process || process->closed) return false;
  process->kill(code);
  return true;
}
