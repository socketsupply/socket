#include "android.hh"

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
    Throw(env, ExceptionCheckException);
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
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    auto rootDirectory = core->GetRootDirectory();

    if (!rootDirectory.size()) {
      Throw(env, RootDirectoryIsNotReachableException);
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
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    auto assetManager = core->GetAssetManager();

    if (!assetManager) {
      Throw(env, AssetManagerIsNotReachableException);
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
      Throw(env, NativeCoreNotInitializedException);
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
   * `NativeCore::verifyEnvironment()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyEnvironment)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    // @TODO
    return true;
  }

  /**
   * `NativeCore::verifyJavaVM()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyJavaVM)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    if (!core->GetJavaVM()) {
      Throw(env, NativeCoreJavaVMNotInitializedException);
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyRefs()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyRefs)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    auto refs = core->GetRefs();

    if (!refs.core) {
      Throw(env, NativeCoreRefsNotInitializedException);
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
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    auto loop = uv_default_loop();

    struct { uv_fs_t opendir, readdir; } requests = {0};

    if (!loop) {
      Throw(env, UVLoopNotInitializedException);
      return false;
    }

    auto rootDirectory = core->GetRootDirectory();

    err = uv_fs_opendir(loop, &requests.opendir, rootDirectory.c_str(), nullptr);

    if (err != 0) {
      Throw(env, UVException(err));
      return false;
    }

    // TODO(jwerle):
    // - libuv filesystem readdir (check for files/ & cache/)
    // - libuv filesystem write OK to files/`VITAL_CHECK_FILE`
    // - libuv filesystem read OK from files/`VITAL_CHECK_FILE`
    // - AssetManager `VITAL_CHECK_FILE` check

    err = uv_run(loop, UV_RUN_DEFAULT);

    if (err != 0) {
      Throw(env, UVException(err));
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
      Throw(env, NativeCoreNotInitializedException);
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
      Throw(env, NativeCoreNotInitializedException);
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
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto preload = core->GetJavaScriptPreloadSource();

    if (!preload) {
      Throw(env, JavaScriptPreloadSourceNotInitializedException);
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
