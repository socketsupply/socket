#include "android.hh"
#include <stdio.h>

#define debug(format, ...)                                                     \
  {                                                                            \
    printf("SSC::Core::Debug:");                                               \
    printf(format, ##__VA_ARGS__);                                             \
    printf("\n");                                                              \
    fflush(stdout);                                                            \
  }

extern "C" {
  jlong package_export(Core_createPointer)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = new SSC::Core();
    debug("Core::createPointer(%p)", (void *) core);
    return (jlong) (void *) core;
  }

  void package_export(Core_destroyPointer)(
    JNIEnv *env,
    jobject self,
    jlong pointer
  ) {
    if (pointer != 0) {
      debug("Core::destroyPointer(%p)", (void *) pointer);
      auto core = (SSC::Core *) pointer;
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
