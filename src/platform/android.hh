#ifndef SOCKET_RUNTIME_PLATFORM_ANDROID_H
#define SOCKET_RUNTIME_PLATFORM_ANDROID_H

#include "android/content_resolver.hh"
#include "android/environment.hh"
#include "android/looper.hh"
#include "android/mime.hh"
#include "android/string_wrap.hh"
#include "android/types.hh"

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
