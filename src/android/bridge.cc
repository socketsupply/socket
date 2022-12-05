#include "internal.hh"

using namespace SSC::android;

namespace SSC::android {
  Bridge::Bridge (JNIEnv* env, jobject self, Runtime* runtime)
    : IPC::Bridge(runtime)
  {
    this->env = env;
    this->self = env->NewGlobalRef(self);
    this->pointer = reinterpret_cast<jlong>(this);
    this->runtime = runtime;
    this->router.dispatchFunction = [this](auto callback) {
      if (callback != nullptr) {
        callback();
      }
    };
  }

  Bridge::~Bridge () {
    this->env->DeleteGlobalRef(this->self);
    IPC::Bridge::~Bridge();
  }
}

extern "C" {
  jlong external(Bridge, alloc)(
    JNIEnv *env,
    jobject self,
    jlong runtimePointer
  ) {
    auto runtime = Runtime::from(runtimePointer);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return 0;
    }

    auto bridge = new Bridge(env, self, runtime);

    if (bridge == nullptr) {
      Throw(env, BridgeNotInitializedException);
      return 0;
    }

    return bridge->pointer;
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

  jboolean external(Bridge, route)(
    JNIEnv *env,
    jobject self,
    jstring messageString,
    jbyteArray byteArray,
    jlong requestId
  ) {
    auto bridge = Bridge::from(env, self);

    if (bridge == nullptr) {
      Throw(env, BridgeNotInitializedException);
      return false;
    }

    JavaVM* jvm = nullptr;
    auto jniVersion = env->GetVersion();
    env->GetJavaVM(&jvm);

    auto msg = StringWrap(env, messageString);
    auto size = byteArray != nullptr ? env->GetArrayLength(byteArray) : 0;
    auto bytes = size > 0 ? new char[size]{0} : nullptr;

    if (size > 0 && bytes != nullptr) {
      env->GetByteArrayRegion(byteArray, 0, size, (jbyte*) bytes);
    }

    debug("rootDirectory= %s", bridge->runtime->rootDirectory.c_str());
    uv_chdir(bridge->runtime->rootDirectory.c_str());
    debug("route: %s", msg.c_str());
    return bridge->route(msg.str(), bytes, size, [=](auto result) mutable {
      auto attachment = JNIEnvironmentAttachment { jvm, jniVersion };
      auto self = bridge->self;
      auto env = attachment.env;

        debug("in callback");
      if (!attachment.hasException()) {
        debug("here");
        auto bytes = env->NewByteArray(result.post.length);
        env->SetByteArrayRegion(bytes, 0, result.post.length, (jbyte *) result.post.body);
        CallVoidClassMethodFromEnvironment(
          env,
          self,
          "onInternalRouteResponse",
          "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[B)V",
          requestId,
          env->NewStringUTF(result.seq.c_str()),
          env->NewStringUTF(result.source.c_str()),
          env->NewStringUTF(result.str().c_str()),
          env->NewStringUTF(result.post.headers.c_str()),
          bytes
        );
      }
    });
  }
}
