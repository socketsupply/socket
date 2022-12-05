#include "../core/core.hh"
#include "native.hh"

using namespace SSC;

class Runtime : public SSC::Core {
  public:
    JNIEnv *env = nullptr;
    jobject self = nullptr;

    Runtime (JNIEnv* env, jobject self) : SSC::Core() {
      this->env = env;
      this->self = env->NewGlobalRef(self);
    }

    ~Runtime () {
      this->env->DeleteGlobalRef(this->self);
    }
};

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
    auto pointer = GetObjectClassField(env, Long, "pointer", "J");
    auto runtime = reinterpret_cast<Runtime*>(pointer);

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
