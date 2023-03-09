#include "internal.hh"

using namespace SSC::android;

static auto onInternalRouteResponseSignature =
  "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[B)V";

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
    jstring uriString,
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
    auto attachment = JNIEnvironmentAttachment { jvm, jniVersion };

    if (attachment.hasException()) {
      return false;
    }

    auto uri = StringWrap(env, uriString);
    auto size = byteArray != nullptr ? env->GetArrayLength(byteArray) : 0;
    auto input = size > 0 ? new char[size]{0} : nullptr;

    if (size > 0 && input != nullptr) {
      env->GetByteArrayRegion(byteArray, 0, size, (jbyte*) input);
    }

    auto routed = bridge->route(uri.str(), input, size, [=](auto result) mutable {
      if (result.seq == "-1") {
        bridge->router.send(result.seq, result.str(), result.post);
        return;
      }

      auto attachment = JNIEnvironmentAttachment { jvm, jniVersion };
      auto self = bridge->self;
      auto env = attachment.env;

      if (!attachment.hasException()) {
        auto size = result.post.length;
        auto body = result.post.body;
        auto bytes = body ? env->NewByteArray(size) : nullptr;

        if (bytes != nullptr) {
          env->SetByteArrayRegion(bytes, 0, size, (jbyte *) body);
        }

        auto seq = env->NewStringUTF(result.seq.c_str());
        auto source = env->NewStringUTF(result.source.c_str());
        auto value = env->NewStringUTF(result.str().c_str());
        auto headers = env->NewStringUTF(result.post.headers.c_str());

        CallVoidClassMethodFromEnvironment(
          env,
          self,
          "onInternalRouteResponse",
          onInternalRouteResponseSignature,
          requestId,
          seq,
          source,
          value,
          headers,
          bytes
        );

        env->DeleteLocalRef(seq);
        env->DeleteLocalRef(source);
        env->DeleteLocalRef(value);
        env->DeleteLocalRef(headers);

        if (bytes != nullptr) {
          env->DeleteLocalRef(bytes);
        }
      }
    });

    delete [] input;

    if (!routed) {
      auto attachment = JNIEnvironmentAttachment { jvm, jniVersion };
      auto env = attachment.env;

      if (!attachment.hasException()) {
        auto msg = SSC::IPC::Message{uri.str()};
        auto err = SSC::JSON::Object::Entries {
          {"source", uri.str()},
          {"err", SSC::JSON::Object::Entries {
            {"message", "Not found"},
            {"type", "NotFoundError"},
            {"url", uri.str()}
          }}
        };

        auto seq = env->NewStringUTF(msg.seq.c_str());
        auto source =env->NewStringUTF(msg.name.c_str());
        auto value = env->NewStringUTF(SSC::JSON::Object(err).str().c_str());
        auto headers = env->NewStringUTF("");

        CallVoidClassMethodFromEnvironment(
          env,
          self,
          "onInternalRouteResponse",
          onInternalRouteResponseSignature,
          requestId,
          seq,
          source,
          value,
          headers,
          nullptr
        );

        env->DeleteLocalRef(seq);
        env->DeleteLocalRef(source);
        env->DeleteLocalRef(value);
        env->DeleteLocalRef(headers);
      }
    }

    return routed;
  }
}
