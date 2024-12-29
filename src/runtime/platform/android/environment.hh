#ifndef SOCKET_RUNTIME_PLATFORM_ANDROID_ENVIRONMENT_H
#define SOCKET_RUNTIME_PLATFORM_ANDROID_ENVIRONMENT_H

#include "../types.hh"
#include "native.hh"

/**
 * Gets class for object for `self` from `env`.
 * @param {JNIEnv*} env
 * @param {jobject} self
 */
#define GetObjectClassFromAndroidEnvironment(env, self) ({                     \
  env->GetObjectClass(self);                                                   \
})

/**
 * Get field on object `self` from `env`.
 * @param {JNIEnv*} env
 * @param {jobject} self
 * @param {typeof any} Type
 * @param {const char*} field
 * @param {const char*} sig
 */
#define GetObjectClassFieldFromAndroidEnvironment(                             \
  env,                                                                         \
  self,                                                                        \
  Type,                                                                        \
  field,                                                                       \
  sig                                                                          \
) ({                                                                           \
  auto Class = GetObjectClassFromAndroidEnvironment(env, self);                \
  auto id = env->GetFieldID(Class, field, sig);                                \
  env->Get##Type##Field(self, id);                                             \
})

/**
 * Gets the JNI `Exception` class from environment.
 * @param {JNIEnv*} env
 * @param {jobject} self
 */
#define GetExceptionClassFromAndroidEnvironment(env) ({                        \
  env->FindClass("java/lang/Exception");                                       \
})

/**
 * @param {JNIEnv*} env
 * @param {jobject} object
 * @param {const char*} method
 * @param {const char* sig
 * @param {...any} ...
 */
#define CallObjectClassMethodFromAndroidEnvironment(                           \
  env,                                                                         \
  object,                                                                      \
  method,                                                                      \
  sig,                                                                         \
  ...                                                                          \
) ({                                                                           \
  auto Class = env->GetObjectClass(object);                                    \
  auto ID = env->GetMethodID(Class, method, sig);                              \
  env->CallObjectMethod(object, ID, ##__VA_ARGS__);                            \
})

/**
 * @param {JNIEnv*} env
 * @param {Object|Boolean|Int|Float|Void} Type
 * @param {jobject} object
 * @param {const char*} method
 * @param {const char* sig
 * @param {...any} ...
 */
#define CallClassMethodFromAndroidEnvironment(                                 \
  env,                                                                         \
  Type,                                                                        \
  object,                                                                      \
  method,                                                                      \
  sig,                                                                         \
  ...                                                                          \
) ({                                                                           \
  auto Class = env->GetObjectClass(object);                                    \
  auto ID = env->GetMethodID(Class, method, sig);                              \
  env->Call##Type##Method(object, ID, ##__VA_ARGS__);                          \
})
/**
 * @param {JNIEnv*} env
 * @param {jobject} object
 * @param {const char*} method
 * @param {const char* sig
 * @param {...any} ...
 */
#define CallVoidClassMethodFromAndroidEnvironment(                             \
  env,                                                                         \
  object,                                                                      \
  method,                                                                      \
  sig,                                                                         \
  ...                                                                          \
) ({                                                                           \
  auto Class = env->GetObjectClass(object);                                    \
  auto ID = env->GetMethodID(Class, method, sig);                              \
  env->CallVoidMethod(object, ID, ##__VA_ARGS__);                              \
})

namespace ssc::android {
  /**
   * A `JVMEnvironment` constainer holders a poitner the current `JavaVM`
   * instance for the current `JNIEnv`.
   */
  struct JVMEnvironment {
    JavaVM* jvm = nullptr;
    JNIEnv* env = nullptr;
    int jniVersion = 0;

    JVMEnvironment () = default;
    JVMEnvironment (JNIEnv* env);
    JVMEnvironment (JavaVM* jvm);
    int version () const;
    JavaVM* get () const;
  };

  /**
   * A `JNIEnvironmentAttachment` container can attach to the current thread
   * for the `JavaVM` instance. This is useful to run code on the same thread
   * as the Android application.
   */
  struct JNIEnvironmentAttachment {
    JavaVM *jvm = nullptr;
    JNIEnv *env = nullptr;
    int status = 0;
    int version = 0;
    Atomic<bool> attached = false;

    JNIEnvironmentAttachment () = default;
    JNIEnvironmentAttachment (const JVMEnvironment& jvm);
    JNIEnvironmentAttachment (JavaVM *jvm, int version);
    JNIEnvironmentAttachment (const JNIEnvironmentAttachment&) = delete;
    JNIEnvironmentAttachment (JNIEnvironmentAttachment&&);
    ~JNIEnvironmentAttachment ();
    JNIEnvironmentAttachment& operator= (const JNIEnvironmentAttachment&) = delete;
    JNIEnvironmentAttachment& operator= (JNIEnvironmentAttachment&&);

    void attach (JavaVM *jvm, int version);
    void detach ();

    bool hasException () const;
    void printException () const;
  };
}
#endif
