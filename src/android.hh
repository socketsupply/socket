#ifndef SSC_ANDROID_H
#define SSC_ANDROID_H

#ifndef __ANDROID__
#define __ANDROID__
#endif

#ifndef ANDROID
#define ANDROID 1
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef SETTINGS
#define SETTINGS ""
#endif

#ifndef VERSION
#define VERSION ""
#endif

#ifndef VERSION_HASH
#define VERSION_HASH ""
#endif

/**
 * Java Native Interface
 * @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
 * @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html
 * @see https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp276
 */
#include <jni.h>
#include <stdint.h>

#include "core.hh"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#define debug(format, ...)                                                     \
  {                                                                            \
    __android_log_print(                                                       \
      ANDROID_LOG_DEBUG, __FUNCTION__, format, ##__VA_ARGS__                   \
    );                                                                         \
  }

/**
 * @TODO
 */
#define VITAL_CHECK_FILE "vital_check_ok.txt"

/**
 * Defined by the Socket SDK preprocessor
 */
#define PACKAGE_NAME __BUNDLE_IDENTIFIER__

/**
 * Creates a named native package export for the configured bundle suitable for
 * for definition only.
 * @param name The name of the package function to export
 */
#define exports(namespace, name)                                               \
  JNIEXPORT JNICALL Java___BUNDLE_IDENTIFIER___##namespace##_##name

/**
 * Gets the `NativeCore` binding class.
 */
#define GetNativeCoreBindingClass(env) env->GetObjectClass(self)

/**
 * Gets `NativeCore` instance pointer from environment.
 */
#define GetNativeCoreFromEnvironment(env)                                      \
  ({                                                                           \
    auto Class = GetNativeCoreBindingClass(env);                               \
    auto pointerField = env->GetFieldID(Class, "pointer", "J");                \
    auto core = (NativeCore *) env->GetLongField(self, pointerField);          \
    (core);                                                                    \
  })

/**
 * Gets JNI `Exception` class from environment.
 */
#define GetExceptionClass(env)                                                 \
  ({                                                                           \
    auto Exception = env->FindClass("java/lang/Exception");                    \
    (Exception);                                                               \
  })

/**
 * Calls method on `NativeCore` instance in environment.
 */
#define CallNativeCoreMethodFromEnvironment(env, object, method, signature)    \
  ({                                                                           \
    auto Class = env->GetObjectClass(object);                                  \
    auto id = env->GetMethodID(Class, method, signature);                      \
    auto value = env->CallObjectMethod(object, id);                            \
    (value);                                                                   \
  })

/**
 * Generic `Exception` throw helper
 */
#define Throw(env, E) env->ThrowNew(GetExceptionClass(env), E)

/**
 * `NativeCoreNotInitialized` Exception
 */
#define NativeCoreNotInitializedError "NativeCore is not initialized"

/**
 * `UVLoopNotInitialized` Exception
 */
#define UVLoopNotInitializedError "UVLoop is not initialized"

/**
 * `ExceptionCheck` Exception
 */
#define ExceptionCheckError "ExceptionCheck"

/**
 * `AssetManagerIsNotReachable` Exception
 */
#define AssetManagerIsNotReachableError "AssetManager is not reachable through binding"

/**
 * `RootDirectoryIsNotReachable` Exception
 */
#define RootDirectoryIsNotReachableError "Root directory in file system is not reachable through binding"

/**
 * `UVError` Exception
 */
#define UVError(code) uv_strerror(code)

/**
 * `JavaScriptPreloadSourceNotInitialized` Exception
 */
#define JavaScriptPreloadSourceNotInitializedError "JavaScript preload source is not initialized"

#endif
