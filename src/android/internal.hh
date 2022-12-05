#ifndef SSC_ANDROID_INTERNAL_H
#define SSC_ANDROID_INTERNAL_H

#include "../core/core.hh"
#include "../ipc/ipc.hh"
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
#define GetObjectClassFromEnvironment(env, self) env->GetObjectClass(self)

/**
 * Get field on object `self` from `env`.
 */
#define GetObjectClassFieldFromEnvironment(env, self, Type, field, sig)        \
  ({                                                                           \
    auto Class = GetObjectClassFromEnvironment(env, self);                     \
    auto id = env->GetFieldID(Class, field, sig);                              \
    env->Get##Type##Field(self, id);                                           \
  })

/**
 * Gets the JNI `Exception` class from environment.
 */
#define GetExceptionClassFromEnvironment(env)                                  \
  env->FindClass("java/lang/Exception")

/**
 */
#define CallObjectClassMethodFromEnvironment(env, object, method, sig, ...)    \
  ({                                                                           \
    auto Class = env->GetObjectClass(object);                                  \
    auto ID = env->GetMethodID(Class, method, sig);                            \
    env->CallObjectMethod(object, ID, ##__VA_ARGS__);                          \
  })

/**
 * Calls `callback(id, data)` method on `Core` instance in environment.
 */
#define CallVoidMethodFromEnvironment(env, object, method, signature, ...)     \
  ({                                                                           \
    auto Class = env->GetObjectClass(object);                                  \
    auto ID = env->GetMethodID(Class, method, signature);                      \
    env->CallVoidMethod(object, ID, ##__VA_ARGS__);                            \
  })

/**
 * Converts a `jstring` to an ID type
 */
#define StringToRuntimeID(env, string)                                         \
  std::stoull(StringWrap(env, string).str())

/**
 * Generic `Exception` throw helper
 */
#define Throw(env, E)                                                          \
  ({                                                                           \
    env->ThrowNew(GetExceptionClassFromEnvironment(env), E);                   \
    (void) 0;                                                                  \
  })

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

namespace SSC::android {

  /**
   * A container for a JNI string (jstring).
   */
  class StringWrap {
    JNIEnv *env = nullptr;
    jstring ref = nullptr;
    const char *string = nullptr;
    size_t length = 0;
    jboolean needsRelease = false;

    public:
      StringWrap (JNIEnv *env);
      StringWrap (const StringWrap &copy);
      StringWrap (JNIEnv *env, jstring ref);
      StringWrap (JNIEnv *env, SSC::String string);
      StringWrap (JNIEnv *env, const char *string);
      ~StringWrap ();

      void set (SSC::String string);
      void set (const char *string);
      void set (jstring ref);
      void release ();

      const SSC::String str ();
      const jstring j_str ();
      const char * c_str ();
      const size_t size ();

      const StringWrap &
      operator= (const StringWrap &string) {
        *this = string;
        this->needsRelease = false;
        return *this;
      }
  };

  class Bridge : public SSC::IPC::Bridge {
    public:
      static auto from (JNIEnv* env, jobject self) {
        auto pointer = GetObjectClassFieldFromEnvironment(env, self, Long, "pointer", "J");
        return reinterpret_cast<Bridge*>(pointer);
      }

      JNIEnv *env = nullptr;
      jobject self = nullptr;

      Bridge (JNIEnv* env, jobject self, SSC::Core* core)
        : SSC::IPC::Bridge(core)
      {
        this->env = env;
        this->self = env->NewGlobalRef(self);
      }

      ~Bridge () {
        this->env->DeleteGlobalRef(this->self);
        SSC::IPC::Bridge::~Bridge();
      }
  };


  class Runtime : public SSC::Core {
    public:
      static auto from (JNIEnv* env, jobject self) {
        auto pointer = GetObjectClassFieldFromEnvironment(env, self, Long, "pointer", "J");
        return reinterpret_cast<Runtime*>(pointer);
      }

      JNIEnv *env = nullptr;
      jobject self = nullptr;

      Runtime (JNIEnv* env, jobject self) : SSC::Core() {
        this->env = env;
        this->self = env->NewGlobalRef(self);
      }

      ~Runtime () {
        this->env->DeleteGlobalRef(this->self);
      }
  };

  class Window {
    public:
      static auto from (JNIEnv* env, jobject self) {
        auto pointer = GetObjectClassFieldFromEnvironment(env, self, Long, "pointer", "J");
        return reinterpret_cast<Window*>(pointer);
      }

      JNIEnv* env = nullptr;
      jobject self = nullptr;
      SSC::IPC::Bridge* bridge = nullptr;
      SSC::Map config;
      SSC::String preloadSource;
      SSC::WindowOptions options;
      SSC::Map envvars;

      Window (
        JNIEnv* env,
        jobject self,
        SSC::IPC::Bridge* bridge,
        SSC::WindowOptions options
      );

      ~Window () {
        this->env->DeleteGlobalRef(this->self);
      }
  };
}

#endif
