#include "internal.hh"

using namespace SSC::android;

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
    auto bridge = Bridge::from(env, self);

    if (bridge == nullptr) {
      Throw(env, BridgeNotInitializedException);
      return false;
    }

    delete bridge;
    return true;
  }
}
