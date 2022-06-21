#include "android.hh"

extern "C" {
  void package_export(Core_initialize)(
    JNIEnv *env,
    jobject self,
  ) {
    auto CoreClass env->GetObjectClass(self);
    auto field = env->GetFieldID(CoreClass, "pointer", "J"); // "J" means `Long`
    auto core = new Core();
                                                             //
    env->SetLongField(self, field, (jlong) (void *) core);
  }

  void package_export(Core_destroy)(
    JNIEnv *env,
    jobject self,
  ) {
    auto CoreClass env->GetObjectClass(self);
    auto field = env->GetFieldID(CoreClass, "pointer", "J"); // "J" means `Long`
    auto core (Core *) env->GetLongField(self, field);

    delete core;
    env->SetLongField(self, field, (jlong) 0);
  }

  void package_export(core_fsOpen)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    jstring path,
    int flags
  ) {
    // @TODO(jwerle): Core::fsOpen()
  }

  void package_export(core_fsClose)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id
  ) {
    // @TODO(jwerle): Core::fsClose
  }

  void package_export(core_fsRead)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    int len,
    int offset
  ) {
    // @TODO(jwerle): Core::fsRead
  }

  void package_export(core_fsWrite)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    uint64_t id,
    jstring data,
    int64_t offset
  ) {
    // @TODO(jwerle): Core::fsWrite
  }

  void package_export(core_fsStat)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsStat
  }

  void package_export(core_fsUnlink)(
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsUnlink
  }

  void package_export(core_fsRename)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring pathA,
    jstring pathB
  ) {
    // @TODO(jwerle): Core::fsRename
  }

  void package_export(core_fsCopyFile)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring pathA,
    jstring pathB,
    int flags
  ) {
    // @TODO(jwerle): Core::fsCopyFile
  }

  void package_export(core_fsRmDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsRmDir
  }

  void package_export(core_fsMkDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path,
    int mode
  ) {
    // @TODO(jwerle): Core::fsMkDir
  }

  void package_export(core_fsReadDir)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path
  ) {
    // @TODO(jwerle): Core::fsReadDir
  }
}
