#ifndef SOCKET_RUNTIME_EXTENSION_H
#define SOCKET_RUNTIME_EXTENSION_H

#if defined(SOCKET_RUNTIME_EXTENSION_WASM)
#include "webassembly.h"
#else
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <uv.h>
#include "platform.h"
#endif

#if defined(__cplusplus)
#define SOCKET_RUNTIME_EXTENSION_EXTERN_BEGIN extern "C" {
#define SOCKET_RUNTIME_EXTENSION_EXTERN_END };
#else
#define SOCKET_RUNTIME_EXTENSION_EXTERN_BEGIN
#define SOCKET_RUNTIME_EXTENSION_EXTERN_END
#endif

#if defined(SOCKET_RUNTIME_EXTENSION_WASM)
# define SOCKET_RUNTIME_EXTENSION_EXPORT extern
#elif SOCKET_RUNTIME_PLATFORM_WINDOWS
# define SOCKET_RUNTIME_EXTENSION_EXPORT __declspec(dllexport)
#elif SOCKET_RUNTIME_PLATFORM_LINUX
# define SOCKET_RUNTIME_EXTENSION_EXPORT __attribute__((visibility("default")))
#elif SOCKET_RUNTIME_PLATFORM_MACOS || SOCKET_RUNTIME_PLATFORM_IOS
# define SOCKET_RUNTIME_EXTENSION_EXPORT __attribute__((visibility("default")))
#else
# define SOCKET_RUNTIME_EXTENSION_EXPORT
#endif

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
 * The packed version of the extension ABI useful for semantic
 * comparison purposes.
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
  SOCKET_RUNTIME_EXTENSION_EXTERN_BEGIN                                        \
    static sapi_extension_registration_t __sapi_extension__ = {                \
      SOCKET_RUNTIME_EXTENSION_ABI_VERSION,                                    \
      _name, _initializer, ##__VA_ARGS__                                       \
    };                                                                         \
                                                                               \
    SOCKET_RUNTIME_EXTENSION_EXPORT                                            \
    unsigned long __sapi_extension_abi () {                                    \
      return __sapi_extension__.abi;                                           \
    }                                                                          \
                                                                               \
    SOCKET_RUNTIME_EXTENSION_EXPORT                                            \
    const char* __sapi_extension_name () {                                     \
      return __sapi_extension__.name;                                          \
    }                                                                          \
                                                                               \
    SOCKET_RUNTIME_EXTENSION_EXPORT                                            \
    const char* __sapi_extension_description () {                              \
      return __sapi_extension__.description;                                   \
    }                                                                          \
                                                                               \
    SOCKET_RUNTIME_EXTENSION_EXPORT                                            \
    const char* __sapi_extension_version () {                                  \
      return __sapi_extension__.version;                                       \
    }                                                                          \
                                                                               \
    SOCKET_RUNTIME_EXTENSION_EXPORT                                            \
    const sapi_extension_registration_initializer_t                            \
    __sapi_extension_initializer () {                                          \
      return __sapi_extension__.initializer;                                   \
    }                                                                          \
                                                                               \
    SOCKET_RUNTIME_EXTENSION_EXPORT                                            \
    const sapi_extension_registration_deinitializer_t                          \
    __sapi_extension_deinitializer () {                                        \
      return __sapi_extension__.deinitializer;                                 \
    }                                                                          \
                                                                               \
    SOCKET_RUNTIME_EXTENSION_EXPORT                                            \
    const sapi_extension_registration_t* __sapi_extension_init () {            \
      return &__sapi_extension__;                                              \
    }                                                                          \
  SOCKET_RUNTIME_EXTENSION_EXTERN_END                                          \

SOCKET_RUNTIME_EXTENSION_EXTERN_BEGIN
  typedef struct sapi_extension_registration sapi_extension_registration_t;

  /**
   * Internal APIs
   */

  /**
   * Internal ABI version getter for a registered extension.
   * @ignore
   * @private
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  unsigned long __sapi_extension_abi ();

  /**
   * Internal name getter for a registered extension.
   * @ignore
   * @private
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* __sapi_extension_name ();

  /**
   * Internal description getter for a registered extension.
   * @ignore
   * @private
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* __sapi_extension_description ();

  /**
   * Internal version getter for a registered extension.
   * @ignore
   * @private
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* __sapi_extension_version ();

  /**
   * Internal initializer for a registered extension.
   * @ignore
   * @private
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_extension_registration_t* __sapi_extension_init ();

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
   * @param parent   - An optional parent to own the new context
   * @param retained - `true` if the returned context should be retained (owned) by the caller
   * @return The new context (`sapi_context_t*`)
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_context_t* sapi_context_create (
    sapi_context_t* parent,
    bool retained
  );

  /**
   * Set user data on a context.
   * @param context - An extension context
   * @param data    - User data
   * @return `true` if the user data was set, `false` otherwise
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_context_set_data (
    sapi_context_t* context,
    const void* data
  );

  /**
   * Queues `callback` to be called sometime in the future for a `context`.
   * @param context  - An extension context
   * @param data     - User data to be given to `callback` when called
   * @param callback - The callback to dispatch
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_context_retain (sapi_context_t* context);

  /**
   * Releases a context, freeing any allocated memory associated with the
   * context. This function SHOULD NOT be called from an extension initializer.
   * This function is typically called when a prior `sapi_context_retain()`
   * function was called for a given context.
   * @param context - An extension context
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_context_release (sapi_context_t* context);

  /**
   * If a context is retained, then this function returns `true`, otherwise
   * `false` is returned.
   * @param context - An extension context
   * @return `true` if retained, otherwise `false`.
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_context_retained (const sapi_context_t* context);

  /**
   * Gets the libuv event loop associated with the context.
   * @param context - An extension context
   * @return The libuv event loop (`uv_loop_t`).
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  uv_loop_t* sapi_context_get_loop (
    const sapi_context_t* context
  );

  /**
   * Get the IPC bridge router for this context.
   * @param context - An extension context
   * @return A IPC bridge router (`sapi_ipc_router_t*`)
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const struct sapi_ipc_router* sapi_context_get_router (
    const sapi_context_t* context
  );

  /**
   * Get user data from context.
   * @param context - An extension context
   * @return An opaque pointer to user data
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const void * sapi_context_get_data (
    const sapi_context_t* context
  );

  /**
   * Get parent context for this context
   * @param context - An extension context
   * @return A pointer to the parent context. This value may be `NULL`.
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_context_t* sapi_context_get_parent (
    const sapi_context_t* context
  );

  /**
   * Set a context error code.
   * @param context - An extension context
   * @param code    - The code of the error
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_context_error_set_code (
    sapi_context_t* context,
    int code
  );

  /**
   * Get a context error code.
   * @param context - An extension context
   * @return The code of the error
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  int sapi_context_error_get_code (const sapi_context_t* context);

  /**
   * Set a context error name.
   * @param context - An extension context
   * @param name    - The name of the error
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_context_error_set_name (
    sapi_context_t* context,
    const char* name
  );

  /**
   * Get a context error name.
   * @param context - An extension context
   * @return The name of the error
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_context_error_get_name (const sapi_context_t* context);

  /**
   * Set a context error message.
   * @param context - An extension context
   * @param message - The message of the error
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_context_error_set_message (
    sapi_context_t* context,
    const char* message
  );

  /**
   * Get a context error message.
   * @param context - An extension context
   * @return The message of the error
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_context_error_get_message (const sapi_context_t* context);

  /**
   * Set a context error location.
   * @param context  - An extension context
   * @param location - The location of the error
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_context_error_set_location (
    sapi_context_t* context,
    const char* location
  );

  /**
   * Get a context error location.
   * @param context - An extension context
   * @return The location of the error
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_context_error_get_location (const sapi_context_t* context);

  /**
   * @param context - An extension context
   * @return Allocated memmory for the context
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void* sapi_context_alloc (sapi_context_t* context, unsigned int size);


  /**
   * JavaScript API
   * The _JavaScript API_ is a general API for evaluating and interacting with the
   * available JavaScript context in a WebView associated with a context.
   */

  /**
   * Evaluate named JavaScript in context.
   * @param context - An extension context
   * @param name    - The name of the script to evaluate
   * @param source  - The source of the script to evaluate
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_javascript_evaluate (
    sapi_context_t* context,
    const char* name,
    const char* source
  );

  /**
   * Get an environment variable
   * @param context - An extension context
   * @param name    - The name of the environment variable
   * @return The value of the environment which can be `NULL`
   */
  const char* sapi_env_get (
    sapi_context_t* context,
    const char* name
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
  #define SAPI_JSON_TYPE_RAW 7

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
   * Set JSON `value` for JSON `object` at `key`. Generally, an alias to the
   * `sapi_json_object_set_value` function.
   * @param object - The object to set a value on
   * @param key    - The key of the value to set
   * @param value  - The JSON value to set
   */
  #define sapi_json_object_set(object, key, value)                             \
    sapi_json_object_set_value(                                                \
      (sapi_json_object_t*) (object),                                          \
      (const char*)(key),                                                      \
      (sapi_json_any((value)))                                                 \
    )

  /**
   * Set JSON `value` for JSON `array` at `index`. Generally, an alias to the
   * `sapi_json_array_set` function.
   * @param array - The array to set a value on
   * @param index - The index of the value to set
   * @param value - The JSON value to set
   */
  #define sapi_json_array_set(array, index, value)                             \
    sapi_json_array_set_value(                                                 \
      (sapi_json_array_t*) (array),                                            \
      (unsigned int) (index),                                                  \
      (sapi_json_any((value)))                                                 \
    )

  /**
   * Push a JSON `value` to the end of a JSON `array`. Generally, an alias to
   * the `sapi_json_array_push_value` function.
   * @param array - The array to set a value on
   * @param value - The JSON value to set
   */
  #define sapi_json_array_push(array, value)                                   \
    sapi_json_array_push_value (                                               \
      (sapi_json_array_t*) (array),                                            \
      sapi_json_any((value))                                                   \
    )

  /**
   * Convert JSON `value` to a string. Generally, an alias to the
   * `sapi_json_string_create` function.
   * @param value - The JSON value to convert to a string
   * @return The JSON value as a string
   */
  #define sapi_json_stringify(value)                                           \
    sapi_json_stringify_value(sapi_json_any(value))

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
   * An opaque JSON type that represents to a "raw" JSON value.
   */
  typedef struct sapi_json_raw sapi_json_raw_t;

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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_json_type_t sapi_json_typeof (const sapi_json_any_t*);

  /**
   * Creates new JSON object.
   * @param context - A context associated with the extension
   * @return The JSON object
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_json_object_t* sapi_json_object_create (sapi_context_t* context);

  /**
   * Creates new JSON array.
   * @param context - A context associated with the extension
   * @return The JSON array
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_json_array_t* sapi_json_array_create (sapi_context_t* context);

  /**
   * Creates new JSON string.
   * @param context - A context associated with the extension
   * @param string  - The raw string
   * @return The JSON string
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_json_boolean_t* sapi_json_boolean_create (
    sapi_context_t* context,
    bool boolean
  );

  /**
   * Creates new JSON number.
   * @param context - A context associated with the extension
   * @param number - The number
   * @return The JSON number
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_json_number_t* sapi_json_number_create (
    sapi_context_t* context,
    const double number
  );

  /**
   * Creates raw JSON from a source string. The source string is NOT parsed.
   * @param context - A context associated with the extension
   * @param source  - The raw JSON source string
   * @return The JSON created from raw source.
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_json_any_t* sapi_json_raw_from (
    sapi_context_t* context,
    const char* source
  );

  /**
   * Set JSON `value` for JSON `object` at `key`.
   * @param object - The object to set a value on
   * @param key    - The key of the value to set
   * @param value  - The JSON value to set
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_json_object_set_value (
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_json_array_set_value (
    sapi_json_array_t* array,
    unsigned int index,
    sapi_json_any_t* value
  );

  /**
   * Get JSON `value` for JSON `array` at `index`.
   * @param array - The array to set a value on
   * @param index - The index of the value to set
   * @return The JSON value to set
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_json_any_t* sapi_json_array_get (
    const sapi_json_array_t* array,
    unsigned int index
  );

  /**
   * Push a JSON `value` to the end of a JSON `array`.
   * @param array - The array to set a value on
   * @param value - The JSON value to set
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_json_array_push_value (
    sapi_json_array_t* array,
    sapi_json_any_t* value
  );

  /**
   * Pop and return the last JSON `value` in a JSON `array`
   * @param array - The array to set a value on
   * @return The popped JSON value.
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_json_any_t* sapi_json_array_pop (
    sapi_json_array_t* json
  );

  /**
   * Convert JSON `value` to a string.
   * @param value - The JSON value to convert to a string
   * @return The JSON value as a string
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char * sapi_json_stringify_value (const sapi_json_any_t*);


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
  struct sapi_extension_registration {
    unsigned long abi:32;
    // required
    const char* name;
    const sapi_extension_registration_initializer_t initializer;

    // optional
    const sapi_extension_registration_deinitializer_t deinitializer;
    const char* description;
    const char* version;

    // reserved for future ABI changes
    char __reserved__[1024];
  };

  /**
   * Register a new extension. There is typically no need to call this directly.
   * @param registration - The registration info for an extension
   * @return `true` if successful, otherwise `false`
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_extension_register (
    const sapi_extension_registration_t* registration
  );

  /**
   * Predicate to determine if a policy is allowed in an extension context.
   * Multiple policy names can be given as a list seperated by `,`.
   * ie: "ipc,context,process"
   * @param context - An extension context
   * @param allowed - An extension context policy name
   * @return `true` if `allowed` is allowed in `context`.
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_extension_is_allowed (sapi_context_t* context, const char *allowed);

  /**
   * Print `message` to the stdout log.
   * @param context - An extension context
   * @param message - A message to write to stdout
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_log (const sapi_context_t* ctx, const char *message);

  /**
   * Print `message` to the stderr log.
   * @param context - An extension context
   * @param message - A message to write to stderr
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_debug (const sapi_context_t* ctx, const char *message);

  /**
   * A simple printf macro for extension implementers with a total max output
   * buffer size of `BUFSIZ` (@see `stdio.h`, `string.h`)
   * @param format - Format string for formatted output
   * @param ...    - `format` argument values
   */
  #define sapi_printf(ctx, format, ...) ({                                     \
    char _buffer[BUFSIZ] = {0};                                                \
    snprintf(_buffer, BUFSIZ, format, ##__VA_ARGS__);                          \
    sapi_log(ctx, _buffer);                                                    \
  })

  /**
   * Computes a random `uint64_t` value.
   * @return The random `uint64_t` value
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  int sapi_ipc_message_get_index (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the value associated with the IPC message.
   * @param message The IPC message
   * @return The IPC message value (possibly `NULL`)
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_ipc_message_get_value (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the buffer bytes associated with the IPC message.
   * @param message The IPC message
   * @return The IPC message va (possibly `NULL`)
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const unsigned char* sapi_ipc_message_get_bytes (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the buffer bytes size associated with the IPC message.
   * @param message The IPC message
   * @return The IPC message va (possibly `NULL`)
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  unsigned int sapi_ipc_message_get_bytes_size (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the IPC message name.
   * @param message The IPC message
   * @return The IPC message name (possibly `NULL`)
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_ipc_message_get_name (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the sequence of the IPC message. If
   * @param message The IPC message
   * @return The IPC message sequence value (possibly `NULL`, "", "-1", or "...")
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_ipc_message_get_seq (
    const sapi_ipc_message_t* message
  );

  /**
   * Get the URI of the IPC message.
   * @param message The IPC message
   * @return The IPC message URI value (possibly `NULL`)
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_ipc_message_get_uri (
    const sapi_ipc_message_t* message
  );

  /**
   * Get an arbitrary IPC message parameter value.
   * @param message The IPC message
   * @return The IPC message parameter value (possibly `NULL`)
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_ipc_result_t* sapi_ipc_result_create (
    sapi_context_t* context,
    sapi_ipc_message_t *message
  );

  /**
   * Set IPC result sequence value.
   * @param result - An IPC request result
   * @param seq    - An IPC sequence value
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_ipc_result_set_seq (
    sapi_ipc_result_t* result,
    const char* seq
  );

  /**
   * Get IPC result sequence value.
   * @param result - An IPC request result
   * @return The result sequence value (possibly `NULL`, "", "-1", or "...")
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_ipc_result_get_seq (
    const sapi_ipc_result_t* result
  );

  /**
   * Get context from IPC result.
   * @param result - An IPC request result
   * @return An extension context
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_context_t* sapi_ipc_result_get_context (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result message
   * @param result  - An IPC request result
   * @param message - The IPC message for this result
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_ipc_result_set_message (
    sapi_ipc_result_t* result,
    sapi_ipc_message_t* message
  );

  /**
   * Get the ICP message for this IPC result.
   * @param result - An IPC request result
   * @return the IPC message
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_ipc_message_t* sapi_ipc_result_get_message (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result JSON value.
   * @param result - An IPC request result
   * @param json   - The result JSON
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_ipc_result_set_json (
    sapi_ipc_result_t* result,
    const sapi_json_any_t* json
  );

  /**
   * Get the IPC result JSON
   * @param result - An IPC request result
   * @return The IPC result JSON
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_json_any_t* sapi_ipc_result_get_json (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result JSON value "data" field.
   * @param result - An IPC request result
   * @param json   - The result JSON "data" field value
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_ipc_result_set_json_data (
    sapi_ipc_result_t* result,
    const sapi_json_any_t* json
  );

  /**
   * Get the result JSON value "data" field value.
   * @param result - An IPC request result
   * @return The result JSON "data" field value
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_json_any_t* sapi_ipc_result_get_json_data (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result JSON value "err"` field.
   * @param result - An IPC request result
   * @param json   - The result JSON "err" field value
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_ipc_result_set_json_error (
    sapi_ipc_result_t* result,
    const sapi_json_any_t* json
  );

  /**
   * Get the result JSON value "err" field value.
   * @param result - An IPC request result
   * @return The result JSON "err" field value
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_json_any_t* sapi_ipc_result_get_json_error (
    const sapi_ipc_result_t* result
  );

  /**
   * Set the IPC result bytes.
   * @param result - An IPC request result
   * @param size   - The size of the bytes
   * @param bytes  - The bytes
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_ipc_result_set_bytes (
    sapi_ipc_result_t* result,
    unsigned int size,
    unsigned char* bytes
  );

  /**
   * Get the IPC result bytes.
   * @param result - An IPC request result
   * @return THe IPC result bytes
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  unsigned char* sapi_ipc_result_get_bytes (
    const sapi_ipc_result_t* result
  );

  /**
   * Get the result bytes size.
   * @param result - An IPC request result
   * @return The size of the IPC result bytes
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  size_t sapi_ipc_result_get_bytes_size (
    const sapi_ipc_result_t* result
  );

  /**
   * Set an IPC result response header.
   * @param result - An IPC request result
   * @param name   - The name of the header
   * @param value  - The value of the header
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  void sapi_ipc_result_set_header (
    sapi_ipc_result_t* result,
    const char* name,
    const char* value
  );

  /**
   * Get an IPC result response header.
   * @param result - An IPC request result
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_ipc_result_t* sapi_ipc_result_from_json (
    sapi_context_t* context,
    sapi_ipc_message_t* message,
    const sapi_json_any_t* json
  );

  /**
   * Write a chunk to the HTTP response associated with the IPC result.
   *
   * ⚠️ You must have called `sapi_ipc_result_set_header` with a `Transfer-Encoding`
   * header of `chunked` to use this function.
   *
   * ⚠️ You must call `sapi_ipc_reply` before calling this function.
   *
   * ⚠️ This must be called on the main thread (using `sapi_context_dispatch` if necessary).
   *
   * Supported on iOS/macOS only.
   *
   * @param result      - An IPC request result
   * @param chunk       - The chunk to write
   * @param chunk_size  - The size of the chunk
   * @param finished    - `true` if this is the last chunk to write
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_send_chunk (
    sapi_ipc_result_t* result,
    const unsigned char* chunk,
    size_t chunk_size,
    bool finished
  );

  /**
   * Write an event to the HTTP response associated with the IPC result.
   *
   * ⚠️ You must have called `sapi_ipc_result_set_header` with a `Content-Type`
   * header of `text/event-stream` to use this function.
   *
   * ⚠️ You must call `sapi_ipc_reply` before calling this function.
   *
   * ⚠️ The `name` and `data` arguments must be null-terminated strings. Either
   * can be empty as long as it's null-terminated and the other is not empty.
   *
   * ⚠️ This must be called on the main thread (using `sapi_context_dispatch` if necessary).
   *
   * Supported on iOS/macOS only.
   *
   * @param result   - An IPC request result
   * @param name     - The event name
   * @param data     - The event data
   * @param finished - `true` if this is the last event to write
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_send_event (
    sapi_ipc_result_t* result,
    const char* name,
    const unsigned char* data,
    bool finished
  );

  /**
   * When an IPC route is triggered through HTTP, you can set a cancellation
   * handler for cleanup if the HTTP request is aborted.
   *
   * @param result  - An IPC request result
   * @param handler - The cancellation handler
   * @param data    - Optional user data
   */
  bool sapi_ipc_set_cancellation_handler (
    sapi_ipc_result_t* result,
    void (*handler)(void*),
    void* data
  );

  /**
   * Creates a "reply" for an IPC route request.
   * @param result - An IPC request result
   * @return `true` if successful, otherwise `false`
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_reply (const sapi_ipc_result_t* result);

  /**
   * Convenience method for replying with an error message.
    * @param result - An IPC request result
    * @param error  - An error message to include in the result
    * @return `true` if successful, otherwise `false`
    */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_reply_with_error (sapi_ipc_result_t* result, const char* error);

  /**
   * Send JSON to the bridge to propagate to the WebView.
   * @param context - An extension context
   * @param message - The IPC message this send request sources from
   * @param json    - The result JSON
   * @return `true` if successful, otherwise `false`
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_send_json (
    sapi_context_t* context,
    sapi_ipc_message_t* message,
    sapi_json_any_t* json
  );

  /**
   * Send JSON to the bridge to propagate to the WebView with a result.
   * @param context - An extension context
   * @param result  - The IPC request result
   * @param json    - The result JSON
   * @return `true` if successful, otherwise `false`
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_send_json_with_result (
    sapi_context_t* context,
    sapi_ipc_result_t* result,
    sapi_json_any_t* json
  );

  /**
   * Send bytes to the bridge to propagate to the WebView.
   * @param context - An extension context
   * @param message - The IPC message this send request sources from
   * @param size    - The size of the bytes
   * @param bytes   - The bytes
   * @param headers - The response headers
   * @return `true` if successful, otherwise `false`
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_send_bytes (
    sapi_context_t* context,
    sapi_ipc_message_t* message,
    unsigned int size,
    unsigned char* bytes,
    const char* headers
  );

  /**
   * Send bytes to the bridge to propagate to the WebView with a result.
   * @param context - An extension context
   * @param result  - The IPC request result
   * @param size    - The size of the bytes
   * @param bytes   - The bytes
   * @param headers - The response headers
   * @return `true` if successful, otherwise `false`
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_send_bytes_with_result (
    sapi_context_t* context,
    sapi_ipc_result_t* result,
    unsigned int size,
    unsigned char* bytes,
    const char* headers
  );

  /**
   * Emit IPC `event` with `data`
   * @param context - An extension context
   * @param name    - The event name to emit on the webview `globalThis` object
   * @param data    - Event data to emit
   * @return `true` if successful, otherwise `false`
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_emit (
    sapi_context_t* context,
    const char* name,
    const char* data
  );

  /**
   * Invoke an IPC route with optional `bytes` and `size`. IPC paramters should
   * be included in the URI (eg `ipc://domain.command?key=&value&key=value`)
   * @param context - An extension context
   * @param url     - IPC URI to invoke
   * @param size    - Optional size of `bytes`
   * @param bytes   - IPC bytes to include in message
   * @return `true` if successful, otherwise `false`
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_invoke (
    sapi_context_t* context,
    const char* url,
    unsigned int size,
    const char* bytes,
    sapi_ipc_router_result_callback_t callback
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_router_map (
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_ipc_router_unmap (
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
   * An opaque pointer for a spawned child process.
   */
  typedef struct sapi_process_spawn sapi_process_spawn_t;

  /**
   * TODO
   */
  typedef void (*sapi_process_spawn_stdout_callback_t)(
    const sapi_process_spawn_t* process,
    const char* output,
    unsigned int size
  );

  /**
   * TODO
   */
  typedef void (*sapi_process_spawn_stderr_callback_t)(
    const sapi_process_spawn_t* process,
    const char* output,
    unsigned int size
  );

  /**
   * TODO
   */
  typedef void (*sapi_process_spawn_exit_callback_t)(
    const sapi_process_spawn_t* process,
    int exit_code
  );

  /**
   * Execute `command` as a child process. stdout and stderr are both captured
   * as interleaved output.
   * @param context - An extension context
   * @param command - A system shell command to execute
   * @return The process execution result with output.
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_process_exec_t* sapi_process_exec (
    sapi_context_t* context,
    const char* command
  );

  /**
   * Get the exit code of an execution result.
   * @param process - The process execution result
   * @return The exit code of the process
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  int sapi_process_exec_get_exit_code (
    const sapi_process_exec_t* process
  );

  /**
   * Get the output of an execution result.
   * @param process - The process execution result
   * @return The output of the process
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const char* sapi_process_exec_get_output (const sapi_process_exec_t* process);

  /**
   * Spawn a child process with `command`, a `NULL` terminated list of
   * arguments, and callbacks for stdout and stderr
   * @param TODO
   * @return TODO
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_process_spawn_t* sapi_process_spawn (
    sapi_context_t* context,
    const char* command,
    const char* argv,
    const char* path,
    sapi_process_spawn_stdout_callback_t onstdout,
    sapi_process_spawn_stderr_callback_t onstderr,
    sapi_process_spawn_exit_callback_t onexit
  );

  /**
   * Get the exit code of a spawned process.
   * @param process - The spawned process result
   * @return The exit code of the process
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  int sapi_process_spawn_get_exit_code (
    const sapi_process_spawn_t* process
  );

  SOCKET_RUNTIME_EXTENSION_EXPORT
  unsigned long sapi_process_spawn_get_pid (
    const sapi_process_spawn_t* process
  );

  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_context_t* sapi_process_spawn_get_context (
    const sapi_process_spawn_t* process
  );

  SOCKET_RUNTIME_EXTENSION_EXPORT
  int sapi_process_spawn_wait (
    const sapi_process_spawn_t* process
  );

  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_process_spawn_write (
    const sapi_process_spawn_t* process,
    const char* bytes,
    unsigned long size
  );

  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_process_spawn_close_stdin (
    const sapi_process_spawn_t* process
  );

  /**
   * TODO
   * @param TODO
   * @return TODO
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_process_spawn_kill (
    sapi_process_spawn_t* process,
    int code
  );

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
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
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
  SOCKET_RUNTIME_EXTENSION_EXPORT
  bool sapi_extension_unload (sapi_context_t* context, const char *name);

  /**
   * Get an extensions registration info
   * @param context - An extension context
   * @param name    - The name of the extension
   * @return The extension registration info
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  const sapi_extension_registration_t* sapi_extension_get (
    const sapi_context_t* context,
    const char *name
  );

  /**
   * Clone an IPC message
   * @param context - An extension context
   * @param message - The message to clone
   * @return The cloned message
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_ipc_message_t* sapi_ipc_message_clone (
    sapi_context_t* context,
    const sapi_ipc_message_t* message
  );

  /**
   * Clone an IPC result
   * @param context - An extension context
   * @param result  - The IPC request result to clone
   */
  SOCKET_RUNTIME_EXTENSION_EXPORT
  sapi_ipc_result_t* sapi_ipc_result_clone (
    sapi_context_t* context,
    const sapi_ipc_result_t* result
  );

SOCKET_RUNTIME_EXTENSION_EXTERN_END

#endif
