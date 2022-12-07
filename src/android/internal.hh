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

#define CallVoidClassMethodFromEnvironment(env, object, method, sig, ...)      \
  ({                                                                           \
    auto Class = env->GetObjectClass(object);                                  \
    auto ID = env->GetMethodID(Class, method, sig);                            \
    env->CallVoidMethod(object, ID, ##__VA_ARGS__);                            \
  })

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
  struct JVMEnvironment {
    JavaVM* jvm = nullptr;
    int jniVersion = 0;

    JVMEnvironment (JNIEnv* env) {
      this->jniVersion = env->GetVersion();
      env->GetJavaVM(&jvm);
    }

    int version () {
      return this->jniVersion;
    }

    JavaVM* get () {
      return this->jvm;
    }
  };

  struct JNIEnvironmentAttachment {
    JNIEnv *env = nullptr;
    JavaVM *jvm = nullptr;
    int status = 0;
    int version = 0;
    bool attached = false;

    JNIEnvironmentAttachment () = default;
    JNIEnvironmentAttachment (JavaVM *jvm, int version) {
      this->attach(jvm, version);
    }

    ~JNIEnvironmentAttachment () {
      this->detach();
    }

    void attach (JavaVM *jvm, int version) {
      this->jvm = jvm;
      this->version = version;

      if (jvm != nullptr) {
        this->status = this->jvm->GetEnv((void **) &this->env, this->version);

        if (this->status == JNI_EDETACHED) {
          this->attached = this->jvm->AttachCurrentThread(&this->env, 0);
        }
      }
    }

    void detach () {
      auto jvm = this->jvm;
      auto attached = this->attached;

      if (this->hasException()) {
        this->printException();
      }

      this->env = nullptr;
      this->jvm = nullptr;
      this->status = 0;
      this->attached = false;

      if (attached && jvm != nullptr) {
        jvm->DetachCurrentThread();
      }
    }

    inline bool hasException () {
      return this->env != nullptr && this->env->ExceptionCheck();
    }

    inline void printException () {
      if (this->env != nullptr) {
        this->env->ExceptionDescribe();
      }
    }
  };

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
      StringWrap (JNIEnv *env, String string);
      StringWrap (JNIEnv *env, const char *string);
      ~StringWrap ();

      void set (String string);
      void set (const char *string);
      void set (jstring ref);
      void release ();

      const String str ();
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

  class Runtime : public Core {
    public:
      static auto from (JNIEnv* env, jobject self) {
        auto pointer = GetObjectClassFieldFromEnvironment(env, self, Long, "pointer", "J");
        return reinterpret_cast<Runtime*>(pointer);
      }

      static auto from (jlong pointer) {
        return reinterpret_cast<Runtime*>(pointer);
      }

      JNIEnv *env = nullptr;
      jobject self = nullptr;
      jlong pointer = 0;
      String rootDirectory = "";

      Runtime (JNIEnv* env, jobject self, String rootDirectory);
      ~Runtime ();
  };

  class Bridge : public IPC::Bridge {
    public:
      static auto from (JNIEnv* env, jobject self) {
        auto pointer = GetObjectClassFieldFromEnvironment(env, self, Long, "pointer", "J");
        return reinterpret_cast<Bridge*>(pointer);
      }

      static auto from (jlong pointer) {
        return reinterpret_cast<Bridge*>(pointer);
      }

      JNIEnv *env = nullptr;
      jobject self = nullptr;
      jlong pointer = 0;
      Runtime* runtime = nullptr;

      Bridge (JNIEnv* env, jobject self, Runtime* runtime);
      ~Bridge ();
  };

  class Window {
    public:
      static auto from (JNIEnv* env, jobject self) {
        auto pointer = GetObjectClassFieldFromEnvironment(env, self, Long, "pointer", "J");
        return reinterpret_cast<Window*>(pointer);
      }

      static auto from (jlong pointer) {
        return reinterpret_cast<Window*>(pointer);
      }

      JNIEnv* env = nullptr;
      jobject self = nullptr;
      jlong pointer = 0;
      Bridge* bridge = nullptr;
      Map config;
      String preloadSource;
      WindowOptions options;
      Map envvars;

      Window (
        JNIEnv* env,
        jobject self,
        Bridge* bridge,
        WindowOptions options
      );

      ~Window ();

    void evaluateJavaScript (String source, JVMEnvironment& jvm);
  };
}

#endif
