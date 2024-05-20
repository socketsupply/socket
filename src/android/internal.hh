#ifndef SOCKET_RUNTIME_ANDROID_INTERNAL_H
#define SOCKET_RUNTIME_ANDROID_INTERNAL_H

#include "../core/core.hh"
#include "../ipc/ipc.hh"
#include "../window/options.hh"

/**
 * Defined by the Socket preprocessor
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
      bool isEmulator = false;

      Runtime (JNIEnv* env, jobject self, String rootDirectory);
      ~Runtime ();
      bool isPermissionAllowed (const String&) const;
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
