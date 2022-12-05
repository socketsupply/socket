#include "internal.hh"

using namespace SSC::android;

namespace SSC::android {
  Runtime::Runtime (JNIEnv* env, jobject self, String rootDirectory)
    : SSC::Core()
  {
    this->env = env;
    this->self = env->NewGlobalRef(self);
    this->pointer = reinterpret_cast<jlong>(this);
    this->rootDirectory = rootDirectory;
  }

  Runtime::~Runtime () {
    this->env->DeleteGlobalRef(this->self);
  }
}

extern "C" {
  jlong external(Runtime, alloc)(
    JNIEnv *env,
    jobject self,
    jstring rootDirectory
  ) {
    auto runtime = new Runtime(env, self, StringWrap(env, rootDirectory).str());

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return 0;
    }

    return runtime->pointer;
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
