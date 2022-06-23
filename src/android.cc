#include "android.hh"
#include "jni.h"

#include <stdio.h>

constexpr auto VITAL_CHECK_FILE = "file:///android_asset/vital_check_ok.txt";

#ifdef __ANDROID__
#include <android/log.h>
#define debug(format, ...)                                                     \
    {                                                                          \
      __android_log_print(                                                     \
        ANDROID_LOG_DEBUG,                                                     \
        "NativeCore(",                                                         \
        format,                                                                \
        ##__VA_ARGS__                                                          \
      );                                                                       \
    }
#else
#define debug(format, ...)                                                     \
   {                                                                           \
     printf("SSC::Core::Debug:");                                              \
     printf(format, ##__VA_ARGS__);                                            \
     printf("\n");                                                             \
     fflush(stdout);                                                           \
   }
#endif

struct NativeRefs {
  jobject core;
  jobject callbacks;
};

class NativeCore : SSC::Core {
  JavaVM *jvm;
  JNIEnv *env;
  NativeRefs refs;

  public:
  NativeCore(JNIEnv *env, jobject core) : Core() {
    this->env = env;
    this->env->GetJavaVM(&this->jvm);
    //this->refs.core = this->env->NewGlobalRef(core);
  }

  ~NativeCore () {
    //this->env->DeleteGlobalRef(this->refs.core);
  }

  JavaVM *
  GetJavaVM () {
    return this->jvm;
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
      return false;
    }

    return true;
  }

  void package_export(Core_verifyNativeExceptions)(
    JNIEnv *env,
    jobject self
  ) {
    auto Exception = env->FindClass("java/lang/Exception");
    env->ThrowNew(Exception, "exception check");
  }

  jboolean package_export(Core_verifyLoop)(
    JNIEnv *env,
    jobject self
  ) {
    if (!uv_default_loop()) {
      return false;
    }

    return true;
  }

  jboolean package_export(Core_verifyFS)(
    JNIEnv *env,
    jobject self
  ) {
    auto CoreClass = env->GetObjectClass(self);
    auto Exception = env->FindClass("java/lang/Exception");
    auto pointerField = env->GetFieldID(CoreClass, "pointer", "J");
    auto core = (NativeCore *) env->GetLongField(self, pointerField);
    auto loop = uv_default_loop();

    jboolean ok = false;

    struct { uv_fs_t open, read, close; } reqs = {0};
    int err = 0;
    int fd = 0;

    if (!core) {
      env->ThrowNew(Exception, "core is not initialized");
      return false;
    }

    if (!loop) {
      env->ThrowNew(Exception, "loop is not initialized");
      return false;
    }

    fd = uv_fs_open(loop, &reqs.open, VITAL_CHECK_FILE, 0, 0, [](uv_fs_t *req) {
      uv_fs_req_cleanup(req);
    });

    if (fd < 0) {
      env->ThrowNew(Exception, uv_strerror(fd));
      return false;
    }

    err = uv_run(loop, UV_RUN_DEFAULT);

    if (err != 0) {
      env->ThrowNew(Exception, uv_strerror(err));
      return false;
    }

    auto buf = new char[3];
    //static char buf[2] = {0};
    const uv_buf_t iov = uv_buf_init(buf, 3);

    reqs.read.data = &ok;
    err = uv_fs_read(loop, &reqs.read, fd, &iov, 1, 0, [](uv_fs_t* req) {
      if (req->result == 0) {
        //auto value = (char *) req->bufs[0].base;
        //if (req->bufs[0].len == 2) {
          //if (value[0] == 'O' && value[1] == 'K') {
            //*((jboolean *) req->data) = true;
          //}
        //}
      }

      uv_fs_req_cleanup(req);
    });

    if (err != 0) {
      env->ThrowNew(Exception, uv_strerror(err));
      return false;
    }

    err = uv_run(loop, UV_RUN_DEFAULT);

    if (err != 0) {
      env->ThrowNew(Exception, uv_strerror(err));
      return false;
    }

    err = uv_fs_close(loop, &reqs.close, fd, [](uv_fs_t* req) {
      uv_fs_req_cleanup(req);
    });

    if (err != 0) {
      env->ThrowNew(Exception, uv_strerror(err));
      return false;
    }

    if (err != 0) {
      env->ThrowNew(Exception, uv_strerror(err));
      return false;
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
