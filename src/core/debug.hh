#ifndef SSC_CORE_DEBUG_H
#define SSC_CORE_DEBUG_H

#include "config.hh"
#include "platform.hh"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <OSLog/OSLog.h>

// Apple
#ifndef debug
// define `ssc_os_debug` (macos/ios)
#if defined(SSC_CLI)
#  define ssc_os_debug(...)
#else
static os_log_t SSC_OS_LOG_DEBUG_BUNDLE = nullptr;
// wrap `os_log*` functions for global debugger
#define ssc_os_debug(format, fmt, ...) ({                                      \
  if (!SSC_OS_LOG_DEBUG_BUNDLE) {                                              \
    static auto userConfig = SSC::getUserConfig();                             \
    static auto bundleIdentifier = userConfig["meta_bundle_identifier"];       \
    SSC_OS_LOG_DEBUG_BUNDLE = os_log_create(                                   \
      bundleIdentifier.c_str(),                                                \
      "socket.runtime.debug"                                                   \
    );                                                                         \
  }                                                                            \
                                                                               \
  auto string = [NSString stringWithFormat: @fmt, ##__VA_ARGS__];              \
  os_log_error(                                                                \
    SSC_OS_LOG_DEBUG_BUNDLE,                                                   \
    "%{public}s",                                                              \
    string.UTF8String                                                          \
  );                                                                           \
})
#endif

// define `debug(...)` macro
#define debug(format, ...) ({                                                  \
  NSLog(@format, ##__VA_ARGS__);                                               \
  ssc_os_debug("%{public}@", format, ##__VA_ARGS__);                           \
})
#endif // `debug`
#endif  // `__APPLE__`

// Linux
#if defined(__linux__) && !defined(__ANDROID__)
#  ifndef debug
#    define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#  endif // `debug`
#endif // `__linux__`

// Android (Linux)
#if defined(__linux__) && defined(__ANDROID__)
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
#if defined(_WIN32)
#  if defined(_WIN32) && defined(DEBUG)
#    define _WIN32_DEBUG 1
#  endif // `_WIN32 && DEBUG`
#  ifndef debug
#    define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#  endif // `debug`
#endif // `_WIN32`

#endif
