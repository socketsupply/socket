#ifndef SSC_ANDROID_H
#define SSC_ANDROID_H

#ifndef __ANDROID__
#  define __ANDROID__
#endif

#ifndef ANDROID
#  define ANDROID 1
#endif

#ifndef DEBUG
#  define DEBUG 0
#endif

#ifndef SETTINGS
#  define SETTINGS ""
#endif

#ifndef VERSION
#  define VERSION ""
#endif

#ifndef VERSION_HASH
#  define VERSION_HASH ""
#endif

/**
 * Java Native Interface
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp276
 */
#include <jni.h>

// libc
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// c++
#include <any>
#include <map>
#include <string>

// android
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

// SSC
#include "common.hh"
#include "core.hh"

typedef std::map<std::string, std::string> AppConfig;
typedef std::map<std::string, std::string> EnvironmentVariables;

typedef std::string NativeCoreSequence;
typedef jlong NativeCallbackID;
typedef uint64_t NativeCoreID;

// Forward declaration
class NativeFileSystem;
class NativeCore;

/**
 * @TODO
 */
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
 * Gets the `NativeCore` binding class from the JNI environment.
 */
#define GetNativeCoreBindingClass(env) env->GetObjectClass(self)

/**
 * Gets `NativeCore` instance pointer from JNI environment.
 */
#define GetNativeCoreFromEnvironment(env)                                      \
  ({                                                                           \
    auto Class = GetNativeCoreBindingClass(env);                               \
    auto pointerField = env->GetFieldID(Class, "pointer", "J");                \
    auto core = (NativeCore *) env->GetLongField(self, pointerField);          \
    (core);                                                                    \
  })

/**
 * Gets the JNI `Exception` class from environment.
 */
#define GetExceptionClass(env)                                                 \
  ({                                                                           \
    auto Exception = env->FindClass("java/lang/Exception");                    \
    (Exception);                                                               \
  })

/**
 * Calls method on `NativeCore` instance in environment.
 */
#define CallNativeCoreMethodFromEnvironment(                                   \
  env, object, method, signature, ...                                          \
)                                                                              \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    auto _value = env->CallObjectMethod(object, _id, ##__VA_ARGS__);           \
    (_value);                                                                  \
  })

/**
 * Calls `callback(id, data)` method on `NativeCore` instance in environment.
 */
#define CallNativeCoreVoidMethodFromEnvironment(                               \
  env, object, method, signature, ...                                          \
)                                                                              \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    env->CallVoidMethod(object, _id, ##__VA_ARGS__);                           \
  })

/**
 * @TODO
 */
#define EvaluateJavaScriptInEnvironment(env, object, source)                   \
  CallNativeCoreVoidMethodFromEnvironment(                                     \
    env, object, "evaluateJavascript", "(Ljava/lang/String;)V", source        \
  );

/**
 * Generic `Exception` throw helper
 */
#define Throw(env, E)                                                          \
  ({                                                                           \
    env->ThrowNew(GetExceptionClass(env), E);                                  \
    (void) 0;                                                                  \
  });

/**
 * Translate a libuv error to a message suitable for `Throw(...)`
 */
#define UVException(code) uv_strerror(code)

/**
 * Errors thrown from the JNI/NDK bindings
 */
#define AssetManagerIsNotReachableException                                    \
  "AssetManager is not reachable through binding"
#define ExceptionCheckException "ExceptionCheck"
#define JavaScriptPreloadSourceNotInitializedException                         \
  "JavaScript preload source is not initialized"
#define NativeCoreJavaVMNotInitializedException                                \
  "NativeCore JavaVM is not initialized"
#define NativeCoreNotInitializedException "NativeCore is not initialized"
#define NativeCoreRefsNotInitializedException                                  \
  "NativeCore refs are not initialized"
#define RootDirectoryIsNotReachableException                                   \
  "Root directory in file system is not reachable through binding"
#define UVLoopNotInitializedException "UVLoop is not initialized"

/**
 * A container for a JNI string (jstring).
 */
class NativeString {
  JNIEnv *env;
  jstring ref;
  size_t length;
  const char *string;
  jboolean needsRelease;

  public:
  /**
   * `NativeString` class constructors.
   */
  NativeString (JNIEnv *env);
  NativeString (const NativeString &copy);
  NativeString (JNIEnv *env, jstring ref);
  NativeString (JNIEnv *env, std::string string);
  NativeString (JNIEnv *env, const char *string);
  ~NativeString ();

  /**
   * @TODO
   */
  const NativeString &
  operator= (const NativeString &string) {
    debug("COPY NativeString");
    *this = string;
    this->needsRelease = false;
    return *this;
  }

  /**
   * Various ways to set the internal value of a `NativeString` instance.
   */
  void Set (std::string string);
  void Set (const char *string);
  void Set (jstring ref);

  /**
   * Releases memory back to the JavaVM is needed. This is called
   * internally by the `NativeString` destructor.
   */
  void Release ();

  /**
   * Various ways to convert a `NativeString` to other string representations.
   */
  const char * c_str ();
  const std::string str ();
  const jstring jstr ();

  /**
   * Returns the computed size of internal string representation.
   */
  const size_t
  size ();
};

/**
 * A structured collection of global/local JNI references
 * for continued persistence in `NativeCore`.
 */
class NativeCoreRefs {
  JNIEnv *env;

  public:
  jobject core;
  jobject callbacks;

  NativeCoreRefs (JNIEnv *env) {
    this->env = env;
  }

  void Release ();
};

struct NativeFileSystemRequestContext {
  const NativeFileSystem *fs;
  NativeCoreSequence seq;
  NativeCallbackID callback;
  NativeCoreID id;
  NativeCore *core;
};

class NativeFileSystem {
  NativeCore *core;
  JNIEnv *env;

  public:
  NativeFileSystem (JNIEnv *env, NativeCore *core);

  NativeFileSystemRequestContext * CreateRequestContext (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  const std::string CreateJSONError (
    NativeCoreID id,
    const std::string message
  ) const;

  void CallbackAndFinalizeContext (
    NativeFileSystemRequestContext *context,
    std::string data
  ) const;

  void CallbackWithPostAndFinalizeContext (
    NativeFileSystemRequestContext *context,
    std::string data,
    SSC::Post post
  ) const;

  void Open (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string path,
    int flags,
    NativeCallbackID callback
  ) const;

  void Close (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void Read (
    NativeCoreSequence seq,
    NativeCoreID id,
    int len,
    int offset,
    NativeCallbackID callback
  ) const;

  void Write (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string data,
    int16_t offset
  ) const;

  void Stat (
    NativeCoreSequence seq,
    std::string path
  ) const;

  void Unlink (
    NativeCoreSequence seq,
    std::string path
  ) const;

  void Rename (
    NativeCoreSequence seq,
    std::string from,
    std::string to
  ) const;

  void CopyFile (
    NativeCoreSequence seq,
    std::string from,
    std::string to,
    int flags
  ) const;

  void RemoveDirectory (NativeCoreSequence seq, std::string path) const;

  void
  MakeDirectory (NativeCoreSequence seq, std::string path, int mode) const;

  void
  ReadDirectory (NativeCoreSequence seq, std::string path) const;
};

/**
 * An extended `SSC::Core` class for Android NDK/JNI
 * imeplementation.
 */
class NativeCore : public SSC::Core {
  // special floating variable that points to `refs.core`
  jobject self;

  // JNI
  JavaVM *jvm;
  JNIEnv *env;

  // Native
  NativeCoreRefs refs;
  NativeString rootDirectory;

  // webkti webview
  std::string javaScriptPreloadSource;
  SSC::WindowOptions windowOptions;

  // application
  AppConfig config;
  EnvironmentVariables environmentVariables;

  public:
  NativeCore (JNIEnv *env, jobject core);
  ~NativeCore ();

  jboolean ConfigureEnvironment ();
  jboolean ConfigureWebViewWindow ();

  void * GetPointer () const;

  JavaVM * GetJavaVM ();

  AppConfig & GetAppConfig ();

  const NativeString GetAppConfigValue (const char *key) const;
  const NativeString GetAppConfigValue (std::string key) const;
  const EnvironmentVariables & GetEnvironmentVariables () const;

  const NativeFileSystem GetNativeFileSystem () const;
  const NativeString & GetRootDirectory () const;
  const NativeCoreRefs & GetRefs () const;

  AAssetManager * GetAssetManager () const;

  NativeString GetPlatformType () const;
  NativeString GetPlatformOS () const;

  const char * GetJavaScriptPreloadSource () const;
};

#endif
