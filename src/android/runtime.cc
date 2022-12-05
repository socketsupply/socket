#include "internal.hh"

using namespace SSC::android;

extern "C" {
  jlong external(Runtime, alloc)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = new Runtime(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return 0;
    }

    return reinterpret_cast<jlong>(runtime);
  }

  jboolean external(Runtime, dealloc)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    delete runtime;
    return true;
  }

  jboolean external(Runtime, isDebugEnabled) (
    JNIEnv* env,
    jobject self
  ) {
    return SSC::isDebugEnabled();
  }
}
