#include "../app/app.hh"
#include "../platform/android.hh"

#include "window.hh"

using namespace SSC;

namespace SSC {
  Window::Window (SharedPointer<Core> core, const Window::Options& options)
    : options(options),
      core(core),
      bridge(core, IPC::Bridge::Options {
        options.userConfig,
        options.as<IPC::Preload::Options>()
      }),
      hotkey(this),
      dialog(this)
  {
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
      .conduit = this->core->conduit.port,
      .userScript = options.userScript
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
    return ScreenSize {0, 0};
  }

  void Window::about () {
  }

  void Window::eval (const String& source) {
    const auto app = App::sharedApplication();
    const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
    const auto sourceString = attachment.env->NewStringUTF(source.c_str());

    // `activity.evaluateJavaScript(index, source): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
      "evaluateJavaScript",
      "(ILjava/lang/String;)Z",
      this->index,
      sourceString
    );

    attachment.env->DeleteLocalRef(sourceString);
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
    const auto urlString = attachment.env->NewStringUTF(url.c_str());

    // `activity.navigateWindow(index, url): Boolean`
    const auto result = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
      "navigateWindow",
      "(ILjava/lang/String;)Z",
      this->index,
      urlString
    );

    attachment.env->DeleteLocalRef(urlString);

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
    const auto titleString = attachment.env->NewStringUTF(title.c_str());

    // `activity.setWindowTitle(index, url): Boolean`
    CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      app->activity,
      "setWindowTitle",
      "(ILjava/lang/String;)Z",
      this->index,
      titleString
    );

    attachment.env->DeleteLocalRef(titleString);
  }

  Window::Size Window::getSize () {
    return Size {0, 0};
  }

  const Window::Size Window::getSize () const {
    return Size {0, 0};
  }

  void Window::setSize (int height, int width, int _) {
  }

  void Window::setPosition (float x, float y) {
  }

  void Window::setContextMenu (const String&, const String&) {
  }

  void Window::closeContextMenu (const String&) {
  }

  void Window::closeContextMenu () {
  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
  }

  void Window::setBackgroundColor (const String& rgba) {
  }

  String Window::getBackgroundColor () {
    return "";
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
  }

  void Window::setSystemMenu (const String& dsl) {
  }

  void Window::setMenu (const String& dsl, const bool& isTrayMenu) {
  }

  void Window::setTrayMenu (const String& dsl) {
  }

  void Window::showInspector () {
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
