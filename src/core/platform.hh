#ifndef SSC_CORE_PLATFORM_H
#define SSC_CORE_PLATFORM_H

// All Platforms
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

// Apple (macOS/iOS)
#if defined(__APPLE__)
#include <CoreLocation/CoreLocation.h>
#include <CoreBluetooth/CoreBluetooth.h>
#include <Foundation/Foundation.h>
#include <Network/Network.h>
#include <OSLog/OSLog.h>
#include <TargetConditionals.h>
#include <UserNotifications/UserNotifications.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <Webkit/Webkit.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#include <_types/_uint64_t.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <UIKit/UIKit.h>
#else
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <objc/objc-runtime.h>
#endif // `TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR`
#endif // `__APPLE__`

// Linux
#if defined(__linux__) && !defined(__ANDROID__)
#include <cstring>
#include <JavaScriptCore/JavaScript.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <webkit2/webkit2.h>
#endif // `__linux__`

// Android (Linux)
#if defined(__ANDROID__)
// Java Native Interface
// @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#endif // `__ANDROID__`

// Windows
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#undef _WINSOCKAPI_
#define _WINSOCKAPI_

#include <WinSock2.h>
#include <windows.h>

#include <dwmapi.h>
#include <io.h>
#include <tchar.h>
#include <wingdi.h>

#include <signal.h>
#include <shlobj_core.h>
#include <shobjidl.h>

#if !defined(SOCKET_RUNTIME_EXTENSION)
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#endif

#include <shellapi.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "WebView2LoaderStatic.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "dbghelp.lib")

#define isatty _isatty
#define fileno _fileno
#endif // `_WIN32`


// POSIX[like] Platforms
#if !defined(_WIN32)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "config.hh"
#include "types.hh"

#if defined(_WIN32)
# define SSC_PLATFORM_NAME "win32"
# define SSC_PLATFORM_OS "win32"
# define SSC_PLATFORM_ANDROID 0
# define SSC_PLATFORM_APPLE 0
# define SSC_PLATFORM_IOS 0
# define SSC_PLATFORM_IOS_SIMULATOR 0
# define SSC_PLATFORM_LINUX 0
# define SSC_PLATFORM_MACOS 0
# define SSC_PLATFORM_UXIX 0
# define SSC_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
# include <TargetConditionals.h>
# define SSC_PLATFORM_APPLE 1
# define SSC_PLATFORM_NAME "darwin"
# define SSC_PLATFORM_ANDROID 0
# define SSC_PLATFORM_IOS_SIMULATOR 0
# define SSC_PLATFORM_LINUX 0
# define SSC_PLATFORM_WINDOWS 0

#if TARGET_OS_IPHONE
# define SSC_PLATFORM_MACOS 0
# define SSC_PLATFORM_IOS 1
# define SSC_PLATFORM_IOS_SIMULATOR 0
# define SSC_PLATFORM_OS "ios"
#elif TARGET_IPHONE_SIMULATOR
# define SSC_PLATFORM_MACOS 0
# define SSC_PLATFORM_IOS 1
# define SSC_PLATFORM_IOS_SIMULATOR 1
# define SSC_PLATFORM_OS "ios"
#else
# define SSC_PLATFORM_MACOS 1
# define SSC_PLATFORM_IOS 0
# define SSC_PLATFORM_IOS_SIMULATOR 0
# define SSC_PLATFORM_OS "mac"
#endif

#if defined(__unix__) || defined(unix) || defined(__unix)
# define SSC_PLATFORM_UXIX 1
#else
# define SSC_PLATFORM_UXIX 0
#endif

#elif defined(__linux__)
# undef linux
# define SSC_PLATFORM_NAME "linux"
# define SSC_PLATFORM_APPLE 0
# define SSC_PLATFORM_IOS 0
# define SSC_PLATFORM_IOS_SIMULATOR 0
# define SSC_PLATFORM_MACOS 0
# define SSC_PLATFORM_WINDOWS 0

#ifdef __ANDROID__
# define SSC_PLATFORM_OS "android"
# define SSC_PLATFORM_ANDROID 1
# define SSC_PLATFORM_LINUX 0
#else
# define SSC_PLATFORM_OS "linux"
# define SSC_PLATFORM_ANDROID 0
# define SSC_PLATFORM_LINUX 1
#endif

#if defined(__unix__) || defined(unix) || defined(__unix)
# define SSC_PLATFORM_UXIX 1
#else
# define SSC_PLATFORM_UXIX 0
#endif

#elif defined(__FreeBSD__)
# define SSC_PLATFORM_NAME "freebsd"
# define SSC_PLATFORM_OS "freebsd"
# define SSC_PLATFORM_ANDROID 0
# define SSC_PLATFORM_IOS 0
# define SSC_PLATFORM_IOS_SIMULATOR 0
# define SSC_PLATFORM_LINUX 0
# define SSC_PLATFORM_MACOS 0
# define SSC_PLATFORM_WINDOWS 0

#if defined(__unix__) || defined(unix) || defined(__unix)
# define SSC_PLATFORM_UXIX 0
#else
# define SSC_PLATFORM_UXIX 1
#endif

#elif defined(BSD)
# define SSC_PLATFORM_NAME "openbsd"
# define SSC_PLATFORM_OS "openbsd"
# define SSC_PLATFORM_ANDROID 0
# define SSC_PLATFORM_APPLE 0
# define SSC_PLATFORM_IOS 0
# define SSC_PLATFORM_IOS_SIMULATOR 0
# define SSC_PLATFORM_LINUX 0
# define SSC_PLATFORM_MACOS 0
# define SSC_PLATFORM_WINDOWS 0

#if defined(__unix__) || defined(unix) || defined(__unix)
# define SSC_PLATFORM_UXIX 0
#else
# define SSC_PLATFORM_UXIX 1
#endif
#endif

#if SSC_PLATFORM_ANDROID || SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
#define SSC_PLATFORM_MOBILE 1
#define SSC_PLATFORM_DESKTOP 0
#else
#define SSC_PLATFORM_MOBILE 0
#define SSC_PLATFORM_DESKTOP 1
#endif

#if !defined(SSC_PLATFORM_SANDBOXED)
#define SSC_PLATFORM_SANDBOXED 0
#endif

namespace SSC {
  struct RuntimePlatform {
    const String arch = "";
    const String os = "";
    bool mac = false;
    bool ios = false;
    bool win = false;
    bool android = false;
    bool linux = false;
    bool unix = false;
  };

  extern const RuntimePlatform platform;
}

#endif
