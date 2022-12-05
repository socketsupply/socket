#ifndef SSC_ANDROID_NATIVE_H
#define SSC_ANDROID_NATIVE_H

#include "../core/core.hh"
#include "../core/runtime-preload.hh"
#include "../window/options.hh"

/**
 * Defined by the Socket SDK preprocessor
 */
#define PACKAGE_NAME __BUNDLE_IDENTIFIER__

/**
 * Creates a named native package export for the configured bundle suitable for
 * for definition only.
 * @param name The name of the package function to export
 */
#define external(namespace, name)                                              \
  JNIEXPORT JNICALL Java___BUNDLE_IDENTIFIER___##namespace##_##name

/**
 * Gets class for object for `self` from `env`.
 */
#define GetObjectClass(env) env->GetObjectClass(self)

/**
 * Get field on object `self` from `env`.
 */
#define GetObjectClassField(env, Type, field, sig)                             \
  ({                                                                           \
    auto Class = GetObjectClass(env);                                          \
    auto id = env->GetFieldID(Class, field, sig);                              \
    env->Get##Type##Field(self, id);                                           \
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
 */
#define CallObjectClassMethod(env, object, method, signature, ...)             \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    auto _value = env->CallObjectMethod(object, _id, ##__VA_ARGS__);           \
    (_value);                                                                  \
  })

/**
 * Calls `callback(id, data)` method on `Core` instance in environment.
 */
#define CallCoreVoidMethodFromEnvironment(env, object, method, signature, ...) \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    env->CallVoidMethod(object, _id, ##__VA_ARGS__);                           \
  })


/**
 * Converts a `jstring` to an ID type
 */
#define GetCoreIDFromJString(env, string)                                      \
  (CoreID) std::stoull(NativeString(env, string).str())

/**
 * @TODO
 */
#define EvaluateJavaScriptInEnvironment(env, object, source)                   \
  CallCoreVoidMethodFromEnvironment(                                     \
    env, object, "evaluateJavascript", "(Ljava/lang/String;)V", source         \
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
#define BridgeNotInitializedException "Bridge is not initialized"
#define CoreJavaVMNotInitializedException "Core JavaVM is not initialized"
#define CoreNotInitializedException "Core is not initialized"
#define CoreRefsNotInitializedException "Core refs are not initialized"
#define RuntimeNotInitializedException "Runtime is not initialized"
#define RootDirectoryIsNotReachableException                                   \
  "Root directory in file system is not reachable through binding"
#define UVLoopNotInitializedException "UVLoop is not initialized"
#define WindowNotInitializedException "Window is not initialized"

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
   NativeString (JNIEnv *env, SSC::String string);
   NativeString (JNIEnv *env, const char *string);
    ~NativeString ();

    /**
     * @TODO
     */
    const NativeString &
    operator= (const NativeString &string) {
      *this = string;
      this->needsRelease = false;
      return *this;
    }

    /**
     * Various ways to set the internal value of a `NativeString` instance.
     */
    void set (SSC::String string);
    void set (const char *string);
    void set (jstring ref);

    /**
     * Releases memory back to the JavaVM is needed. This is called
     * internally by the `NativeString` destructor.
     */
    void release ();

    /**
     * Various ways to convert a `NativeString` to other string representations.
     */
    const char * c_str ();
    const SSC::String str ();
    const jstring jstr ();

    /**
     * Returns the computed size of internal string representation.
     */
    const size_t size ();
};

#endif
