#ifndef SOCKET_RUNTIME_PLATFORM_PLATFORM_H
#define SOCKET_RUNTIME_PLATFORM_PLATFORM_H

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
#include <objc/runtime.h>
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
  #if SOCKET_RUNTIME_DESKTOP_EXTENSION
  #include <webkit2/webkit-web-extension.h>
  #else
  #include <webkit2/webkit2.h>
  #endif
#endif // `__linux__`

// Android (Linux)
#if defined(__ANDROID__)
// Java Native Interface
// @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/looper.h>
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
#include <fileapi.h>
#include <io.h>
#include <objidl.h>
#include <signal.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <tchar.h>
#include <urlmon.h>
#include <uxtheme.h>
#include <wingdi.h>
#include <wrl.h>

#if !defined(SOCKET_RUNTIME_EXTENSION)
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#endif

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "libllama.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "WebView2LoaderStatic.lib")
#pragma comment(lib, "Ws2_32.lib")

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

#include <socket/platform.h>
#include "string.hh"
#include "types.hh"

#if !SOCKET_RUNTIME_PLATFORM_WINDOWS
#include <dlfcn.h>
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
  void msleep (uint64_t ms);
  uint64_t rand64 ();
}

#endif
