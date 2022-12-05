#include "../core/core.hh"
#include "../ipc/ipc.hh"
#include "internal.hh"

using namespace SSC::android;

namespace SSC::android {
  Window::Window (
    JNIEnv* env,
    jobject self,
    SSC::IPC::Bridge* bridge,
    SSC::WindowOptions options
  ) : options(options) {
    this->env = env;
    this->self = env->NewGlobalRef(self);
    this->bridge = bridge;
    this->config = SSC::parseConfig(SSC::decodeURIComponent(SSC::getSettingsSource()));

    SSC::StringStream stream;

    for (auto const &var : SSC::split(this->config["env"], ',')) {
      auto key = SSC::trim(var);
      auto value = SSC::getEnv(key.c_str());

      if (value.size() > 0) {
        stream << key << "=" << SSC::encodeURIComponent(value) << "&";
        envvars[key] = value;
      }
    }

    StringWrap rootDirectory(env, (jstring) CallObjectClassMethodFromEnvironment(
      env,
      self,
      "getRootDirectory",
      "()Ljava/lang/String;"
    ));

    options.headless = this->config["headless"] == "true";
    options.debug = SSC::isDebugEnabled() ? true : false;
    options.env = stream.str();
    options.cwd = rootDirectory.str();

    preloadSource.assign(
      "window.addEventListener('unhandledrejection', e => {        \n"
      "  console.log(e.reason || e.message || e);                  \n"
      "});                                                         \n"
      "                                                            \n"
      "window.addEventListener('error', e => {                     \n"
      "  const message = e.reason || e.message || e;               \n"
      "  if (!/debug-evaluate/.test(message)) {                    \n"
      "    console.log(message);                                   \n"
      "  }                                                         \n"
      "});                                                         \n"
      "                                                            \n"
      + createPreload(options)
    );
  }
}

extern "C" {
  jlong external(Window, alloc)(
    JNIEnv *env,
    jobject self,
    jlong bridgePointer
  ) {
    auto bridge = reinterpret_cast<SSC::IPC::Bridge*>(bridgePointer);

    if (bridge == nullptr) {
      Throw(env, BridgeNotInitializedException);
      return 0;
    }

    auto options = SSC::WindowOptions {};
    auto window = new Window(env, self, bridge, options);

    if (window == nullptr) {
      Throw(env, WindowNotInitializedException);
      return 0;
    }

    return reinterpret_cast<jlong>(window);
  }

  jboolean external(Window, dealloc)(
    JNIEnv *env,
    jobject self
  ) {
    auto window = Window::from(env, self);

    if (window == nullptr) {
      Throw(env, WindowNotInitializedException);
      return false;
    }

    delete window;
    return true;
  }

  jstring external(Window, getPathToFileToLoad)(
    JNIEnv *env,
    jobject self
  ) {
    auto window = Window::from(env, self);

    if (window == nullptr) {
      Throw(env, WindowNotInitializedException);
      return nullptr;
    }

    auto filename = window->config["android_index_html"];

    if (filename.size() > 0) {
      return env->NewStringUTF(filename.c_str());
    }

    return env->NewStringUTF("index.html");
  }

  jstring external(Window, getJavaScriptPreloadSource)(
    JNIEnv *env,
    jobject self
  ) {
    auto window = Window::from(env, self);

    if (window == nullptr) {
      Throw(env, WindowNotInitializedException);
      return nullptr;
    }

    auto source = window->preloadSource.c_str();

    return env->NewStringUTF(source);
  }
}
