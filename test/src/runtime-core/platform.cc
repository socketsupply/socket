#include "tests.hh"
#include "src/core/platform.hh"

namespace SSC::Tests {
  void platform (const Harness& t) {
    t.test("SSC::platform.arch", [](auto t) {
      t.assert(SSC::platform.arch, "SSC::platform.arch is not empty");
    #if defined(__x86_64__) || defined(_M_X64)
      t.equals(SSC::platform.arch, "x86_64", "SSC::platform.arch == \"x86_64\"");
    #elif defined(__aarch64__) || defined(_M_ARM64)
      t.equals(SSC::platform.arch , "arm64", "SSC::platform.arch == \"arm64\"");
    #else
      t.equals(SSC::platform.arch , "unknown", "SSC::platform.arch == \"unknown\"");
    #endif
    });

    t.test("SSC::platform.os", [](auto t) {
      t.assert(SSC::platform.os, "SSC::platform.osis not empty");
    #if defined(_WIN32)
      t.equals(SSC::platform.os, "win32", "SSC::platform.os == \"win32\"");
    #elif defined(__APPLE__)
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      t.equals(SSC::platform.os, "ios", "SSC::platform.os == \"ios\"");
      #else
      t.equals(SSC::platform.os, "mac", "SSC::platform.os == \"mac\"");
      #endif
    #elif defined(__ANDROID__)
      t.equals(SSC::platform.os, "android", "SSC::platform.os == \"android\"");
    #elif defined(__linux__)
      t.equals(SSC::platform.os, "linux", "SSC::platform.os == \"linux\"");
    #elif defined(__FreeBSD__)
      t.equals(SSC::platform.os, "freebsd", "SSC::platform.os == \"freebsd\"");
    #elif defined(BSD)
      t.equals(SSC::platform.os, "openbsd", "SSC::platform.os == \"openbsd\"");
    #endif
    });

    t.test("SSC::platform.{mac,ios,win,linux,unix}", [](auto t) {
    #if defined(_WIN32)
      t.equals(SSC::platform.mac, false, "SSC::platform.mac = false");
      t.equals(SSC::platform.ios, false, "SSC::platform.ios = false");
      t.equals(SSC::platform.win, true, "SSC::platform.win = true");
      t.equals(SSC::platform.linux, false, "SSC::platform.linux = false");
      t.equals(SSC::platform.android, false, "SSC::platform.android = false");
    #elif defined(__APPLE__)
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      t.equals(SSC::platform.mac, false, "SSC::platform.mac = false");
      t.equals(SSC::platform.ios, true, "SSC::platform.ios = true");
      t.equals(SSC::platform.win, false, "SSC::platform.win = false");
      t.equals(SSC::platform.linux, false, "SSC::platform.linux = false");
      t.equals(SSC::platform.android, false, "SSC::platform.android = false");
      #else
      t.equals(SSC::platform.mac, true, "SSC::platform.mac = true");
      t.equals(SSC::platform.ios, false, "SSC::platform.ios = false");
      t.equals(SSC::platform.win, false, "SSC::platform.win = false");
      t.equals(SSC::platform.linux, false, "SSC::platform.linux = false");
      t.equals(SSC::platform.android, false, "SSC::platform.android = false");
      #endif
    #elif defined(__ANDROID__)
      t.equals(SSC::platform.mac, false, "SSC::platform.mac = false");
      t.equals(SSC::platform.ios, false, "SSC::platform.ios = false");
      t.equals(SSC::platform.win, false, "SSC::platform.win = false");
      t.equals(SSC::platform.linux, true, "SSC::platform.linux = true");
      t.equals(SSC::platform.android, true, "SSC::platform.android = true");
    #elif defined(__linux__)
      t.equals(SSC::platform.mac, false, "SSC::platform.mac = false");
      t.equals(SSC::platform.ios, false, "SSC::platform.ios = false");
      t.equals(SSC::platform.win, false, "SSC::platform.win = false");
      t.equals(SSC::platform.linux, true, "SSC::platform.linux = true");
      t.equals(SSC::platform.android, false, "SSC::platform.android = false");
    #elif defined(__FreeBSD__)
      t.equals(SSC::platform.mac, false, "SSC::platform.mac = false");
      t.equals(SSC::platform.ios, false, "SSC::platform.ios = false");
      t.equals(SSC::platform.win, false, "SSC::platform.win = false");
      t.equals(SSC::platform.linux, false, "SSC::platform.linux = false");
      t.equals(SSC::platform.android, false, "SSC::platform.android = false");
    #elif defined(BSD)
      t.equals(SSC::platform.mac, false, "SSC::platform.mac = false");
      t.equals(SSC::platform.ios, false, "SSC::platform.ios = false");
      t.equals(SSC::platform.win, false, "SSC::platform.win = false");
      t.equals(SSC::platform.linux, false, "SSC::platform.linux = false");
      t.equals(SSC::platform.android, false, "SSC::platform.android = false");
    #endif

    #if defined(__unix__) || defined(unix) || defined(__unix)
      t.equals(SSC::platform.unix, true, "SSC::platform.unix = true");
    #else
      t.equals(SSC::platform.unix, false, "SSC::platform.unix = false");
    #endif
    });
  }
}
