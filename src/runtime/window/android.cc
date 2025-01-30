#include "../app.hh"
#include "../color.hh"
#include "../crypto.hh"
#include "../runtime.hh"
#include "../javascript.hh"

#include "../window.hh"

using namespace ssc::runtime;
using ssc::runtime::javascript::getEmitToRenderProcessJavaScript;
using ssc::runtime::crypto::rand64;
using ssc::runtime::color::Color;
using ssc::runtime::app::App;

namespace ssc::runtime::window {
  Window::Window (SharedPointer<bridge::Bridge> bridge, const Window::Options& options)
    : options(options),
      bridge(bridge),
      hotkey(this),
      dialog(this)
  {
    auto userConfig = options.userConfig;
    const auto attachment = android::JNIEnvironmentAttachment(this->bridge->context.getRuntime()->android.jvm);

    this->self = nullptr;
    this->index = this->options.index;
    this->bridge->configureNavigatorMounts();

    this->bridge->navigateHandler  = [this] (const auto url) {
      this->navigate(url);
    };

    this->bridge->evaluateJavaScriptHandler = [this] (const auto source) {
      this->eval(source);
    };

    this->bridge->client.preload = webview::Preload::compile({
      .client = this->bridge->client,
      .index = options.index,
      .userScript = options.userScript,
      .userConfig = options.userConfig,
      .conduit = {
        {"port", this->bridge->context.getRuntime()->services.conduit.port},
        {"hostname", this->bridge->context.getRuntime()->services.conduit.hostname},
        {"sharedKey", this->bridge->context.getRuntime()->services.conduit.sharedKey}
      }
    });

    // `activity.createWindow(index, shouldExitApplicationOnClose): Unit`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      this->bridge->context.android.activity,
      "createWindow",
      "(IZZ)V",
      options.index,
      options.shouldExitApplicationOnClose,
      options.headless
    );

    this->hotkey.init();
    this->bridge->init();
    this->bridge->configureSchemeHandlers({});
  }

  Window::~Window () {
    if (this->self) {
      const auto app = App::sharedApplication();
      const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
      attachment.env->DeleteGlobalRef(this->self);
      this->self = nullptr;
    }

    this->close();
  }

  ScreenSize Window::getScreenSize () {
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.getWindowWidth(index): String`
    const auto width = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->runtime.android.activity,
      "getScreenSizeWidth",
      "()I"
    );

    const auto height = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->runtime.android.activity,
      "getScreenSizeHeight",
      "()I"
    );

    return ScreenSize {width, height};
  }

  void Window::about () {
    // XXX(@jwerle): not supported
    // TODO(@jwerle): figure out we'll go about making this possible
  }

  void Window::eval (const String& source, const EvalCallback callback) {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    const auto token = std::to_string(rand64());

    // `activity.evaluateJavaScript(index, source): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->runtime.android.activity,
      "evaluateJavaScript",
      "(ILjava/lang/String;Ljava/lang/String;)Z",
      this->index,
      attachment.env->NewStringUTF(source.c_str()),
      attachment.env->NewStringUTF(token.c_str())
    );

    this->evaluateJavaScriptCallbacks.insert_or_assign(token, callback);
  }

  void Window::show () {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);

    // `activity.showWindow(index): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->runtime.android.activity,
      "showWindow",
      "(I)Z",
      this->index
    );
  }

  void Window::hide () {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);

    // `activity.hideWindow(index): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->runtime.android.activity,
      "hideWindow",
      "(I)Z",
      this->index
    );
  }

  void Window::kill () {
    return this->close(0);
  }

  void Window::exit (int code) {
    return this->close(code);
  }

  void Window::close (int code) {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    JSON::Object json = JSON::Object::Entries {
      {"data", this->index}
    };

    this->eval(getEmitToRenderProcessJavaScript("window-closed", json.str()));

    // `activity.closeWindow(index): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->runtime.android.activity,
      "closeWindow",
      "(I)Z",
      this->index
    );
  }

  void Window::minimize () {
    this->hide();
  }

  void Window::maximize () {
    this->show();
  }

  void Window::restore () {
    this->show();
  }

  void Window::navigate (const String& url) {
    if (this->self == nullptr) {
      this->pendingNavigationLocation = url;
      return;
    }

    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);

    // `activity.navigateWindow(index, url): Boolean`
    const auto result = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->runtime.android.activity,
      "navigateWindow",
      "(ILjava/lang/String;)Z",
      this->index,
      attachment.env->NewStringUTF(url.c_str())
    );

    if (!result) {
      this->pendingNavigationLocation = url;
    }
  }

  const String Window::getTitle () const {
    if (this->self == nullptr) {
      return "";
    }

    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.getWindowTitle(index): String`
    const auto titleString = (jstring) CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      app->runtime.android.activity,
      "getWindowTitle",
      "(I)Ljava/lang/String;",
      this->index
    );

    return android::StringWrap(attachment.env, titleString).str();
  }

  void Window::setTitle (const String& title) {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.setWindowTitle(index, url): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->runtime.android.activity,
      "setWindowTitle",
      "(ILjava/lang/String;)Z",
      this->index,
      attachment.env->NewStringUTF(title.c_str())
    );
  }

  Window::Size Window::getSize () {
    if (this->self == nullptr) {
      return Size {0, 0};
    }

    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.getWindowWidth(index): String`
    const auto width = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->runtime.android.activity,
      "getWindowWidth",
      "(I)I",
      this->index
    );

    const auto height = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->runtime.android.activity,
      "getWindowHeight",
      "(I)I",
      this->index
    );
    this->size.width = width;
    this->size.height = height;
    return Size {width, height};
  }

  const Window::Size Window::getSize () const {
    if (this->self == nullptr) {
      return Size { 0, 0 };
    }

    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.getWindowWidth(index): Int`
    const auto width = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->runtime.android.activity,
      "getWindowWidth",
      "(I)I",
      this->index
    );

    // `activity.getWindowHeight(index): Int`
    const auto height = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->runtime.android.activity,
      "getWindowHeight",
      "(I)I",
      this->index
    );
    return Size {width, height};
  }

  void Window::setSize (int width, int height, int _) {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.setWindowSize(index, w): Int`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->runtime.android.activity,
      "setWindowSize",
      "(III)Z",
      this->index,
      width,
      height
    );
  }

  void Window::setPosition (float x, float y) {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    this->position.x = x;
    this->position.y = y;
    // `activity.setWindowPosition(index, x, y)`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->runtime.android.activity,
      "setWindowPosition",
      "(IFF)Z",
      this->index,
      x,
      y
    );
  }

  void Window::setContextMenu (const String&, const String&) {
    // XXX(@jwerle): not supported
  }

  void Window::closeContextMenu (const String&) {
    // XXX(@jwerle): not supported
  }

  void Window::closeContextMenu () {
    // XXX(@jwerle): not supported
  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto color = Color(r, g, b, a);
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.setWindowBackgroundColor(index, color)`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->runtime.android.activity,
      "setWindowBackgroundColor",
      "(IJ)Z",
      this->index,
      color.pack()
    );
  }

  void Window::setBackgroundColor (const String& rgba) {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto color = Color(rgba);
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.setWindowBackgroundColor(index, color)`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->runtime.android.activity,
      "setWindowBackgroundColor",
      "(IJ)Z",
      this->index,
      color.pack()
    );
  }

  void Window::setBackgroundColor (const Color& color) {
    if (this->self == nullptr) return;
    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.setWindowBackgroundColor(index, color)`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->runtime.android.activity,
      "setWindowBackgroundColor",
      "(IJ)Z",
      this->index,
      color.pack()
    );
  }

  String Window::getBackgroundColor () {
    if (this->self == nullptr) {
      return "";
    }

    const auto app = App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    // `activity.getWindowBackgroundColor(index): Int`
    const auto color = Color(CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->runtime.android.activity,
      "getWindowBackgroundColor",
      "(I)I",
      this->index
    ));

    return color.str();
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    // XXX(@jwerle): not supported
  }

  void Window::setSystemMenu (const String& dsl) {
    // XXX(@jwerle): not supported
  }

  void Window::setMenu (const String& dsl, const bool& isTrayMenu) {
    // XXX(@jwerle): not supported
  }

  void Window::setTrayMenu (const String& dsl) {
    // XXX(@jwerle): not supported
  }

  void Window::showInspector () {
    // XXX(@jwerle): not supported
  }

  void Window::handleApplicationURL (const String& url) {
    JSON::Object json = JSON::Object::Entries {{
      "url", url
    }};

    this->bridge->emit("applicationurl", json.str());
  }
}

extern "C" {
  void ANDROID_EXTERNAL(window, Window, onMessage) (
    JNIEnv* env,
    jobject self,
    jint index,
    jstring messageString,
    jbyteArray byteArray
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      return ANDROID_THROW(env, "Missing 'App' in environment");
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      return ANDROID_THROW(env, "Invalid window index (%d) requested", index);
    }

    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    const auto message = android::StringWrap(attachment.env, messageString).str();
    const auto size = byteArray != nullptr ? attachment.env->GetArrayLength(byteArray) : 0;

    if (byteArray && size > 0) {
      auto bytes = std::make_shared<unsigned char[]>(size);
      attachment.env->GetByteArrayRegion(byteArray, 0, size, (jbyte*) bytes.get());
      window->bridge->route(message, bytes, size);
    } else {
      window->bridge->route(message, nullptr, 0);
    }
  }

  void ANDROID_EXTERNAL(window, Window, onReady) (
    JNIEnv* env,
    jobject self,
    jint index
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      return ANDROID_THROW(env, "Missing 'App' in environment");
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      return ANDROID_THROW(env, "Invalid window index (%d) requested", index);
    }

    window->self = env->NewGlobalRef(self);

    if (!window->options.headless) {
      app->dispatch([window]() {
        window->show();
      });
    }
  }

  void ANDROID_EXTERNAL(window, Window, onEvaluateJavascriptResult) (
    JNIEnv* env,
    jobject self,
    jint index,
    jstring tokenString,
    jstring resultString
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      return ANDROID_THROW(env, "Missing 'App' in environment");
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      return ANDROID_THROW(env, "Invalid window index (%d) requested", index);
    }

    const auto token = android::StringWrap(env, tokenString).str();
    const auto result = android::StringWrap(env, resultString).str();

    Lock lock(window->mutex);

    if (!window->evaluateJavaScriptCallbacks.contains(token)) {
      return;
    }

    const auto callback = window->evaluateJavaScriptCallbacks.at(token);

    if (callback != nullptr) {
      if (result == "null" || result == "undefined") {
        callback(JSON::Null());
      } else if (result == "true") {
        callback(JSON::Boolean(true));
      } else if (result == "false") {
        callback(JSON::Boolean(false));
      } else {
        double number = 0.0f;

        try {
          number = std::stod(result);
        } catch (...) {
          callback(JSON::String(result));
          return;
        }

        callback(JSON::Number(number));
      }
    }
  }

  jstring ANDROID_EXTERNAL(window, Window, getPendingNavigationLocation) (
    JNIEnv* env,
    jobject self,
    jint index
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return nullptr;
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window index (%d) requested", index);
      return nullptr;
    }

    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    const auto pendingNavigationLocation = window->pendingNavigationLocation;
    window->pendingNavigationLocation = "";
    return attachment.env->NewStringUTF(pendingNavigationLocation.c_str());
  }

  jstring ANDROID_EXTERNAL(window, Window, getBundleIdentifier) (
    JNIEnv* env,
    jobject self,
    jint index
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return nullptr;
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window index (%d) requested", index);
      return nullptr;
    }

    auto userConfig = window->options.userConfig;
    return env->NewStringUTF(userConfig["meta_bundle_identifier"].c_str());
  }

  jstring ANDROID_EXTERNAL(window, Window, getPreloadUserScript) (
    JNIEnv* env,
    jobject self,
    jint index
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return nullptr;
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window index (%d) requested", index);
      return nullptr;
    }

    auto userConfig = window->options.userConfig;
    auto preloadUserScriptSource = webview::Preload::compile({
      .features = webview::Preload::Options::Features {
        .useGlobalCommonJS = false,
        .useGlobalNodeJS = false,
        .useTestScript = false,
        .useHTMLMarkup = false,
        .useESM = false,
        .useGlobalArgs = true
      },
      .client = window->bridge->client,
      .index = window->options.index,
      .userScript = window->options.userScript,
      .userConfig = window->options.userConfig,
      .conduit = {
        {"port", window->bridge->context.getRuntime()->services.conduit.port},
        {"hostname", window->bridge->context.getRuntime()->services.conduit.hostname},
        {"sharedKey", window->bridge->context.getRuntime()->services.conduit.sharedKey}
      }
    });

    return env->NewStringUTF(preloadUserScriptSource.compile().c_str());
  }

  void ANDROID_EXTERNAL(window, Window, handleApplicationURL) (
    JNIEnv* env,
    jobject self,
    jint index,
    jstring urlString
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      return ANDROID_THROW(env, "Missing 'App' in environment");
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      return ANDROID_THROW(env, "Invalid window index (%d) requested", index);
    }

    const auto url = android::StringWrap(env, urlString).str();
    window->handleApplicationURL(url);
  }
}
