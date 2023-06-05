#ifndef SOCKET_RUNTIME_EXTENSION_H
#define SOCKET_RUNTIME_EXTENSION_H

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <uv.h>

#include "platform.h"

/**
 * Major version of the extension ABI.
 */
#define SOCKET_RUNTIME_EXTENSION_ABI_VERSION_MAJOR (unsigned int) 0

/**
 * Minor version of the extension ABI.
 */
#define SOCKET_RUNTIME_EXTENSION_ABI_VERSION_MINOR (unsigned int) 0

/**
 * Minor version of the extension ABI.
 */
#define SOCKET_RUNTIME_EXTENSION_ABI_VERSION_PATCH (unsigned int) 1

/**
 * The packed version of the extension ABI useful for semantic comparison purposes.
 */
#define SOCKET_RUNTIME_EXTENSION_ABI_VERSION ((int) (                          \
 SOCKET_RUNTIME_EXTENSION_ABI_VERSION_MAJOR << 16 |                            \
 SOCKET_RUNTIME_EXTENSION_ABI_VERSION_MINOR << 8  |                            \
 SOCKET_RUNTIME_EXTENSION_ABI_VERSION_PATCH << 0                               \
))

/**
 * Macro for registering an extension by name with a required initializer.
 * @param name          - *required* The name of the extension to register
 * @param initializer   - *required* A function that is called to initialize
 *                        the extension in the runtime
 * @param deinitializer - (optional) A function that is called to deinitialize
 *                        the extension in the runtime.
 * @param description   - (optional) A human readable description of the
 *                        registered extension
 * @param version       - (optional) A human readable version of the
 *                        registered extension
 * @see sapi_extension_registration_initializer
 */
#define SOCKET_RUNTIME_REGISTER_EXTENSION(_name, _initializer, ...)            \
  extern "C" {                                                                 \
    static sapi_extension_registration_t __sapi_extension__ = {                \
      SOCKET_RUNTIME_EXTENSION_ABI_VERSION,                                    \
      _name, _initializer, ##__VA_ARGS__                                       \
    };                                                                         \
                                                                               \
    const unsigned int __sapi_extension_abi () {                               \
      return __sapi_extension__.abi;                                           \
    }                                                                          \
                                                                               \
    const char* __sapi_extension_name () {                                     \
      return __sapi_extension__.name;                                          \
    }                                                                          \
                                                                               \
    const char* __sapi_extension_description () {                              \
      return __sapi_extension__.description;                                   \
    }                                                                          \
                                                                               \
    const char* __sapi_extension_version () {                                  \
      return __sapi_extension__.version;                                       \
    }                                                                          \
                                                                               \
    const sapi_extension_registration_t* __sapi_extension_init () {            \
      return &__sapi_extension__;                                              \
    }                                                                          \
  }

#if defined(__cplusplus)
extern "C" {
#endif

  /**
   * Context API
   * The _Context API_ is a general API for interacting with the
   * currently available to an extension. Most higher level functions will need
   * a context (`sapi_context_t`).
   */

  /**
   * An opaque pointer for an extension context. A context is provided to initializers,
   * deinitializers, and IPC route requests.
   */
  typedef struct sapi_context sapi_context_t;

  /**
   * A callback to be called at a later time given to the
   * `sapi_context_dispatch()` function.
   * @param context
   * @param data
   */
  typedef void (*sapi_context_dispatch_callback)(
    sapi_context_t* context,
    const void* data
  );

  /**
   * Creates a new context. The context is retained if a `parent` is not given
   * and therefor emust be disposed with `sapi_context_release()`.
   * @param parent - An optional parent to own the new context
   * @return The new context (`sapi_context_t*`)
   */
  sapi_context_t* sapi_context_create (sapi_context_t* parent);

  /**
   * Queues `callback` to be called sometime in the future for a `context`.
   * @param context  - An extension context
   * @param data     - User data to be given to `callback` when called
   * @param callback - The callback to dispatch
   */
  bool sapi_context_dispatch (
    sapi_context_t* context,
    const void* data,
    sapi_context_dispatch_callback callback
  );

  /**
   * Retain a context preventing any allocated memory from being deallocated
   * when the context is considered no longer valid. This function SHOULD NOT
   * be called from an extension initializer. The `sapi_context_release()`
   * function SHOULD be called at a later time to release any allocated memory
   * associated with the context.
   * @param context - An extension context
   */
  void sapi_context_retain (sapi_context_t* context);

  /**
   * Releases a context, freeing any allocated memory associated with the
   * context. This function SHOULD NOT be called from an extension initializer.
   * This function is typically called when a prior `sapi_context_retain()`
   * function was called for a given context.
   * @param context - An extension context
   */
  void sapi_context_release (sapi_context_t* context);

  /**
   * If a context is retained, then this function returns `true`, otherwise
   * `false` is returned.
   * @param context - An extension context
   * @return `true` if retained, otherwise `false`.
   */
  bool sapi_context_retained (const sapi_context_t* context);

  /**
   * Gets the libuv event loop associated with the context.
   * @param context - An extension context
   * @return The libuv event loop (`uv_loop_t`).
   */
  uv_loop_t* sapi_context_get_loop (
    const sapi_context_t* context
  );

  /**
   * Get the IPC bridge router for this context.
   * @param context - An extension context
   * @return A IPC bridge router (`sapi_ipc_router_t*`)
   */
  const struct sapi_ipc_router* sapi_context_get_router (
    const sapi_context_t* context
  );

  /**
   * Set a context error code.
   * @param context - An extension context
   * @param code    - The code of the error
   */
  void sapi_context_error_set_code (
    sapi_context_t* context,
    const int code
  );

  /**
   * Get a context error code.
   * @param context - An extension context
   * @return The code of the error
   */
  const int sapi_context_error_get_code (const sapi_context_t* context);

  /**
   * Set a context error name.
   * @param context - An extension context
   * @param name    - The name of the error
   */
  void sapi_context_error_set_name (
    sapi_context_t* context,
    const char* name
  );

  /**
   * Get a context error name.
   * @param context - An extension context
   * @return The name of the error
   */
  const char* sapi_context_error_get_name (const sapi_context_t* context);

  /**
   * Set a context error message.
   * @param context - An extension context
   * @param message - The message of the error
   */
  void sapi_context_error_set_message (
    sapi_context_t* context,
    const char* message
  );

  /**
   * Get a context error message.
   * @param context - An extension context
   * @return The message of the error
   */
  const char* sapi_context_error_get_message (const sapi_context_t* context);

  /**
   * Set a context error location.
   * @param context  - An extension context
   * @param location - The location of the error
   */
  void sapi_context_error_set_location (
    sapi_context_t* context,
    const char* location
  );

  /**
   * Get a context error location.
   * @param context - An extension context
   * @preturn The location of the error
   */
  const char* sapi_context_error_get_location (const sapi_context_t* context);


  /**
   * JavaScript API
   * The _JavaScript API_ is a general API for evaluating and interacting with the
   * available JavaScript context in a WebView associated with a context.
   */

  /**
   * @param context - An extension context
   * @param name    - The name of the script to evaluate
   * @param source  - The source of the script to evaluate
   */
  void sapi_context_evaluate_javascript (
    sapi_context_t* context,
    const char* name,
    const char* source
  );


  /**
   * JSON API
   * The _JSON API_ is a general purpose JSON builder.
   */

  /**
   * JSON type enumeration.
   */
  #define SAPI_JSON_TYPE_EMPTY -1
  #define SAPI_JSON_TYPE_ANY 0
  #define SAPI_JSON_TYPE_NULL 1
  #define SAPI_JSON_TYPE_OBJECT 2
  #define SAPI_JSON_TYPE_ARRAY 3
  #define SAPI_JSON_TYPE_BOOLEAN 4
  #define SAPI_JSON_TYPE_NUMBER 5
  #define SAPI_JSON_TYPE_STRING 6

  /**
   * `true` if `value` is an empty JSON type.
   * @param value - The value to test
   */
  #define sapi_json_value_is_empty(value)                                      \
    (SAPI_JSON_TYPE_EMPTY == sapi_json_typeof((sapi_json_any_t*) (value)))

  /**
   * `true` if `value` is a null JSON type.
   * @param value - The value to test
   */
  #define sapi_json_value_is_null(value)                                       \
    (SAPI_JSON_TYPE_NULL == sapi_json_typeof((sapi_json_any_t*) (value)))

  /**
   * `true` if `value` is an object JSON type.
   * @param value - The value to test
   */
  #define sapi_json_value_is_object(value)                                     \
    (SAPI_JSON_TYPE_OBJECT == sapi_json_typeof((sapi_json_any_t*) (value)))

  /**
   * `true` if `value` is an array JSON type.
   * @param value - The value to test
   */
  #define sapi_json_value_is_array(value)                                      \
    (SAPI_JSON_TYPE_ARRAY == sapi_json_typeof((sapi_json_any_t*) (value)))

  /**
   * `true` if `value` is a boolean JSON type.
   * @param value - The value to test
   */
  #define sapi_json_value_is_boolean(value)                                    \
    (SAPI_JSON_TYPE_BOOLEAN == sapi_json_typeof((sapi_json_any_t*) (value)))

  /**
   * `true` if `value` is a number JSON type.
   * @param value - The value to test
   */
  #define sapi_json_value_is_number(value)                                     \
    (SAPI_JSON_TYPE_NUMBER == sapi_json_typeof((sapi_json_any_t*) (value)))

  /**
   * `true` if `value` is a string JSON type.
   * @param value - The value to test
   */
  #define sapi_json_value_is_string(value)                                     \
    (SAPI_JSON_TYPE_STRING == sapi_json_typeof((sapi_json_any_t*) (value)))

  /**
   * An opaque JSON type that represents "any" JSON value
   */
  typedef struct sapi_json_any sapi_json_any_t;

  /**
   * An opaque JSON type that represents a `null` JSON value.
   */
  typedef struct sapi_json_null sapi_json_null_t;

  /**
   * An opaque JSON type that represents an "object" JSON value.
   */
  typedef struct sapi_json_object sapi_json_object_t;

  /**
   * An opaque JSON type that represents an "array" JSON value.
   */
  typedef struct sapi_json_array sapi_json_array_t;

  /**
   * An opaque JSON type that represents a `true` or `false` JSON value.
   */
  typedef struct sapi_json_boolean sapi_json_boolean_t;

  /**
   * An opaque JSON type that represents a "number" JSON value.
   */
  typedef struct sapi_json_number sapi_json_number_t;

  /**
   * An opaque JSON type that represents a "string" JSON value.
   */
  typedef struct sapi_json_string sapi_json_string_t;

  /**
   * A scalar type that represents the JSON type enumeration.
   */
  typedef int sapi_json_type_t;

  /**
   * Casts `value` to a `sapi_json_any_t*`.
   * @param value - The value to cast
   */
  #define sapi_json_any(value) ((sapi_json_any_t*)(value))

  /**
   * Casts `value` to a `sapi_json_any_t*`.
   * @param value - The value to cast
   */
  #define sapi_json_bool(value) ((sapi_json_any_t*)(value))

  /**
   * Computes the `sapi_json_type_t` enumeration value of a given `sapi_json_type_t`
   * type.
   */
  const sapi_json_type_t sapi_json_typeof (const sapi_json_any_t*);

  /**
   * Creates new JSON object.
   * @param context - A context associated with the extension
   * @return The JSON object
   */
  sapi_json_object_t* sapi_json_object_create (sapi_context_t* context);

  /**
   * Creates new JSON array.
   * @param context - A context associated with the extension
   * @return The JSON array
   */
  sapi_json_array_t* sapi_json_array_create (sapi_context_t* context);

  /**
   * Creates new JSON string.
   * @param context - A context associated with the extension
   * @param string  - The raw string
   * @return The JSON string
   */
  sapi_json_string_t* sapi_json_string_create (
    sapi_context_t* context,
    const char* string
  );

  /**
   * Creates new JSON boolean.
   * @param context - A context associated with the extension
   * @param boolean - The boolean
   * @return The JSON boolean
   */
  sapi_json_boolean_t* sapi_json_boolean_create (
    sapi_context_t* context,
    const bool boolean
  );

  /**
   * Creates new JSON number.
   * @param context - A context associated with the extension
   * @param number - The number
   * @return The JSON number
   */
  sapi_json_number_t* sapi_json_number_create (
    sapi_context_t* context,
    const double number
  );

  /**
   * Set JSON `value` for JSON `object` at `key`.
   * @param object - The object to set a value on
   * @param key    - The key of the value to set
   * @param value  - The JSON value to set
   */
  void sapi_json_object_set (
    sapi_json_object_t* object,
    const char* key,
    sapi_json_any_t* value
  );

  /**
   * Get JSON `value` for JSON `object` at `key`.
   * @param object - The object to set a value on
   * @param key    - The key of the value to set
   * @return The JSON value to set
   */
  sapi_json_any_t* sapi_json_object_get (
    const sapi_json_object_t* object,
    const char* key
  );

  /**
   * Set JSON `value` for JSON `array` at `index`.
   * @param array - The array to set a value on
   * @param index - The index of the value to set
   * @param value - The JSON value to set
   */
  void sapi_json_array_set (
    sapi_json_array_t* array,
    const unsigned int index,
    sapi_json_any_t* value
  );

  /**
   * Get JSON `value` for JSON `array` at `index`.
   * @param array - The array to set a value on
   * @param index - The index of the value to set
   * @return The JSON value to set
   */
  sapi_json_any_t* sapi_json_array_get (
    const sapi_json_array_t* array,
    const unsigned int index
  );

  /**
   * Push a JSON `value` to the end of a JSON `array`.
   * @param array - The array to set a value on
   * @param value - The JSON value to set
   */
  void sapi_json_array_push (
    sapi_json_array_t* json,
    sapi_json_any_t* any
  );

  /**
   * Pop and return the last JSON `value` in a JSON `array`
   * @param array - The array to set a value on
   * @return The popped JSON value.
   */
  sapi_json_any_t* sapi_json_array_pop (
    sapi_json_array_t* json
  );

  /**
   * Convert JSON `value` to a string.
   * @param value - The JSON value to convert to a string
   * @return The JSON value as a string
   */
  const char * sapi_json_stringify (const sapi_json_any_t*);


  /**
   * Miscellaneous API
   */

  /**
   * Called when initializing an extension.
   * @param context - An extension context
   * @param data    - Optional user data
   * @return `true` if initialization was successful, otherwise `false`
   */
  typedef bool (*sapi_extension_registration_initializer_t)(
    sapi_context_t* context,
    const void* data
  );

  /**
   * Called when deinitializing an extension.
   * @param context - An extension context
   * @param data    - Optional user data
   * @return `true` if deinitialization was successful, otherwise `false`
   */
  typedef bool (*sapi_extension_registration_deinitializer_t)(
    sapi_context_t* context,
    const void* data
  );

  /**
   * A container for an extension registration.
   */
  typedef struct sapi_extension_registration sapi_extension_registration_t;
  struct sapi_extension_registration {
    unsigned long abi;
    // required
    const char* name;
    const sapi_extension_registration_initializer_t initializer;

    // optional
    const sapi_extension_registration_deinitializer_t deinitializer;
    const char* description;
    const char* version;

    // reserved for future ABI changes
    char __reserved[1024];
  };

  /**
   * Register a new extension. There is typically no need to call this directly.
   * @param registration - The registration info for an extension
   * @return `true` if successful, otherwise `false`
   */
  bool sapi_extension_register (
    const sapi_extension_registration_t* registration
  );

  /**
   * Print `message` to the stdout log.
   * @param context - An extension context
   * @param message - A message to write to stdout
   */
  void sapi_log (const sapi_context_t* ctx, const char *message);

  /**
   * Print `message` to the stderr log.
   * @param context - An extension context
   * @param message - A message to write to stderr
   */
  void sapi_debug (const sapi_context_t* ctx, const char *message);

  /**
   * A simple printf macro for extension implementers with a total max output
   * buffer size of `BUFSIZ` (@see `stdio.h`, `string.h`)
   * @param format - Format string for formatted output
   * @param ...    - `format` argument values
   */
  #define sapi_printf(ctx, format, ...) ({                                     \
    char buffer[BUFSIZ] = {0};                                                 \
    auto size = snprintf(nullptr, 0, format, ##__VA_ARGS__) + 1;               \
    snprintf(buffer, size, format, ##__VA_ARGS__);                             \
    sapi_log(ctx, buffer);                                                     \
  })

  /**
   * Computes a random `uint64_t` value.
   * @return The random `uint64_t` value
   */
  uint64_t sapi_rand64 ();


  /**
   * IPC API
   * The _IPC API_ provides an interface for _interprocess communication_
   * between the native runtime extension and the WebView. The API allows for
   * creating and responding to IPC router routes.
   */

  /**
   * An opaque pointer for an IPC message. Use `sapi_ipc_message_*` functions
   * for interacting with this type.
   */
  typedef struct sapi_ipc_message sapi_ipc_message_t;

  /**
   * An opaque pointer for an IPC bridge.
   */
  typedef struct sapi_ipc_bridge sapi_ipc_bridge_t;

  /**
   * An opaque pointer for an IPC result. Use `sapi_ipc_result_*` functions
   * for interacting with this type.
   */
  typedef struct sapi_ipc_result sapi_ipc_result_t;

  /**
   * An opaque pointer for an IPC router. Use `sapi_ipc_router_*` functions
   * for interacting with this type.
   */
  typedef struct sapi_ipc_router sapi_ipc_router_t;

  /**
   * A callback called when an IPC route receives a request.
   * @param context - The extension context for this request
   * @param message - The IPC message for this request
   * @param router  - The IPC router associated with the extension context
   */
  typedef void (*sapi_ipc_router_message_callback_t)(
    sapi_context_t* context,
    sapi_ipc_message_t* message,
    const sapi_ipc_router_t* router
  );

  /**
   * A callback for `sapi_ipc_invoke()` called when an IPC has been invoked.
   * @param result - The result of the invocation
   * @param router - The IPC router associated with the extension context
   */
  typedef void (*sapi_ipc_router_result_callback_t)(
    const sapi_ipc_result_t* result,
    const sapi_ipc_router_t* router
  );

  /**
   * Get the window index the IPC message is associated with.
   * @param message The IPC message
   * @return The window index the IPC message was initiated from or intended for
   */
  int sapi_ipc_message_get_index (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the value associated with the IPC message.
   * @param message The IPC message
   * @return The IPC message value (possibly `NULL`)
   */
  const char* sapi_ipc_message_get_value (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the IPC message name.
   * @param message The IPC message
   * @return The IPC message name (possibly `NULL`)
   */
  const char* sapi_ipc_message_get_name (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the sequence of the IPC message. If
   * @param message The IPC message
   * @return The IPC message sequence value (possibly `NULL`, "", "-1", or "...")
   */
  const char* sapi_ipc_message_get_seq (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the URI of the IPC message.
   * @param message The IPC message
   * @return The IPC message URI value (possibly `NULL`)
   */
  const char* sapi_ipc_message_get_uri (
    const sapi_ipc_message_t* message
  );

  /**
   * Get an arbitrary IPC message parameter value.
   * @param message The IPC message
   * @return The IPC message parameter value (possibly `NULL`)
   */
  const char* sapi_ipc_message_get (
    const sapi_ipc_message_t* message,
    const char* key
  );

  /**
   * Create a new IPC result from a given `context` and IPC `message`.
   * @param context - An extension context for a IPC request
   * @param message - The IPC message for this request
   * @return The new IPC result
   */
  sapi_ipc_result_t* sapi_ipc_result_create (
    sapi_context_t* context,
    sapi_ipc_message_t *message
  );

  /**
   * Set IPC result sequence value.
   * @param result - An IPC request result
   * @param seq    - An IPC sequence value
   */
  void sapi_ipc_result_set_seq (
    sapi_ipc_result_t* result,
    const char* seq
  );

  /**
   * Get IPC result sequence value.
   * @param result - An IPC request result
   * @return The result sequence value (possibly `NULL`, "", "-1", or "...")
   */
  const char* sapi_ipc_result_get_seq (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result message
   * @param result  - An IPC request result
   * @param message - The IPC message for this result
   */
  void sapi_ipc_result_set_message (
    sapi_ipc_result_t* result,
    sapi_ipc_message_t* message
  );

  /**
   * Get the ICP message for this IPC result.
   * @param result - An IPC request result
   * @return the IPC message
   */
  const sapi_ipc_message_t* sapi_ipc_result_get_message (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result JSON value.
   * @param result - An IPC request result
   * @param json   - The result JSON
   */
  void sapi_ipc_result_set_json (
    sapi_ipc_result_t* result,
    const sapi_json_any_t* json
  );

  /**
   * Get the IPC result JSON
   * @param result - An IPC request result
   * @return The IPC result JSON
   */
  const sapi_json_any_t* sapi_ipc_result_get_json (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result JSON value "data" field.
   * @param result - An IPC request result
   * @param json   - The result JSON "data" field value
   */
  void sapi_ipc_result_set_json_data (
    sapi_ipc_result_t* result,
    const sapi_json_any_t* json
  );

  /**
   * Get the result JSON value "data" field value.
   * @param result - An IPC request result
   * @return The result JSON "data" field value
   */
  const sapi_json_any_t* sapi_ipc_result_get_json_data (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result JSON value "err"` field.
   * @param result - An IPC request result
   * @param json   - The result JSON "err" field value
   */
  void sapi_ipc_result_set_json_error (
    sapi_ipc_result_t* result,
    const sapi_json_any_t* json
  );

  /**
   * Get the result JSON value "err" field value.
   * @param result - An IPC request result
   * @return The result JSON "err" field value
   */
  const sapi_json_any_t* sapi_ipc_result_get_json_error (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result bytes.
   * @param result - An IPC request result
   * @param size   - The size of the bytes
   * @param bytes  - The bytes
   */
  void sapi_ipc_result_set_bytes (
    sapi_ipc_result_t* result,
    const unsigned int size,
    const unsigned char* bytes
  );

  /**
   * Get the IPC result bytes.
   * @param result - An IPC request result
   * @return THe IPC result bytes
   */
  const unsigned char* sapi_ipc_result_get_bytes (
    const sapi_ipc_result_t* result
  );

  /**
   * Get the result bytes size.
   * @param result - An IPC request result
   * @return The size of the IPC result bytes
   */
  const unsigned int sapi_ipc_result_get_bytes_size (
    const sapi_ipc_result_t* result
  );

  /**
   * Set an IPC result response header.
   * @param result - An IPC request result
   * @param name   - The name of the header
   * @param value  - The value of the header
   */
  void sapi_ipc_result_set_header (
    sapi_ipc_result_t* result,
    const char* name,
    const char* value
  );

  /**
   * Get an IPC result response header.
   * @param result - An IPC request result
   */
  const char* sapi_ipc_result_get_header (
    const sapi_ipc_result_t* result,
    const char* name
  );

  /**
   * Creates a new IPC result from `context`, `message` and `json`
   * @param context - An extension context
   * @param message - The IPC message for this request
   * @param json    - The result JSON
   * @return The new IPC result
   */
  sapi_ipc_result_t* sapi_ipc_result_from_json (
    sapi_context_t* context,
    sapi_ipc_message_t* message,
    const sapi_json_any_t* json
  );

  /**
   * Creates a "reply" for an IPC route request.
   * @param result - An IPC request result
   * @return `true` if successful, otherwise `false`
   */
  bool sapi_ipc_reply (const sapi_ipc_result_t* result);

  /**
   * Send JSON to the bridge to propagate to the WebView.
   * @param context - An extension context
   * @param seq     - An IPC sequence value
   * @param json    - The result JSON
   * @return `true` if successful, otherwise `false`
   */
  bool sapi_ipc_send_json (
    sapi_context_t* context,
    const char* seq,
    sapi_json_any_t* json
  );

  /**
   * Send bytes to the bridge to propagate to the WebView.
   * @param context - An extension context
   * @param seq     - An IPC sequence value
   * @param size    - The size of the bytes
   * @param bytes   - The bytes
   * @return `true` if successful, otherwise `false`
   */
  bool sapi_ipc_send_bytes (
    sapi_context_t* context,
    const char* seq,
    const unsigned int size,
    const unsigned char* bytes
  );

  /**
   * Map a named route to a callback with optional use data for a given
   * extension context. Routes must "reply" with a result to respond to an
   * incoming request.
   * @param context  - An extension context
   * @param route    - The route name to map
   * @param callback - The callback called when an IPC route receives a request
   * @param data     - User data propagated to `callback`
   */
  void sapi_ipc_router_map (
    sapi_context_t* context,
    const char* route,
    sapi_ipc_router_message_callback_t callback,
    const void* data
  );

  /**
   * Unmap a named route for a given extension context.
   * incoming request.
   * @param context  - An extension context
   * @param route    - The route name to map
   */
  void sapi_ipc_router_unmap (
    sapi_context_t* context,
    const char* route
  );

  /**
   * Bind a callback to a named route. Callbacks SHOULD NOT respond to these
   * requests as they are ignored. Use `"*"` to listen to all routes. This
   * function returns a "token" that can be used to "unlisten" in the future.
   * @param context  - An extension context
   * @param route    - The route name to map
   * @param callback - The callback called when an IPC route receives a request
   * @param data     - User data propagated to `callback`
   * @return A token to be used with `sapi_ipc_router_unlisten()`
   */
  uint64_t sapi_ipc_router_listen (
    sapi_context_t* context,
    const char* route,
    sapi_ipc_router_message_callback_t callback,
    const void* data
  );

  /**
   * Unbinds a callback to a named route. Callbacks SHOULD NOT respond to these
   * requests as they are ignored. Use `"*"` to listen to all routes. This
   * function returns a "token" that can be used to "unlisten" in the future.
   * @param context - An extension context
   * @param route   - The route name to map
   * @param token   - A token from `sapi_ipc_router_listen()`
   * @return `true` if retained, otherwise `false`.
   */
  bool sapi_ipc_router_unlisten (
    sapi_context_t* context,
    const char* route,
    uint64_t token
  );


  /**
   * Process API
   * The _Process API_ provides a simple set of functions for executing child
   * processes and retrieving their output.
   */

  /**
   * An opaque pointer for an executed child process.
   */
  typedef struct sapi_process_exec sapi_process_exec_t;

  /**
   * Execute `command` as a child process. stdout and stderr are both captured
   * as interleaved output.
   * @param context - An extension context
   * @param command - A system shell command to execute
   * @return The process execution result with output.
   */
  sapi_process_exec_t* sapi_process_exec (sapi_context_t*, const char* command);

  /**
   * Frees up a process execution result and any allocated resources.
   * @param process - The process execution result
   * @return The exit code of the process
   */
  int sapi_process_exec_finish (sapi_process_exec_t* process);

  /**
   * Get the exit code of an execution result.
   * @param process - The process execution result
   * @return The exit code of the process
   */
  const int sapi_process_exec_get_exit_code (
    const sapi_process_exec_t* process
  );

  /**
   * Get the output of an execution result.
   * @param process - The process execution result
   * @return The output of the process
   */
  const char* sapi_process_exec_get_output (const sapi_process_exec_t* process);


  /**
   * Config API
   * The _Config API_ provides an interface for getting and setting
   * extension configuration for a given context.
   */

  /**
   * Get a configuration value for an extension
   * @param context - An extension context
   * @param key     - A configuration key path
   * @return The configuration value
   */
  const char * sapi_context_config_get (
    const sapi_context_t* context,
    const char* key
  );

  /**
   * Set a configuration value for an extension
   * @param context - An extension context
   * @param key     - A configuration key path
   * @param value   - The configuration value
   */
  void sapi_context_config_set (
    sapi_context_t* context,
    const char* key,
    const char* value
  );


  /**
   * Extension API
   * The _Extension API_ provides an interface for loading other extensions.
   */

  /**
   * Load an extension by `name` in `context` with optional user `data`.
   * @param context - An extension context
   * @param name    - The name of the extension
   * @param data    - Optional user data to pass to the initializer
   * @return `true` if the extension was loaded, otherwise `false`
   */
  bool sapi_extension_load (
    sapi_context_t* context,
    const char *name,
    const void* data
  );

  /**
   * Unload an extension by `name` in `context`
   * @param context - An extension context
   * @param name    - The name of the extension
   * @return `true` if the extension was loaded, otherwise `false`
   */
  bool sapi_extension_unload (sapi_context_t* context, const char *name);

  /**
   * Get an extensions registration info
   * @param context - An extension context
   * @param name    - The name of the extension
   * @return The extension registration info
   */
  const sapi_extension_registration_t* sapi_extension_get (
    const sapi_context_t* context,
    const char *name
  );

#if defined(__cplusplus)
};
#endif

#endif
