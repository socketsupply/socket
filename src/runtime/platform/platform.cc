#include "../platform.hh"

namespace ssc::runtime {
  extern const RuntimePlatform platform = {
  #if defined(__x86_64__) || defined(_M_X64)
    .arch = "x86_64",
  #elif defined(__aarch64__) || defined(_M_ARM64)
    .arch = "arm64",
  #elif defined(__i386__) && !defined(__ANDROID__)
  #  error Socket is not supported on i386.
  #else
    .arch = "unknown",
  #endif

  // Windows
  #if defined(_WIN32)
    .os = "win32",
    .win = true,
  #endif

  // macOS & iOS
  #if defined(__APPLE__)
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    .os = "ios",
    .ios = true,
    #else
    .os = "mac",
    .mac = true,
    #endif
    #if defined(__unix__) || defined(unix) || defined(__unix)
    .unix = true
    #else
    .unix = false
    #endif
  #endif

  // Android & Linux
  #if defined(__linux__)
    #undef linux
    #ifdef __ANDROID__
    .os = "android",
    .android = true,
    .linux = true,
    #else
    .os = "linux",
    .linux = true,
    #endif

    #if defined(__unix__) || defined(unix) || defined(__unix)
    .unix = true
    #else
    .unix = false
    #endif
  #endif

  // FreeBSD
  #if defined(__FreeBSD__)
    .os = "freebsd",
    #if defined(__unix__) || defined(unix) || defined(__unix)
    .unix = true
    #else
    .unix = false
    #endif
  #endif

  // OpenBSD (possibly)
  #if !defined(__APPLE__) && defined(BSD) && (defined(__unix__) || defined(unix) || defined(__unix))
    .os = "openbsd",
    .unix = true
  #endif
  };

  void msleep (uint64_t ms) {
    std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
}
