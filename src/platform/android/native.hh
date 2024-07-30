#ifndef SOCKET_RUNTIME_PLATFORM_ANDROID_NATIVE_H
#define SOCKET_RUNTIME_PLATFORM_ANDROID_NATIVE_H
#if defined(__ANDROID__)
// Java Native Interface
// @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/looper.h>
#endif
#endif
