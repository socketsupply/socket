// NDK/JNI
#include "android.hh"
#include "common.hh"

// libc
#include <stdio.h>
#include <unistd.h>

// c++
#include <any>
#include <map>
#include <string>

typedef std::map<std::string, std::string> AppConfig;
typedef std::map<std::string, std::string> EnvironmentVariables;

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
    this->Release();
  }

  void Release () {
    this->env->DeleteGlobalRef(this->core);
  }
};

/**
 * An extended `SSC::Core` class for Android NDK/JNI
 * imeplementation.
 */
class NativeCore : SSC::Core {
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
    windowOptions.preload = gPreload;
    windowOptions.title = this->config["title"];
    windowOptions.debug = DEBUG ? true : false;
    windowOptions.env = stream.str();

    this->javaScriptPreloadSource.assign(
      "window.addEventListener('unhandledrejection', e => console.log(e.message));\n"
      "window.addEventListener('error', e => console.log(e.reason));\n"
      "window.external = {\n"
      "  invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg)\n"
      "};\n"
      "console.error = console.warn = console.log;\n"
      "" + createPreload(windowOptions) + "\n"
      "//# sourceURL=preload.js"
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

    return AAssetManager_fromJava(this->env, ref);
  }

  const char * GetJavaScriptPreloadSource () {
    if (javaScriptPreloadSource.size() == 0) {
      return nullptr;
    }

    return javaScriptPreloadSource.c_str();
  }
};

/**
 * `NativeCore` JNI/NDK bindings.
 */
extern "C" {
  /**
   * `NativeCore::createPointer()` binding.
   * @return A pointer to a `NativeCore` instance
   */
  jlong exports(NativeCore, createPointer)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = new NativeCore(env, self);
    auto pointer = core->GetPointer();
    debug("Core::createPointer(%p)", pointer);
    return (jlong) core->GetPointer();
  }

  /**
   * `NativeCore::destroyPointer()` binding.
   * @param pointer Pointer to `NativeCore`
   */
  void exports(NativeCore, destroyPointer)(
    JNIEnv *env,
    jobject self,
    jlong pointer
  ) {
    if (pointer) {
      debug("Core::destroyPointer(%p)", (void *) pointer);
      delete (NativeCore *) pointer;
    }
  }

  /**
   * `NativeCore::verifyPointer()` binding.
   * @param pointer Pointer to verify
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyPointer)(
    JNIEnv *env,
    jobject self,
    jlong pointer
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!pointer || !core) {
      return false;
    }

    if ((void *) pointer != (void *) core) {
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyNativeExceptions()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  void exports(NativeCore, verifyNativeExceptions)(
    JNIEnv *env,
    jobject self
  ) {
    Throw(env, ExceptionCheckError);
  }

  /**
   * `NativeCore::verifyRootDirectory()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyRootDirectory)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedError);
      return false;
    }

    auto rootDirectory = core->GetRootDirectory();

    if (!rootDirectory.size()) {
      Throw(env, RootDirectoryIsNotReachableError);
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyAssetManager()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyAssetManager)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedError);
      return false;
    }

    auto assetManager = core->GetAssetManager();

    if (!assetManager) {
      Throw(env, AssetManagerIsNotReachableError);
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyPlatform()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyPlatform)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedError);
      return false;
    }

    if (core->GetPlatformOS().str() != "android") {
      return false;
    }

    if (core->GetPlatformType().str() != "linux") {
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyLoop()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyLoop)(
    JNIEnv *env,
    jobject self
  ) {
    if (!uv_default_loop()) {
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyFileSystem()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyFileSystem)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);
    jboolean ok = true;
    int err = 0;

    if (!core) {
      Throw(env, NativeCoreNotInitializedError);
      return false;
    }

    auto loop = uv_default_loop();

    struct { uv_fs_t opendir, readdir; } requests = {0};

    if (!loop) {
      Throw(env, UVLoopNotInitializedError);
      return false;
    }

    auto rootDirectory = core->GetRootDirectory();

    err = uv_fs_opendir(loop, &requests.opendir, rootDirectory.c_str(), nullptr);

    if (err != 0) {
      Throw(env, UVError(err));
      return false;
    }

    // TODO(jwerle):
    // - libuv filesystem readdir (check for files/ & cache/)
    // - libuv filesystem write OK to files/`VITAL_CHECK_FILE`
    // - libuv filesystem read OK from files/`VITAL_CHECK_FILE`
    // - AssetManager `VITAL_CHECK_FILE` check

    err = uv_run(loop, UV_RUN_DEFAULT);

    if (err != 0) {
      Throw(env, UVError(err));
      return false;
    }

    return ok;
  }

  /**
   * `NativeCore::configureEnvironment()` binding.
   * @return `true` if configuration was succcessful, otherwise `false`.
   */
  jboolean exports(NativeCore, configureEnvironment)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedError);
      return false;
    }

    return core->ConfigureEnvironment();
  }

  /**
   * `NativeCore::configureWebViewWindow()` binding.
   * @return `true` if configuration was succcessful, otherwise `false`.
   */
  jboolean exports(NativeCore, configureWebViewWindow)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedError);
      return false;
    }

    return core->ConfigureWebViewWindow();
  }

  /**
   * `NativeCore::getJavaScriptPreloadSource()` binding.
   * @return JavaScript preload source code injected into WebView.
   */
  jstring exports(NativeCore, getJavaScriptPreloadSource)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedError);
      return env->NewStringUTF("");
    }

    auto preload = core->GetJavaScriptPreloadSource();

    if (!preload) {
      Throw(env, JavaScriptPreloadSourceNotInitializedError);
      return env->NewStringUTF("");
    }

    return env->NewStringUTF(preload);
  }

  void exports(NativeCore, fsOpen)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    jstring path,
    int flags
  ) {
    // @TODO(jwerle): Core::fsOpen()
  }

  void exports(NativeCore, fsClose)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id
  ) {
    // @TODO(jwerle): Core::fsClose
  }

  void exports(NativeCore, fsRead)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    int len,
    int offset
  ) {
    // @TODO(jwerle): Core::fsRead
  }

  void exports(NativeCore, fsWrite)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    jstring data,
    int64_t offset
  ) {
    // @TODO(jwerle): Core::fsWrite
  }

  void exports(NativeCore, fsStat)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsStat
  }

  void exports(NativeCore, fsUnlink)(
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsUnlink
  }

  void exports(NativeCore, fsRename)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring pathA,
    jstring pathB
  ) {
    // @TODO(jwerle): Core::fsRename
  }

  void exports(NativeCore, fsCopyFile)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring pathA,
    jstring pathB,
    int flags
  ) {
    // @TODO(jwerle): Core::fsCopyFile
  }

  void exports(NativeCore, fsRmDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsRmDir
  }

  void exports(NativeCore, fsMkDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path,
    int mode
  ) {
    // @TODO(jwerle): Core::fsMkDir
  }

  void exports(NativeCore, fsReadDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsReadDir
  }
}
