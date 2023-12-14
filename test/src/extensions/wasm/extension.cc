#include <socket/extension.h>

static bool initialize (
  sapi_context_t* context,
  const void* data
) {
  sapi_log(context, "from wasm");
  sapi_javascript_evaluate(context, "foo", "console.log('hello world!')");
  sapi_log(context, sapi_env_get(context, "HOME"));
  sapi_context_dispatch(context, NULL, [](sapi_context_t* context, const void* data) {
    sapi_log(context, "dispatched callback");
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
