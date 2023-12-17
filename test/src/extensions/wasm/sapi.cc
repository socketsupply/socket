#include "extension.hh"
#include <string.h>

void initialize_sapi_tests (sapi_context_t* context) {
  test(context != NULL);
  auto object = sapi_json_object_create(context);
  test(object != NULL);
  auto string = sapi_json_string_create(context, "world");
  test(string != NULL);
  sapi_json_object_set(object, "hello", string);
  test(strcmp("{\"hello\":\"world\"}", sapi_json_stringify(object)) == 0);
  test(sapi_json_typeof(sapi_json_any(object)) == SAPI_JSON_TYPE_OBJECT);
  test(sapi_json_typeof(sapi_json_any(string)) == SAPI_JSON_TYPE_STRING);
  sapi_context_config_set(context, "foo", "bar");
  test(strcmp("bar", sapi_context_config_get(context, "foo")) == 0);
}
