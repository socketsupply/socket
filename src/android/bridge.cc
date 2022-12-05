#include "../core/core.hh"
#include "../ipc/ipc.hh"
#include "native.hh"

class Bridge : public SSC::IPC::Bridge {
  public:
    JNIEnv *env = nullptr;
    jobject self = nullptr;

    Bridge (JNIEnv* env, jobject self, SSC::Core* core)
      : SSC::IPC::Bridge(core)
    {
      this->env = env;
      this->self = env->NewGlobalRef(self);
    }

    ~Bridge () {
      this->env->DeleteGlobalRef(this->self);
      SSC::IPC::Bridge::~Bridge();
    }
};

extern "C" {
  jlong external(Bridge, alloc)(
    JNIEnv *env,
    jobject self,
    jlong corePointer
  ) {
    auto core = reinterpret_cast<SSC::Core*>(corePointer);

    if (core == nullptr) {
      Throw(env, CoreNotInitializedException);
      return 0;
    }

    auto bridge = new Bridge(env, self, core);

    if (bridge == nullptr) {
      Throw(env, BridgeNotInitializedException);
      return 0;
    }

    return reinterpret_cast<jlong>(bridge);
  }

  jboolean external(Bridge, dealloc)(
    JNIEnv *env,
    jobject self
  ) {
    auto pointer = GetObjectClassField(env, Long, "pointer", "J");
    auto bridge = reinterpret_cast<SSC::IPC::Bridge*>(pointer);

    if (bridge == nullptr) {
      Throw(env, BridgeNotInitializedException);
      return false;
    }

    delete bridge;
    return true;
  }
}
