#ifndef SOCKET_RUNTIME_CORE_DEBUG_H
#define SOCKET_RUNTIME_CORE_DEBUG_H

#include "config.hh"

#if SOCKET_RUNTIME_PLATFORM_APPLE
// Apple
#ifndef debug
// define `socket_runtime_os_log_debug` (macos/ios)
#if defined(SSC_CLI)
#  define socket_runtime_os_log_debug(...)
#else
static os_log_t SOCKET_RUNTIME_OS_LOG_DEBUG = nullptr;
// wrap `os_log*` functions for global debugger
#define socket_runtime_os_log_debug(format, fmt, ...) ({                       \
  if (!SOCKET_RUNTIME_OS_LOG_DEBUG) {                                          \
    static auto userConfig = SSC::getUserConfig();                             \
    static auto bundleIdentifier = userConfig["meta_bundle_identifier"];       \
    SOCKET_RUNTIME_OS_LOG_DEBUG = os_log_create(                               \
      bundleIdentifier.c_str(),                                                \
      "socket.runtime.debug"                                                   \
    );                                                                         \
  }                                                                            \
                                                                               \
  auto string = [NSString stringWithFormat: @fmt, ##__VA_ARGS__];              \
  os_log_error(                                                                \
    SOCKET_RUNTIME_OS_LOG_DEBUG,                                               \
    "%{public}s",                                                              \
    string.UTF8String                                                          \
  );                                                                           \
})
#endif

// define `debug(...)` macro
#define debug(format, ...) ({                                                  \
  NSLog(@format, ##__VA_ARGS__);                                               \
  socket_runtime_os_log_debug("%{public}@", format, ##__VA_ARGS__);            \
})
#endif // `debug`
#endif  // `__APPLE__`

// Linux
#if SOCKET_RUNTIME_PLATFORM_LINUX
#  ifndef debug
#    define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#  endif // `debug`
#endif // `__linux__`

// Android (Linux)
#if SOCKET_RUNTIME_PLATFORM_ANDROID
#  ifndef debug
#    define debug(format, ...)                                                 \
      __android_log_print(                                                     \
          ANDROID_LOG_DEBUG,                                                   \
          "Console",                                                           \
          format,                                                              \
          ##__VA_ARGS__                                                        \
        );
#  endif // `debug`
#endif // `__ANDROID__`

// Windows
#if SOCKET_RUNTIME_PLATFORM_WINDOWS && defined(DEBUG)
#  define _WIN32_DEBUG 1
#endif // `_WIN32 && DEBUG`
#ifndef debug
#  define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif // `debug`
#endif
