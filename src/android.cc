#include "android.hh"
#include "jni.h"
#include <stdio.h>

#define debug(format, ...)                                                     \
  {                                                                            \
    printf("SSC::Core::Debug:");                                               \
    printf(format, ##__VA_ARGS__);                                             \
    printf("\n");                                                              \
    fflush(stdout);                                                            \
  }

struct NativeRefs {
  jobject core;
};

class NativeCore : SSC::Core {
  JavaVM *vm;
  JNIEnv *env;
  NativeRefs refs;

  public:
  NativeCore(JNIEnv *env, jobject core) : Core() {
    this->env = env;
    this->env->GetJavaVM(&this->vm);
    this->refs.core = this->env->NewGlobalRef(core);
  }

  ~NativeCore () {
    this->env->DeleteGlobalRef(this->refs.core);
  }

  JavaVM *
  GetJavaVM () {
    return vm;
  }

  void * GetPointer () {
    return (void *) this;
  }
};

extern "C" {
  jlong package_export(Core_createPointer)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = new NativeCore(env, self);
    auto pointer = core->GetPointer();
    debug("Core::createPointer(%p)", pointer);
    return (jlong) core->GetPointer();
  }

  jboolean package_export(Core_verifyPointer)(
    JNIEnv *env,
    jobject self,
    jlong pointer
  ) {
    auto CoreClass = env->GetObjectClass(self);
    auto pointerField = env->GetFieldID(CoreClass, "pointer", "J");
    auto pointerValue = env->GetLongField(self, pointerField);

    if (pointerValue != pointer) {
      return JNI_FALSE;
    }

    return JNI_TRUE;
  }

  void package_export(Core_destroyPointer)(
    JNIEnv *env,
    jobject self,
    jlong pointer
  ) {
    if (pointer != 0) {
      debug("Core::destroyPointer(%p)", (void *) pointer);
      auto core = (NativeCore *) pointer;
      delete core;
    }
  }

  void package_export(Core_fsOpen)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    jstring path,
    int flags
  ) {
    // @TODO(jwerle): Core::fsOpen()
  }

  void package_export(Core_fsClose)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id
  ) {
    // @TODO(jwerle): Core::fsClose
  }

  void package_export(Core_fsRead)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    int len,
    int offset
  ) {
    // @TODO(jwerle): Core::fsRead
  }

  void package_export(Core_fsWrite)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    jstring data,
    int64_t offset
  ) {
    // @TODO(jwerle): Core::fsWrite
  }

  void package_export(Core_fsStat)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsStat
  }

  void package_export(Core_fsUnlink)(
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsUnlink
  }

  void package_export(Core_fsRename)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring pathA,
    jstring pathB
  ) {
    // @TODO(jwerle): Core::fsRename
  }

  void package_export(Core_fsCopyFile)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring pathA,
    jstring pathB,
    int flags
  ) {
    // @TODO(jwerle): Core::fsCopyFile
  }

  void package_export(Core_fsRmDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsRmDir
  }

  void package_export(Core_fsMkDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path,
    int mode
  ) {
    // @TODO(jwerle): Core::fsMkDir
  }

  void package_export(Core_fsReadDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsReadDir
  }
}
