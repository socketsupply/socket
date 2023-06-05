#include "extension.hh"

sapi_process_exec_t* sapi_process_exec (
  sapi_context_t* ctx,
  const char* command
) {
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
