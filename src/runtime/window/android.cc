#include "../app/app.hh"

#include "window.hh"

using namespace SSC;

namespace SSC {
  Window::Window (SharedPointer<Core> core, const Window::Options& options)
    : options(options),
      core(core),
      bridge(core, IPC::Bridge::Options {
        options.index,
        options.userConfig,
        options.as<IPC::Preload::Options>()
      }),
      hotkey(this),
      dialog(this)
  {
    auto userConfig = options.userConfig;
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);

    this->index = this->options.index;
    this->bridge.userConfig = options.userConfig;
    this->bridge.configureNavigatorMounts();

    this->bridge.navigateFunction = [this] (const auto url) {
      this->navigate(url);
    };

    this->bridge.evaluateJavaScriptFunction = [this] (const auto source) {
      this->eval(source);
    };

    this->bridge.client.preload = IPC::Preload::compile({
      .client = UniqueClient {
        .id = this->bridge.client.id,
        .index = this->bridge.client.index
      },
      .index = options.index,
      .userScript = options.userScript,
      .userConfig = options.userConfig,
      .conduit = {
        {"port", this->core->conduit.port},
        {"hostname", this->core->conduit.hostname},
        {"sharedKey", this->core->conduit.sharedKey}
      }
    });

    // `activity.createWindow(index, shouldExitApplicationOnClose): Unit`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->activity,
      "createWindow",
      "(IZZ)V",
      options.index,
      options.shouldExitApplicationOnClose,
      options.headless
    );

    this->hotkey.init();
    this->bridge.init();
    this->bridge.configureSchemeHandlers({});
  }

  Window::~Window () {
    if (this->androidWindowRef) {
      const auto app = App::sharedApplication();
      const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
      attachment.env->DeleteGlobalRef(this->androidWindowRef);
      this->androidWindowRef = nullptr;
    }

    this->close();
  }

  ScreenSize Window::getScreenSize () {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.getWindowWidth(index): String`
    const auto width = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->activity,
      "getScreenSizeWidth",
      "()I"
    );

    const auto height = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->activity,
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
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    const auto token = std::to_string(rand64());

    // `activity.evaluateJavaScript(index, source): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
      "evaluateJavaScript",
      "(ILjava/lang/String;Ljava/lang/String;)Z",
      this->index,
      attachment.env->NewStringUTF(source.c_str()),
      attachment.env->NewStringUTF(token.c_str())
    );

    this->evaluateJavaScriptCallbacks.insert_or_assign(token, callback);
  }

  void Window::show () {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);

    // `activity.showWindow(index): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
      "showWindow",
      "(I)Z",
      this->index
    );
  }

  void Window::hide () {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);

    // `activity.hideWindow(index): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
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
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    JSON::Object json = JSON::Object::Entries {
      {"data", this->index}
    };

    this->eval(getEmitToRenderProcessJavaScript("window-closed", json.str()));

    // `activity.closeWindow(index): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
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
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);

    // `activity.navigateWindow(index, url): Boolean`
    const auto result = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
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
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.getWindowTitle(index): String`
    const auto titleString = (jstring) CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      app->activity,
      "getWindowTitle",
      "(I)Ljava/lang/String;",
      this->index
    );

    return Android::StringWrap(attachment.env, titleString).str();
  }

  void Window::setTitle (const String& title) {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.setWindowTitle(index, url): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
      "setWindowTitle",
      "(ILjava/lang/String;)Z",
      this->index,
      attachment.env->NewStringUTF(title.c_str())
    );
  }

  Window::Size Window::getSize () {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.getWindowWidth(index): String`
    const auto width = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->activity,
      "getWindowWidth",
      "(I)I",
      this->index
    );

    const auto height = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->activity,
      "getWindowHeight",
      "(I)I",
      this->index
    );
    this->size.width = width;
    this->size.height = height;
    return Size {width, height};
  }

  const Window::Size Window::getSize () const {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.getWindowWidth(index): Int`
    const auto width = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->activity,
      "getWindowWidth",
      "(I)I",
      this->index
    );

    // `activity.getWindowHeight(index): Int`
    const auto height = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->activity,
      "getWindowHeight",
      "(I)I",
      this->index
    );
    return Size {width, height};
  }

  void Window::setSize (int width, int height, int _) {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.setWindowSize(index, w): Int`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->activity,
      "setWindowSize",
      "(III)Z",
      this->index,
      width,
      height
    );
  }

  void Window::setPosition (float x, float y) {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    this->position.x = x;
    this->position.y = y;
    // `activity.setWindowBackgroundColor(index, color)`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->activity,
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
    const auto app = App::sharedApplication();
    const auto color = Color(r, g, b, a);
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.setWindowBackgroundColor(index, color)`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->activity,
      "setWindowBackgroundColor",
      "(IJ)Z",
      this->index,
      color.pack()
    );
  }

  void Window::setBackgroundColor (const String& rgba) {
    const auto app = App::sharedApplication();
    const auto color = Color(rgba);
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.setWindowBackgroundColor(index, color)`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->activity,
      "setWindowBackgroundColor",
      "(IJ)Z",
      this->index,
      color.pack()
    );
  }

  void Window::setBackgroundColor (const Color& color) {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.setWindowBackgroundColor(index, color)`
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      app->activity,
      "setWindowBackgroundColor",
      "(IJ)Z",
      this->index,
      color.pack()
    );
  }

  String Window::getBackgroundColor () {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    // `activity.getWindowBackgroundColor(index): Int`
    const auto color = Color(CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      app->activity,
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

    this->bridge.emit("applicationurl", json.str());
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

    const auto window = app->windowManager.getWindow(index);

    if (!window) {
      return ANDROID_THROW(env, "Invalid window index (%d) requested", index);
    }

    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    const auto message = Android::StringWrap(attachment.env, messageString).str();
    const auto size = byteArray != nullptr ? attachment.env->GetArrayLength(byteArray) : 0;

    if (byteArray && size > 0) {
      auto bytes = std::make_shared<char[]>(size);
      attachment.env->GetByteArrayRegion(byteArray, 0, size, (jbyte*) bytes.get());
      window->bridge.route(message, bytes, size);
    } else {
      window->bridge.route(message, nullptr, 0);
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

    const auto window = app->windowManager.getWindow(index);

    if (!window) {
      return ANDROID_THROW(env, "Invalid window index (%d) requested", index);
    }

    window->androidWindowRef = env->NewGlobalRef(self);
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

    const auto window = app->windowManager.getWindow(index);

    if (!window) {
      return ANDROID_THROW(env, "Invalid window index (%d) requested", index);
    }

    const auto token = Android::StringWrap(env, tokenString).str();
    const auto result = Android::StringWrap(env, resultString).str();

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

    const auto window = app->windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window index (%d) requested", index);
      return nullptr;
    }

    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
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

    const auto window = app->windowManager.getWindow(index);

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

    const auto window = app->windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window index (%d) requested", index);
      return nullptr;
    }

    auto userConfig = window->options.userConfig;
    auto preloadUserScriptSource = IPC::Preload::compile({
      .features = IPC::Preload::Options::Features {
        .useGlobalCommonJS = false,
        .useGlobalNodeJS = false,
        .useTestScript = false,
        .useHTMLMarkup = false,
        .useESM = false,
        .useGlobalArgs = true
      },
      .client = UniqueClient {
        .id = window->bridge.client.id,
        .index = window->bridge.client.index
      },
      .index = window->options.index,
      .userScript = window->options.userScript,
      .userConfig = window->options.userConfig,
      .conduit = {
        {"port", window->core->conduit.port},
        {"hostname", window->core->conduit.hostname},
        {"sharedKey", window->core->conduit.sharedKey}
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

    const auto window = app->windowManager.getWindow(index);

    if (!window) {
      return ANDROID_THROW(env, "Invalid window index (%d) requested", index);
    }

    const auto url = Android::StringWrap(env, urlString).str();
    window->handleApplicationURL(url);
  }
}
