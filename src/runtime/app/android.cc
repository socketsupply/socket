#include "../filesystem.hh"
#include "../config.hh"
#include "../string.hh"
#include "../window.hh"
#include "../cwd.hh"
#include "../env.hh"

#include "../app.hh"

using namespace ssc::runtime::app;
using namespace ssc::runtime;

using ssc::runtime::config::isDebugEnabled;
using ssc::runtime::config::getUserConfig;
using ssc::runtime::config::getDevHost;
using ssc::runtime::config::getDevPort;

using ssc::runtime::string::split;
using ssc::runtime::string::trim;

extern "C" {
  jlong ANDROID_EXTERNAL(app, App, alloc)(JNIEnv *env, jobject self) {
    if (App::sharedApplication() == nullptr) {
      const auto app = new App({
        .env = env,
        .self = self
      });
    }

    return reinterpret_cast<jlong>(App::sharedApplication().get());
  }

  jboolean ANDROID_EXTERNAL(app, App, setRootDirectory)(
    JNIEnv *env,
    jobject self,
    jstring rootDirectoryString
  ) {
    const auto rootDirectory = android::StringWrap(env, rootDirectoryString).str();
    setcwd(rootDirectory);
    uv_chdir(rootDirectory.c_str());
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setExternalStorageDirectory)(
    JNIEnv *env,
    jobject self,
    jstring externalStorageDirectoryString
  ) {
    const auto directory = android::StringWrap(env, externalStorageDirectoryString).str();
    filesystem::Resource::setExternalAndroidStorageDirectory(Path(directory));
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setExternalFilesDirectory)(
    JNIEnv *env,
    jobject self,
    jstring externalFilesDirectoryString
  ) {
    const auto directory = android::StringWrap(env, externalFilesDirectoryString).str();
    filesystem::Resource::setExternalAndroidFilesDirectory(Path(directory));
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setExternalCacheDirectory)(
    JNIEnv *env,
    jobject self,
    jstring externalCacheDirectoryString
  ) {
    const auto directory = android::StringWrap(env, externalCacheDirectoryString).str();
    filesystem::Resource::setExternalAndroidCacheDirectory(Path(directory));
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setAssetManager)(
    JNIEnv *env,
    jobject self,
    jobject assetManager
  ) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    if (!assetManager) {
      ANDROID_THROW(env, "'assetManager' object is null");
      return false;
    }

    // `Core::FS` and `filesystem::Resource` will use the asset manager
    // when looking for file resources, the asset manager will
    // take precedence
    filesystem::Resource::setSharedAndroidAssetManager(AAssetManager_fromJava(env, assetManager));
    return true;
  }

  jboolean ANDROID_EXTERNAL(app, App, setMimeTypeMap)(
    JNIEnv *env,
    jobject self,
    jobject mimeTypeMap
  ) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    if (!mimeTypeMap) {
      ANDROID_THROW(env, "'mimeTypeMap' object is null");
      return false;
    }

    android::initializeMimeTypeMap(env->NewGlobalRef(mimeTypeMap), app->runtime.android.jvm);
    return true;
  }

  void ANDROID_EXTERNAL(app, App, setBuildInformation)(
    JNIEnv *env,
    jobject self,
    jstring brandString,
    jstring deviceString,
    jstring fingerprintString,
    jstring hardwareString,
    jstring modelString,
    jstring manufacturerString,
    jstring productString
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->runtime.android.buildInformation.brand = android::StringWrap(env, brandString).str();
    app->runtime.android.buildInformation.device = android::StringWrap(env, deviceString).str();
    app->runtime.android.buildInformation.fingerprint = android::StringWrap(env, fingerprintString).str();
    app->runtime.android.buildInformation.hardware = android::StringWrap(env, hardwareString).str();
    app->runtime.android.buildInformation.model = android::StringWrap(env, modelString).str();
    app->runtime.android.buildInformation.manufacturer = android::StringWrap(env, manufacturerString).str();
    app->runtime.android.buildInformation.product = android::StringWrap(env, productString).str();
    app->runtime.android.isEmulator = (
      (
        app->runtime.android.buildInformation.brand.starts_with("generic") &&
        app->runtime.android.buildInformation.device.starts_with("generic")
      ) ||
      app->runtime.android.buildInformation.fingerprint.starts_with("generic") ||
      app->runtime.android.buildInformation.fingerprint.starts_with("unknown") ||
      app->runtime.android.buildInformation.hardware.find("goldfish") != String::npos ||
      app->runtime.android.buildInformation.hardware.find("ranchu") != String::npos ||
      app->runtime.android.buildInformation.model.find("google_sdk") != String::npos ||
      app->runtime.android.buildInformation.model.find("Emulator") != String::npos ||
      app->runtime.android.buildInformation.model.find("Android SDK built for x86") != String::npos ||
      app->runtime.android.buildInformation.manufacturer.find("Genymotion") != String::npos ||
      app->runtime.android.buildInformation.product.find("sdk_google") != String::npos ||
      app->runtime.android.buildInformation.product.find("google_sdk") != String::npos ||
      app->runtime.android.buildInformation.product.find("sdk") != String::npos ||
      app->runtime.android.buildInformation.product.find("sdk_x86") != String::npos ||
      app->runtime.android.buildInformation.product.find("sdk_gphone64_arm64") != String::npos ||
      app->runtime.android.buildInformation.product.find("vbox86p") != String::npos ||
      app->runtime.android.buildInformation.product.find("emulator") != String::npos ||
      app->runtime.android.buildInformation.product.find("simulator") != String::npos
    );
  }

  void ANDROID_EXTERNAL(app, App, setWellKnownDirectories)(
    JNIEnv *env,
    jobject self,
    jstring downloadsString,
    jstring documentsString,
    jstring picturesString,
    jstring desktopString,
    jstring videosString,
    jstring configString,
    jstring mediaString,
    jstring musicString,
    jstring homeString,
    jstring dataString,
    jstring logString,
    jstring tmpString
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    filesystem::Resource::WellKnownPaths defaults;
    defaults.downloads = Path(android::StringWrap(env, downloadsString).str());
    defaults.documents = Path(android::StringWrap(env, documentsString).str());
    defaults.pictures = Path(android::StringWrap(env, picturesString).str());
    defaults.desktop = Path(android::StringWrap(env, desktopString).str());
    defaults.videos = Path(android::StringWrap(env, videosString).str());
    defaults.config = Path(android::StringWrap(env, configString).str());
    defaults.media = Path(android::StringWrap(env, mediaString).str());
    defaults.music = Path(android::StringWrap(env, musicString).str());
    defaults.home = Path(android::StringWrap(env, homeString).str());
    defaults.data = Path(android::StringWrap(env, dataString).str());
    defaults.log = Path(android::StringWrap(env, logString).str());
    defaults.tmp = Path(android::StringWrap(env, tmpString).str());
    filesystem::Resource::WellKnownPaths::setDefaults(defaults);
  }

  jstring ANDROID_EXTERNAL(app, App, getUserConfigValue)(
    JNIEnv *env,
    jobject self,
    jstring keyString
  ) {
    const auto app = App::sharedApplication();
    const auto key = android::StringWrap(env, keyString).str();
    const auto value = env->NewStringUTF(
      app->runtime.userConfig.contains(key)
        ? app->runtime.userConfig.at(key).c_str()
        : ""
    );

    return value;
  }

  jboolean ANDROID_EXTERNAL(app, App, hasRuntimePermission)(
    JNIEnv *env,
    jobject self,
    jstring permissionString
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    const auto permission = android::StringWrap(env, permissionString).str();
    return app->runtime.hasPermission(permission);
  }

  jboolean ANDROID_EXTERNAL(app, App, isDebugEnabled)(
    JNIEnv *env,
    jobject self
  ) {
    const auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    return isDebugEnabled();
  }

  void ANDROID_EXTERNAL(app, App, onCreateAppActivity)(
    JNIEnv *env,
    jobject self,
    jobject activity
  ) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    if (app->runtime.android.activity) {
      app->runtime.android.jvm.env->DeleteGlobalRef(app->runtime.android.activity);
    }

    app->runtime.android.activity = app->runtime.android.jvm.env->NewGlobalRef(activity);
    app->runtime.android.configure(
      android::JVMEnvironment(app->runtime.android.jvm),
      app->runtime.android.activity
    );

    app->run();

    if (app->runtime.windowManager.getWindowStatus(0) == window::Manager::WINDOW_NONE) {
      auto windowManagerOptions = window::ManagerOptions {};

      for (const auto& arg : split(app->runtime.userConfig["ssc_argv"], ',')) {
        if (arg.find("--test") == 0) {
          windowManagerOptions.features.useTestScript = true;
        }

        windowManagerOptions.argv.push_back("'" + trim(arg) + "'");
      }

      windowManagerOptions.userConfig = app->runtime.userConfig;

      app->runtime.windowManager.configure(windowManagerOptions);

      app->dispatch([=]() {
        auto defaultWindow = app->runtime.windowManager.createDefaultWindow(window::Window::Options {
          .shouldExitApplicationOnClose = true
        });

        if (
          app->runtime.userConfig["webview_service_worker_mode"] != "hybrid" &&
          app->runtime.userConfig["permissions_allow_service_worker"] != "false"
        ) {
          if (app->runtime.windowManager.getWindowStatus(SOCKET_RUNTIME_SERVICE_WORKER_CONTAINER_WINDOW_INDEX) == window::Manager::WINDOW_NONE) {
            auto serviceWorkerWindowOptions = window::Window::Options {};
            auto serviceWorkerUserConfig = app->runtime.userConfig;

            serviceWorkerUserConfig["webview_watch_reload"] = "false";
            serviceWorkerWindowOptions.shouldExitApplicationOnClose = false;
            // serviceWorkerWindowOptions.index = SOCKET_RUNTIME_SERVICE_WORKER_CONTAINER_WINDOW_INDEX;
            serviceWorkerWindowOptions.headless = env::get("SOCKET_RUNTIME_SERVICE_WORKER_DEBUG").size() == 0;
            serviceWorkerWindowOptions.userConfig = serviceWorkerUserConfig;
            serviceWorkerWindowOptions.features.useGlobalCommonJS = false;
            serviceWorkerWindowOptions.features.useGlobalNodeJS = false;

            auto serviceWorkerWindow = app->runtime.windowManager.createWindow(serviceWorkerWindowOptions);
            // app->serviceWorkerContainer.init(&serviceWorkerWindow->bridge);
            // serviceWorkerWindow->navigate(
              // "https://" + app->userConfig["meta_bundle_identifier"] + "/socket/service-worker/index.html"
            // );

            app->runtime.services.timers.setTimeout(256, [=](){
              serviceWorkerWindow->hide();
            });
          }
        }

        if (
          app->runtime.userConfig["permissions_allow_service_worker"] != "false" &&
          app->runtime.userConfig["webview_service_worker_mode"] == "hybrid"
        ) {
          // app->serviceWorkerContainer.init(&defaultWindow->bridge);
        }

        defaultWindow->setTitle(app->runtime.userConfig["meta_title"]);

        static const auto port = getDevPort();
        static const auto host = getDevHost();

        if (isDebugEnabled() && port > 0 && host.size() > 0) {
          defaultWindow->navigate(host + ":" + std::to_string(port));
        } else if (app->runtime.userConfig["webview_root"].size() != 0) {
          defaultWindow->navigate(
            "https://" + app->runtime.userConfig["meta_bundle_identifier"] + app->runtime.userConfig["webview_root"]
          );
        } else {
          defaultWindow->navigate(
            "https://" + app->runtime.userConfig["meta_bundle_identifier"] + "/index.html"
          );
        }
      });
    }
  }

  void ANDROID_EXTERNAL(app, App, onDestroyAppActivity)(
    JNIEnv *env,
    jobject self,
    jobject activity
  ) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    if (app->runtime.android.activity) {
      env->DeleteGlobalRef(app->runtime.android.activity);
      app->runtime.android.activity = nullptr;
    }
  }

  void ANDROID_EXTERNAL(app, App, onDestroy)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    if (app->runtime.android.self) {
      env->DeleteGlobalRef(app->runtime.android.self);
    }

    app->runtime.android.jvm = nullptr;
    app->runtime.android.self = nullptr;

    app->pause();
  }

  void ANDROID_EXTERNAL(app, App, onStart)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->resume();
  }

  void ANDROID_EXTERNAL(app, App, onStop)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->pause();
  }

  void ANDROID_EXTERNAL(app, App, onResume)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->resume();
  }

  void ANDROID_EXTERNAL(app, App, onPause)(JNIEnv *env, jobject self) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    app->pause();
  }

  void ANDROID_EXTERNAL(app, App, onPermissionChange)(
    JNIEnv *env,
    jobject self,
    jstring nameString,
    jstring stateString
  ) {
    auto app = App::sharedApplication();
    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
    }

    const auto name = android::StringWrap(env, nameString).str();
    const auto state = android::StringWrap(env, stateString).str();

    if (name == "geolocation") {
      app->runtime.services.geolocation.permissionChangeObservers.dispatch(JSON::Object::Entries {
        {"name", name},
        {"state", state}
      });
    }

    if (name == "notification") {
      app->runtime.services.notifications.permissionChangeObservers.dispatch(JSON::Object::Entries {
        {"name", name},
        {"state", state}
      });
    }

    if (name == "camera" || name == "microphone") {
      app->runtime.services.mediaDevices.permissionChangeObservers.dispatch(JSON::Object::Entries {
        {"name", name},
        {"state", state}
      });
    }
  }
}
