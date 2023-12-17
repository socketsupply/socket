#include "extension.hh"

static bool initialize (
  sapi_context_t* context,
  const void* data
) {
  initialize_libc_tests();
  initialize_sapi_tests();
  return true;
}

SOCKET_RUNTIME_REGISTER_EXTENSION(
  "wasm",
  initialize,
  NULL,
  "A native extension compiled to WASM",
  "0.0.1"
);
