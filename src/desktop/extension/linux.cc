#define SOCKET_RUNTIME_DESKTOP_EXTENSION 1

#include "../../runtime.hh"
#include "../../app.hh"

#include "../extension.hh"

using namespace ssc;
using namespace ssc::desktop;
using namespace ssc::runtime::types;

namespace JSON = ssc::runtime::JSON;
namespace ipc = ssc::runtime::ipc;

using ssc::runtime::config::getUserConfig;
using ssc::runtime::bridge::Bridge;
using ssc::runtime::Runtime;
using ssc::runtime::setcwd;
using ssc::app::App;

#if SOCKET_RUNTIME_PLATFORM_LINUX
#if defined(__cplusplus)
extern "C" {
#endif
  static bool isMainApplicationDebugEnabled = false; // TODO(@jwerle): handle how this value is configured

  static SharedPointer<Bridge> sharedBridge = nullptr;
  static WebExtensionContext sharedContext;
  static Mutex sharedMutex;

  static SharedPointer<Bridge> getSharedBridge (JSCContext* context) {
    static auto app = App::sharedApplication();
    Lock lock(sharedMutex);

    if (sharedBridge == nullptr) {
      g_object_ref(context);
      sharedBridge = app->runtime.bridgeManager.get(0, {});
      sharedBridge->userConfig = getUserConfig();
      sharedBridge->dispatchHandler = [](auto callback) { callback(); };
      sharedBridge->evaluateJavaScriptHandler = [context] (const auto source) {
        app->dispatch([=] () {
          auto _ = jsc_context_evaluate(context, source.c_str(), source.size());
        });
      };
      sharedBridge->init();
    }

    return sharedBridge;
  }

  static void onMessageResolver (
    JSCValue* resolve,
    JSCValue* reject,
    runtime::ipc::Message* message
  ) {
    auto context = jsc_value_get_context(resolve);
    auto bridge = getSharedBridge(context);
    auto app = App::sharedApplication();

    auto routed = bridge->route(message->str(), message->buffer, [=](auto result) {
      app->dispatch([=] () {
        if (result.queuedResponse.body != nullptr) {
          auto array = jsc_value_new_typed_array(
            context,
            JSC_TYPED_ARRAY_UINT8,
            result.queuedResponse.length
          );

          gsize size = 0;
          auto bytes = jsc_value_typed_array_get_data(array, &size);
          memcpy(bytes, result.queuedResponse.body.get(), size);

          const auto _ = jsc_value_function_call(
            resolve,
            JSC_TYPE_VALUE,
            array,
            G_TYPE_NONE
          );
        } else {
          const auto json = result.json().str();
          if (json.size() > 0) {
            auto _ = jsc_value_function_call(
              resolve,
              JSC_TYPE_VALUE,
              jsc_value_new_string(context, json.c_str()),
              G_TYPE_NONE
            );
          }
        }

        g_object_unref(context);
        g_object_unref(resolve);
        g_object_unref(reject);
      });
    });

    if (routed) {
      g_object_ref(context);
      g_object_ref(resolve);
      g_object_ref(reject);
    } else {
      const auto json = JSON::Object::Entries {
        {"err", JSON::Object::Entries {
          {"message", "Not found"},
          {"type", "NotFoundError"},
          {"source", message->name}
        }}
      };

      const auto _ = jsc_value_function_call(
        resolve,
        JSC_TYPE_VALUE,
        jsc_value_new_string(context, JSON::Object(json).str().c_str()),
        G_TYPE_NONE
      );
    }

    delete message;
  }

  static JSCValue* onMessage (const char* source, JSCValue* value, gpointer userData) {
    auto context = reinterpret_cast<JSCContext*>(userData);
    auto Promise = jsc_context_get_value(context, "Promise");
    auto message = new ipc::Message(source);

    if (jsc_value_is_typed_array(value)) {
      size_t size = 0;
      auto bytes = jsc_value_typed_array_get_data(value, &size);
      message->buffer.set(reinterpret_cast<const char*>(bytes), 0, size);
    }

    if (message->get("__sync__") == "true") {
      auto bridge = getSharedBridge(context);
      auto app = App::sharedApplication();
      auto semaphore = new BinarySemaphore(0);

      ipc::Result returnResult;

      const auto routed = bridge->route(
        message->str(),
        message->buffer,
        [&returnResult, &semaphore] (auto result) mutable {
          returnResult = std::move(result);
          semaphore->release();
        }
      );

      semaphore->acquire();

      delete semaphore;
      delete message;

      if (routed) {
        if (returnResult.queuedResponse.body != nullptr) {
          auto array = jsc_value_new_typed_array(
            context,
            JSC_TYPED_ARRAY_UINT8,
            returnResult.queuedResponse.length
          );

          gsize size = 0;
          auto bytes = jsc_value_typed_array_get_data(array, &size);
          memcpy(bytes, returnResult.queuedResponse.body.get(), size);
          return array;
        } else {
          auto json = returnResult.json().str();
          if (json.size() > 0) {
            return jsc_value_new_string(context, json.c_str());
          }
        }
      }

      const auto json = JSON::Object::Entries {
        {"err", JSON::Object::Entries {
          {"message", "Not found"},
          {"type", "NotFoundError"},
          {"source", source}
        }}
      };

      return jsc_value_new_string(context, JSON::Object(json).str().c_str());
    }

    auto resolver = jsc_value_new_function(
      context,
      nullptr,
      G_CALLBACK(onMessageResolver),
      message,
      nullptr,
      G_TYPE_NONE,
      2,
      JSC_TYPE_VALUE,
      JSC_TYPE_VALUE
    );

    auto promise = jsc_value_constructor_call(
      Promise,
      JSC_TYPE_VALUE,
      resolver,
      G_TYPE_NONE
    );

    g_object_unref(Promise);
    g_object_unref(resolver);
    return promise;
  }

  static void onDocumentLoaded (
    WebKitWebPage* page,
    gpointer userData
  ) {
    auto frame = webkit_web_page_get_main_frame(page);
    auto context = webkit_frame_get_js_context(frame);
    auto __global_ipc_extension_handler = jsc_value_new_function(
      context,
      "__global_ipc_extension_handler",
      G_CALLBACK(onMessage),
      context,
      nullptr,
      JSC_TYPE_VALUE,
      2,
      G_TYPE_STRING,
      JSC_TYPE_VALUE
    );

    jsc_context_set_value(
      context,
      "__global_ipc_extension_handler",
      __global_ipc_extension_handler
    );
  }

  static void onPageCreated (
    WebKitWebExtension* extension,
    WebKitWebPage* page,
    gpointer userData
  ) {
    auto userConfig = getUserConfig();
    g_signal_connect(
      page,
      "document-loaded",
      G_CALLBACK(onDocumentLoaded),
      nullptr
    );
  }

  G_MODULE_EXPORT void webkit_web_extension_initialize_with_user_data (
    WebKitWebExtension* extension,
    const GVariant* userData
  ) {
    g_signal_connect(
      extension,
      "page-created",
      G_CALLBACK(onPageCreated),
      nullptr
    );

    if (!sharedContext.config.bytes) {
      sharedContext.config.size = g_variant_get_size(const_cast<GVariant*>(userData));
      if (sharedContext.config.size) {
        sharedContext.config.bytes = reinterpret_cast<char*>(new unsigned char[sharedContext.config.size]{0});

        memcpy(
          sharedContext.config.bytes,
          g_variant_get_data(const_cast<GVariant*>(userData)),
          sharedContext.config.size
        );
      }
    }

    Runtime::Features features;
    features.usePlatform = true;
    features.useTimers = true;
    features.useFS = true;

    features.useNotifications = false;
    features.useNetworkStatus = false;
    features.usePermissions = false;
    features.useGeolocation = false;
    features.useConduit = false;
    features.useUDP = false;
    features.useDNS = false;
    features.useAI = false;

    auto userConfig = getUserConfig();
    auto cwd = userConfig["web-process-extension_cwd"];

    if (cwd.size() > 0) {
      setcwd(cwd);
      uv_chdir(cwd.c_str());
    }

    static App app(App::Options {
      .userConfig = userConfig,
      .loop = { .dedicatedThread = true },
      .features = features
    });

    app.run();
  }

  const unsigned char* socket_runtime_init_get_user_config_bytes () {
    return reinterpret_cast<const unsigned char*>(sharedContext.config.bytes);
  }

  unsigned int socket_runtime_init_get_user_config_bytes_size () {
    return sharedContext.config.size;
  }

  bool socket_runtime_init_is_debug_enabled () {
    return isMainApplicationDebugEnabled;
  }

  const char* socket_runtime_init_get_dev_host () {
    auto userConfig = getUserConfig();
    if (userConfig.contains("web-process-extension_host")) {
      return userConfig["web-process-extension_host"].c_str();
    }
    return "";
  }

  int socket_runtime_init_get_dev_port () {
    auto userConfig = getUserConfig();
    if (userConfig.contains("web-process-extension_port")) {
      try {
        return std::stoi(userConfig["web-process-extension_port"]);;
      } catch (...) {}
    }

    return 0;
  }
#if defined(__cplusplus)
}
#endif
#endif
