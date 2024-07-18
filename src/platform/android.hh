#ifndef SOCKET_RUNTIME_PLATFORM_ANDROID_H
#define SOCKET_RUNTIME_PLATFORM_ANDROID_H

#include "platform.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "android/environment.hh"
#include "android/looper.hh"
#include "android/mime.hh"
#include "android/string_wrap.hh"

namespace SSC::Android {
  /**
   * An Android AssetManager NDK type.
   */
  using AssetManager = ::AAssetManager;

  /**
   * An Android AssetManager Asset NDK type.
   */
  using Asset = ::AAsset;

  /**
   * An Android AssetManager AssetDirectory NDK type.
   */
  using AssetDirectory = ::AAssetDir;

  /**
   * An opaque `Activity` instance.
   */
  using Activity = ::jobject;

  /**
   * An opaque `Application` instance.
   */
  using Application = ::jobject;

  /**
   * A container that holds Android OS build information.
   */
  struct BuildInformation {
    String brand;
    String device;
    String fingerprint;
    String hardware;
    String model;
    String manufacturer;
    String product;
  };
}
#endif

/**
 * A macro to help define an Android external package class method
 * implementation as a JNI binding.
 */
#define ANDROID_EXTERNAL(packageName, className, methodName)                   \
  JNIEXPORT JNICALL                                                            \
  Java_socket_runtime_##packageName##_##className##_##methodName

/**
 * Throw an exception with formatted message in the Android environment.
 * @param {JNIEnv*} env
 * @param {const char*} message
 * @param {...}
 */
#define ANDROID_THROW(env, message, ...) ({                                    \
  char buffer[BUFSIZ] = {0};                                                   \
  sprintf(buffer, message, ##__VA_ARGS__);                                     \
  env->ThrowNew(                                                               \
    GetExceptionClassFromAndroidEnvironment(env),                              \
    buffer                                                                     \
  );                                                                           \
  (void) 0;                                                                    \
})

#endif
