#include "android.hh"
#include "jni.h"

#include <stdio.h>
#include <unistd.h>

/**
 * A structured collection of global/local JNI references
 * for continued persistence in `NativeCore`.
 */
struct NativeCoreRefs {
  jobject core;
  jobject callbacks;
};

/**
 * An extended `SSC::Core` class for Android NDK/JNI
 * imeplementation.
 */
class NativeCore : SSC::Core {
  JavaVM *jvm;
  JNIEnv *env;
  NativeCoreRefs refs;

  class NativeString {
    JNIEnv *env;
    jstring ref;
    const char *string;

    public:
    NativeString (JNIEnv *env, jstring ref) {
      this->env = env;
      this->ref = ref;
      this->string = env->GetStringUTFChars(ref, 0);
      debug("NativeString constructor called");
    }

    void Release () {
      if (this->ref && this->string) {
        this->env->ReleaseStringUTFChars(this->ref, this->string);
        this->ref = 0;
        this->string = 0;
      }
    }

    ~NativeString () {
      debug("NativeString deconstructor called");
      this->Release();
    }

    const char * c_str () {
      return string;
    }

    size_t size () {
      if (!this->string) {
        return 0;
      }

      return this->env->GetStringUTFLength(this->ref);
    }
  };

  public:
  NativeCore(JNIEnv *env, jobject core) : Core() {
    this->env = env;
    this->env->GetJavaVM(&this->jvm);
    this->refs.core = this->env->NewGlobalRef(core);
  }

  ~NativeCore () {
    this->env->DeleteGlobalRef(this->refs.core);
  }

  JavaVM * GetJavaVM () {
    return this->jvm;
  }

  const NativeString GetRootDirectory () {
    // `NativeCore::getRootDirectory()`
    return NativeString(this->env, (jstring) CallNativeCoreMethodFromEnvironment(
      this->env,
      this->refs.core,
      "getRootDirectory",
      "()Ljava/lang/String;"
    ));
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

  void * GetPointer () {
    return (void *) this;
  }
};

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
   * `NativeCore::verifyNativeExceptions() binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  void exports(NativeCore, verifyNativeExceptions)(
    JNIEnv *env,
    jobject self
  ) {
    auto Exception = GetExceptionClass(env);
    Throw(ExceptionCheck(Exception));
  }

  /**
   * `NativeCore::verifyRootDirectory() binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyRootDirectory)(
    JNIEnv *env,
    jobject self
  ) {
    auto Exception = GetExceptionClass(env);
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(NativeCoreNotInitialized(Exception));
      return false;
    }

    auto rootDirectory = core->GetRootDirectory();

    if (!rootDirectory.size()) {
      Throw(RootDirectoryIsNotReachable(Exception));
      return false;
    }

    debug("rootDirectoryString: %s", rootDirectory.c_str());

    return true;
  }

  /**
   * `NativeCore::verifyAssetManager() binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyAssetManager)(
    JNIEnv *env,
    jobject self
  ) {
    auto Exception = GetExceptionClass(env);
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(NativeCoreNotInitialized(Exception));
      return false;
    }

    auto assetManager = core->GetAssetManager();
    if (!assetManager) {
      Throw(AssetManagerIsNotReachable(Exception));
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyLoop() binding.
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
   * `NativeCore::verifyFileSystem() binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyFileSystem)(
    JNIEnv *env,
    jobject self
  ) {
    auto Exception = GetExceptionClass(env);
    auto core = GetNativeCoreFromEnvironment(env);
    jboolean ok = true;
    int err = 0;

    if (!core) {
      Throw(NativeCoreNotInitialized(Exception));
      return false;
    }

    auto loop = uv_default_loop();

    if (!loop) {
      Throw(UVLoopNotInitialized(Exception));
      return false;
    }

    auto rootDirectory = core->GetRootDirectory();

    // TODO(jwerle):
    // - libuv filesystem readdir (check for files/ & cache/)
    // - libuv filesystem write OK to files/`VITAL_CHECK_FILE`
    // - libuv filesystem read OK from files/`VITAL_CHECK_FILE`
    // - AssetManager `VITAL_CHECK_FILE` check

    err = uv_run(loop, UV_RUN_DEFAULT);

    if (err != 0) {
      env->ThrowNew(Exception, uv_strerror(err));
      return false;
    }

    return ok;
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
