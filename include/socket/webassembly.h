#ifndef SOCKET_RUNTIME_WEBASSEMBLY_H
#define SOCKET_RUNTIME_WEBASSEMBLY_H

typedef unsigned long size_t;
typedef long ssize_t;

/* Exact-width integer types */
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;

struct uv_loop;
typedef struct uv_loop uv_loop_t;

#if !defined(NULL)
#define NULL 0
#endif

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
