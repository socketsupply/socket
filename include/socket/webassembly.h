#ifndef SOCKET_RUNTIME_WEBASSEMBLY_H
#define SOCKET_RUNTIME_WEBASSEMBLY_H

#include "webassembly/stddef.h"
#include "webassembly/stdint.h"
#include "webassembly/string.h"
#include "webassembly/stdio.h"
#include "webassembly/errno.h"

struct uv_loop;
typedef struct uv_loop uv_loop_t;

// platform
#define SOCKET_RUNTIME_PLATFORM_NAME "wasm32"
#define SOCKET_RUNTIME_PLATFORM_OS "web"
#define SOCKET_RUNTIME_PLATFORM_ANDROID 0
#define SOCKET_RUNTIME_PLATFORM_IOS 0
#define SOCKET_RUNTIME_PLATFORM_IOS_SIMULATOR 0
#define SOCKET_RUNTIME_PLATFORM_LINUX 0
#define SOCKET_RUNTIME_PLATFORM_MACOS 0
#define SOCKET_RUNTIME_PLATFORM_WINDOWS 0
#define SOCKET_RUNTIME_PLATFORM_UXIX 0
// when this header is included, this is always `1`
#define SOCKET_RUNTIME_PLATFORM_WASM 1

// arch
#define SOCKET_RUNTIME_ARCH_x64 0
#define SOCKET_RUNTIME_ARCH_ARM64 0
#define SOCKET_RUNTIME_ARCH_UNKNOWN 1

#endif
