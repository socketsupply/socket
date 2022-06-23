#include "android.hh"
#include "jni.h"
#include <stdio.h>

constexpr auto VITAL_CHECK_FILE = "file:///android_asset/vital_check_ok.txt";

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

  jboolean package_export(Core_verifyPointer)(
    JNIEnv *env,
    jobject self,
    jlong pointer
  ) {
    auto CoreClass = env->GetObjectClass(self);
    auto pointerField = env->GetFieldID(CoreClass, "pointer", "J");
    auto pointerValue = env->GetLongField(self, pointerField);

    if (pointer == 0 || pointerValue == 0 || pointerValue != pointer) {
      return JNI_FALSE;
    }

    return JNI_TRUE;
  }

  jboolean package_export(Core_verifyLoop)(
    JNIEnv *env,
    jobject self
  ) {
    if (!uv_default_loop()) {
      return JNI_FALSE;
    }

    return JNI_TRUE;
  }

  jboolean package_export(Core_verifyFS)(
    JNIEnv *env,
    jobject self
  ) {
    auto CoreClass = env->GetObjectClass(self);
    auto pointerField = env->GetFieldID(CoreClass, "pointer", "J");
    auto core = (NativeCore *) env->GetLongField(self, pointerField);
    auto loop = uv_default_loop();

    int err = 0;
    int fd = 0;
    uv_fs_t req;

    jboolean ok = JNI_FALSE;

    if (!core || !loop) {
      return JNI_FALSE;
    }

    req = {0};
    fd = uv_fs_open(loop, &req, VITAL_CHECK_FILE, 0, 0, [](uv_fs_t *req) {
      uv_fs_req_cleanup(req);
    });

    if (fd < 0) {
      env->ThrowNew(CoreClass, "uv_fs_open() failed");
      return JNI_FALSE;
    }

    char buf[2] = {0};
    const uv_buf_t iov = uv_buf_init(buf, 2);

    req = {0};
    req.data = &ok;
    err = uv_fs_read(loop, &req, fd, &iov, 1, 0, [](uv_fs_t* req) {
      auto value = (char *) req->bufs[0].base;

      if (value[0] == 'O' && value[1] == 'K') {
        *(jboolean *) req->data = JNI_TRUE;
      }

      uv_fs_req_cleanup(req);
    });

    if (err != 0) {
      env->ThrowNew(CoreClass, "uv_fs_read() failed");
      return JNI_FALSE;
    }

    req = {0};
    err = uv_fs_close(loop, &req, fd, [](uv_fs_t* req) {
      uv_fs_req_cleanup(req);
    });

    if (err != 0) {
      env->ThrowNew(CoreClass, "uv_fs_close() failed");
      return JNI_FALSE;
    }

    if (uv_run(loop, UV_RUN_DEFAULT) != 0) {
      env->ThrowNew(CoreClass, "uv_run() failed");
      return JNI_FALSE;
    }

    return ok;
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
