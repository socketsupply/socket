#include "extension.hh"

static bool initialize (
  sapi_context_t* context,
  const void* data
) {
  atexit([]() {
    ok("atexit callback called");
  });
  initialize_libc_tests();
  initialize_sapi_tests();
  sapi_context_dispatch(context, data, [](sapi_context_t* context, const void* data) {
    exit(0);
  });
  return true;
}

SOCKET_RUNTIME_REGISTER_EXTENSION(
  "wasm",
  initialize,
  NULL,
  "A native extension compiled to WASM",
  "0.0.1"
);
