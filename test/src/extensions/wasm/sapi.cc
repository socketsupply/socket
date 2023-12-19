#include "extension.hh"
#include <string.h>

static void onhello (
  sapi_context_t* context,
  sapi_ipc_message_t* message,
  const sapi_ipc_router_t* router
) {
  auto result = sapi_ipc_result_create(context, message);
  auto object = sapi_json_object_create(context);
  auto string = sapi_json_string_create(context, "world");
  sapi_json_object_set(object, "hello", string);
  sapi_ipc_result_set_json_data(result, sapi_json_any(object));
  sapi_ipc_result_set_header(result, "header-name", "header value");
  sapi_ipc_reply(result);
}

void initialize_sapi_tests (sapi_context_t* context) {
  auto object = sapi_json_object_create(context);
  auto string = sapi_json_string_create(context, "world");

  test(context != NULL);
  test(object != NULL);
  test(string != NULL);

  sapi_json_object_set(object, "hello", string);

  test(strcmp("{\"hello\":\"world\"}", sapi_json_stringify(object)) == 0);
  test(sapi_json_typeof(sapi_json_any(object)) == SAPI_JSON_TYPE_OBJECT);
  test(sapi_json_typeof(sapi_json_any(string)) == SAPI_JSON_TYPE_STRING);

  sapi_context_config_set(context, "foo", "bar");
  test(strcmp("bar", sapi_context_config_get(context, "foo")) == 0);

  sapi_ipc_router_map(context, "wasm.hello", onhello, NULL);
}
