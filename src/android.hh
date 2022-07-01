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

// libc
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

// c++
#include <any>
#include <map>
#include <string>

// android
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

// SSC
#include "common.hh"
#include "core.hh"

typedef std::map<std::string, std::string> AppConfig;
typedef std::map<std::string, std::string> EnvironmentVariables;

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
 * TODO
 */
#define CallNativeCoreCallbackMethodFromEnvironment(                           \
  env, object, method, signature, ...                                          \
)                                                                              \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    env->CallVoidMethod(object, _id, ##__VA_ARGS__);                           \
  })

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

#define AssetManagerIsNotReachableException "AssetManager is not reachable through binding"
#define ExceptionCheckException "ExceptionCheck"
#define JavaScriptPreloadSourceNotInitializedException "JavaScript preload source is not initialized"
#define NativeCoreJavaVMNotInitializedException "NativeCore JavaVM is not initialized"
#define NativeCoreNotInitializedException "NativeCore is not initialized"
#define NativeCoreRefsNotInitializedException "NativeCore refs are not initialized"
#define RootDirectoryIsNotReachableException "Root directory in file system is not reachable through binding"
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
  NativeString (JNIEnv *env) {
    this->env = env;
    this->ref = 0;
    this->string = 0;
    this->needsRelease = false;

    // debug("NativeString constructor called");
  }

  NativeString (const NativeString &copy) :
    env { copy.env },
    ref { copy.ref },
    length { copy.length },
    string { copy.string },
    needsRelease { false }
  {
    // noop copy constructor
  }

  NativeString (JNIEnv *env, jstring ref) : NativeString(env) {
    if (ref) {
      this->Set(ref);
    }
  }

  NativeString (JNIEnv *env, std::string string) : NativeString(env) {
    if (string.size() > 0) {
      this->Set(string);
    }
  }

  NativeString (JNIEnv *env, const char *string) : NativeString(env) {
    if (string) {
      this->Set(string);
    }
  }

  ~NativeString () {
    // debug("NativeString deconstructor called");
    this->Release();
  }

  NativeString & operator= (const NativeString &string) {
    debug("COPY NativeString");
    *this = string;
    this->needsRelease = false;
    return *this;
  }

  void Set (std::string string) {
    this->Set(string.c_str());
  }

  void Set (const char *string) {
    this->Set(this->env->NewStringUTF(string));
  }

  void Set (jstring ref) {
    if (ref) {
      this->ref = ref;
      this->string = this->env->GetStringUTFChars(ref, &this->needsRelease);
      this->length = this->env->GetStringUTFLength(this->ref);
    }
  }

  void Release () {
    if (this->ref && this->string && this->needsRelease) {
      this->env->ReleaseStringUTFChars(this->ref, this->string);
    }

    this->ref = 0;
    this->length = 0;
    this->string = 0;
    this->needsRelease = false;
  }

  const char * c_str () {
    return this->string;
  }

  const std::string str () {
    std::string value;

    if (this->string) {
      value.assign(this->string);
    }

    return value;
  }

  size_t size () {
    if (!this->string || !this->ref) {
      return 0;
    }

    return this->length;
  }
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

  ~NativeCoreRefs () {
    // noop: `NativeCore` will called `Release()` manually
  }

  void Release () {
    this->env->DeleteGlobalRef(this->core);
  }
};

// Forward declaration
typedef jlong NativeCallbackID;
typedef uint64_t NativeCoreID;

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
  NativeCore (JNIEnv *env, jobject core) : Core(),
    refs(env),
    config(),
    rootDirectory(env),
    environmentVariables(),
    javaScriptPreloadSource("")
  {
    this->env = env;
    this->env->GetJavaVM(&this->jvm);
    this->refs.core = this->env->NewGlobalRef(core);
    this->self = this->refs.core;
  }

  ~NativeCore () {
    this->rootDirectory.Release();
    this->refs.Release();

    this->env = 0;
    this->jvm = 0;
  }

  jboolean ConfigureEnvironment () {
    using SSC::createPreload;
    using SSC::decodeURIComponent;
    using SSC::encodeURIComponent;
    using SSC::getEnv;
    using SSC::parseConfig;
    using SSC::split;
    using SSC::trim;

    std::stringstream stream;

    // `NativeCore::getRootDirectory()`
    this->rootDirectory.Set((jstring) CallNativeCoreMethodFromEnvironment(
      this->env,
      this->refs.core,
      "getRootDirectory",
      "()Ljava/lang/String;"
    ));

    if (this->rootDirectory.size() == 0) {
      return false;
    }

    this->config = parseConfig(decodeURIComponent(STR_VALUE(SETTINGS)));

    for (auto const &tuple : this->config) {
      auto key = tuple.first;
      auto value = tuple.second;

      debug("AppConfig: %s = %s", key.c_str(), value.c_str());
    }

    for (auto const &var : split(this->config["env"], ',')) {
      auto key = trim(var);
      auto value = getEnv(key.c_str());

      if (value.size() > 0) {
        stream << key << "=" << encodeURIComponent(value) << "&";
        environmentVariables[key] = value;
        debug("EnvironmentVariable: %s=%s", key.c_str(), value.c_str());
      }
    }

    windowOptions.executable = this->config["executable"];
    windowOptions.version = this->config["version"];
    windowOptions.preload = gPreloadMobile;
    windowOptions.title = this->config["title"];
    windowOptions.debug = DEBUG ? true : false;
    windowOptions.env = stream.str();

    this->javaScriptPreloadSource.assign(
      "console.error = console.warn = console.log;          \n"
      "                                                     \n"
      "window.addEventListener('unhandledrejection', e => { \n"
      "  console.log(e.reason || e.message || e);           \n"
      "});                                                  \n"
      "                                                     \n"
      "window.addEventListener('error', e => {              \n"
      "  const message = e.reason || e.message || e;        \n"
      "  if (!/debug-evaluate/.test(message)) {             \n"
      "    console.log(message);                            \n"
      "  }                                                  \n"
      "});                                                  \n"
      "                                                     \n"
      "" + createPreload(windowOptions) + "                 \n"
      "//# sourceURL=preload.js                             \n"
    );

    stream.str(""); // clear stream

    if (windowOptions.debug) {
      auto Class = GetNativeCoreBindingClass(this->env);
      auto isDebugEnabledField = this->env->GetFieldID(Class, "isDebugEnabled", "Z");
      this->env->SetBooleanField(self, isDebugEnabledField, true);
    }

    return true;
  }

  jboolean ConfigureWebViewWindow () {
    return true;
  }

  void * GetPointer () {
    return (void *) this;
  }

  JavaVM * GetJavaVM () {
    return this->jvm;
  }

  AppConfig & GetAppConfig () {
    return this->config;
  }

  const NativeCoreRefs & GetRefs () {
    return this->refs;
  }

  const EnvironmentVariables & GetEnvironmentVariables () {
    return this->environmentVariables;
  }

  const NativeString & GetRootDirectory () {
    return this->rootDirectory;
  }

  NativeString GetPlatformType () {
    return NativeString(this->env, "linux");
  }

  NativeString GetPlatformOS () {
    return NativeString(this->env, "android");
  }

  AAssetManager * GetAssetManager () {
    // `NativeCore::getAssetManager()`
    auto ref = CallNativeCoreMethodFromEnvironment(
      this->env,
      this->refs.core,
      "getAssetManager",
      "()Landroid/content/res/AssetManager;"
    );

    if (ref) {
      return AAssetManager_fromJava(this->env, ref);
    }

    return nullptr;
  }

  const char * GetJavaScriptPreloadSource () {
    if (javaScriptPreloadSource.size() == 0) {
      return nullptr;
    }

    return javaScriptPreloadSource.c_str();
  }
};

class NativeFileSystem {
  NativeCore *core;
  JNIEnv *env;

 struct NativeFileSystemRequestContext {
   SSC::DescriptorContext *descriptor;
   NativeFileSystem *fs;
   NativeCallbackID callback;
   NativeCoreID id;
   NativeCore *core;
   uv_fs_t request;
 };

  public:
  NativeFileSystem (JNIEnv *env, NativeCore *core) {
    this->env = env;
    this->core = core;
  }

  NativeFileSystemRequestContext * CreateRequestContext (
    std::string seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) {
    auto context = new NativeFileSystemRequestContext();

    context->descriptor = new SSC::DescriptorContext();
    context->callback = callback;
    context->core = this->core;
    context->fs = this;
    context->id = id;

    context->descriptor->seq = seq;
    context->descriptor->id = id;

    context->request.data = context;

    SSC::descriptors[id] = context->descriptor;
    return context;
  }

  const std::string CreateJSONError (
    NativeCoreID id,
    const std::string message
  ) {
    return SSC::format(R"MSG({
      "value": {
        "err": {
          "id": "$S",
          "message": "$S"
        }
      }
    })MSG", std::to_string(id), message);
  }

  void CallbackAndFinalizeContext (
    NativeFileSystemRequestContext *context,
    std::string data
  ) {
    auto refs = context->core->GetRefs();
    auto jvm = context->core->GetJavaVM();
    JNIEnv *env = 0;

    jvm->AttachCurrentThread(&env, 0);

    CallNativeCoreCallbackMethodFromEnvironment(
      env,
      refs.core,
      "callback",
      "(JLjava/lang/String;)V",
      context->callback,
      env->NewStringUTF(data.c_str())
    );

    delete context;

    //jvm->DetachCurrentThread();
  }

  void Open (
    std::string seq,
    NativeCoreID id,
    std::string path,
    int flags,
    NativeCallbackID callback
  ) {
    using SSC::format;

    auto context = this->CreateRequestContext(seq, id, callback);
    auto loop = uv_default_loop();

    int mode = S_IRUSR | S_IWUSR;
    int err = 0;

    err = uv_fs_open(loop, &context->request, path.c_str(), flags, mode, [](uv_fs_t *req) {
      auto context = (NativeFileSystemRequestContext *) req->data;
      auto id = context->id;
      auto fd = req->result;

      std::string data;

      if (fd < 0) {
        auto err = std::string(uv_strerror(fd));
        data = context->fs->CreateJSONError(id, err);
      } else {
        data = format(
          R"MSG({"value":{"data":{"id": "$S", "fd": $S}}})MSG",
          std::to_string(id),
          std::to_string(fd)
        );
      }

      context->fs->CallbackAndFinalizeContext(context, data);
      uv_fs_req_cleanup(req);
    });

    if (err < 0) {
      delete context;
      return Throw(this->env, UVException(err));
    }

    err = uv_run(loop, UV_RUN_DEFAULT);

    if (err < 0) {
      delete context;
      return Throw(this->env, UVException(err));
    }
  }

  void
  Close (std::string seq, uint64_t id) {
    // reinterpret_cast<SSC::Core *>(this->core)->fsClose(seq, id, [&](auto seq,
    // auto msg, auto post) { });
  }

  void
  Read (std::string seq, uint64_t id, int len, int offset) {
  }

  void
  Write (std::string seq, uint64_t id, std::string data, int16_t offset) {
  }

  void
  Stat (std::string seq, std::string path) {
  }

  void
  Unlink (std::string seq, std::string path) {
  }

  void
  Rename (std::string seq, std::string from, std::string to) {
  }

  void
  CopyFile (std::string seq, std::string from, std::string to, int flags) {
  }

  void
  RemoveDirectory (std::string seq, std::string path) {
  }

  void
  MakeDirectory (std::string seq, std::string path, int mode) {
  }

  void
  ReadDirectory (std::string seq, std::string path) {
  }
};

#endif
