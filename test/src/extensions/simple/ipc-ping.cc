#include <socket/extension.h>

void onping (
  sapi_context_t* context,
  sapi_ipc_message_t* message,
  const sapi_ipc_router_t* router
) {
  const char* pong = sapi_ipc_message_get_value(message);
  sapi_ipc_result_t* result = sapi_ipc_result_create(context, message);
  sapi_ipc_result_set_json_data(
    result,
    sapi_json_any(sapi_json_string_create(context, pong))
  );
  sapi_ipc_reply(result);
}

bool initialize (sapi_context_t* context, const void *data) {
  sapi_ipc_router_map(context, "simple.ping", onping, data);
  return true;
}

bool deinitialize (sapi_context_t* context, const void *data) {
  return true;
}

SOCKET_RUNTIME_REGISTER_EXTENSION(
  "simple-ipc-ping", // name
  initialize, // initializer
  deinitialize, // deinitializer
  "a simple IPC ping extension", // description
  "0.1.2" // version
);
