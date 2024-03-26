#include "internal.hh"

using namespace SSC::android;

static auto onInternalServiceWorkerFetchResponseSignature =
  "("
  "J" // requestId
  "I" // statusCode
  "Ljava/lang/String;" // headers
  "[B" // bytes
  ")V";

namespace SSC::android {
  Runtime::Runtime (JNIEnv* env, jobject self, String rootDirectory)
    : SSC::Core()
  {
    this->env = env;
    this->self = env->NewGlobalRef(self);
    this->pointer = reinterpret_cast<jlong>(this);
    this->rootDirectory = rootDirectory;
  }

  bool Runtime::isPermissionAllowed (const String& name) const {
    static const auto config = SSC::getUserConfig();
    const auto permission = String("permissions_allow_") + replace(name, "-", "_");

    // `true` by default
    if (!config.contains(permission)) {
      return true;
    }

    return config.at(permission) != "false";
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

  jboolean external(Runtime, startEventLoop)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->runEventLoop();
    return true;
  }

  jboolean external(Runtime, stopEventLoop)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->stopEventLoop();
    return true;
  }

  jboolean external(Runtime, startTimers)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->startTimers();
    return true;
  }

  jboolean external(Runtime, stopTimers)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->stopTimers();
    return true;
  }

  jboolean external(Runtime, pause)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->pauseAllPeers();
    runtime->stopTimers();
    runtime->stopEventLoop();

    return true;
  }

  jboolean external(Runtime, resume)(
    JNIEnv *env,
    jobject self
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->runEventLoop();
    runtime->resumeAllPeers();

    return true;
  }

  jboolean external(Runtime, isPermissionAllowed)(
    JNIEnv *env,
    jobject self,
    jstring permission
  ) {
    auto runtime = Runtime::from(env, self);
    auto name = StringWrap(env, permission).str();

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    return runtime->isPermissionAllowed(name);
  }

  jboolean external(Runtime, setIsEmulator)(
    JNIEnv *env,
    jobject self,
    jboolean value
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    runtime->isEmulator = value;
    return true;
  }

  jstring external(Runtime, getConfigValue)(
    JNIEnv *env,
    jobject self,
    jstring keyString
  ) {
    static auto config = SSC::getUserConfig();
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return nullptr;
    }

    auto key = StringWrap(env, keyString).str();
    auto value = config[key];
    return env->NewStringUTF(value.c_str());
  }

  jboolean external(Runtime, serviceWorkerContainerFetch)(
    JNIEnv *env,
    jobject self,
    jlong requestId,
    jstring pathnameString,
    jstring methodString,
    jstring queryString,
    jstring headersString,
    jbyteArray byteArray,
  ) {
    auto runtime = Runtime::from(env, self);

    if (runtime == nullptr) {
      Throw(env, RuntimeNotInitializedException);
      return false;
    }

    if (runtime->serviceWorker.registrations.size() == 0) {
      return false;
    }

    JavaVM* jvm = nullptr;
    auto jniVersion = env->GetVersion();
    env->GetJavaVM(&jvm);
    auto attachment = JNIEnvironmentAttachment { jvm, jniVersion };

    if (attachment.hasException()) {
      return false;
    }

    auto request = SSC::ServiceWorkerContainer::FetchRequest {
      StringWrap(env, pathnameString).str(),
      StringWrap(env, methodString).str()
    };

    auto size = byteArray != nullptr ? env->GetArrayLength(byteArray) : 0;
    auto input = size > 0 ? new char[size]{0} : nullptr;

    if (size > 0 && input != nullptr) {
      env->GetByteArrayRegion(byteArray, 0, size, (jbyte*) input);
    }

    request.buffer.bytes = input;
    request.buffer.size = size;
    request.query = StringWrap(env, queryString).str();
    request.headers = SSC::split(StringWrap(env, headersString).str(), '\n');

    return runtime->serviceWorker.fetch(request, [=](auto response) mutable {
      auto attachment = JNIEnvironmentAttachment { jvm, jniVersion };
      auto env = attachment.env;

      if (attachment.hasException()) {
        return;
      }

      const auto statusCode = response.statusCode;
      const auto headers = env->NewStringUTF(SSC::join(response.headers, '\n').c_str());
      const auto bytes = response.buffer.bytes && response.buffer.size > 0
        ? env->NewByteArray(response.buffer.size)
        : nullptr;

      if (bytes != nullptr) {
        env->SetByteArrayRegion(
          bytes,
          0,
          response.buffer.size,
          (jbyte *) response.buffer.bytes
        );
      }

      CallVoidClassMethodFromEnvironment(
        env,
        runtime->self,
        "onInternalServiceWorkerFetchResponse",
        onInternalServiceWorkerFetchResponseSignature,
        requestId,
        statusCode,
        headers,
        bytes
      );

      env->DeleteLocalRef(headers);

      if (bytes != nullptr) {
        env->DeleteLocalRef(bytes);
      }
    });
  }
}
