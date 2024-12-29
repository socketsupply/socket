#define SOCKET_RUNTIME_DESKTOP_EXTENSION 1
#include "../../platform/platform.hh"
#include "../../runtime/resource.hh"
#include "../../runtime/debug.hh"
#include "../../runtime/trace.hh"
#include "../../runtime/url.hh"
#include "../../app/app.hh"

#include "../extension.hh"

using namespace SSC;

#if SOCKET_RUNTIME_PLATFORM_LINUX
#if defined(__cplusplus)
extern "C" {
#endif
  static SharedPointer<IPC::Bridge> sharedBridge = nullptr;
  static bool isMainApplicationDebugEnabled = false;
  static WebExtensionContext context;
  static Mutex sharedMutex;

  static SharedPointer<IPC::Bridge> getSharedBridge (JSCContext* context) {
    static auto app = App::sharedApplication();
    Lock lock(sharedMutex);

    if (sharedBridge == nullptr) {
      g_object_ref(context);
      auto options = IPC::Bridge::Options(-1, app->userConfig);
      sharedBridge = std::make_shared<IPC::Bridge>(app->runtime, options);
      sharedBridge->dispatchFunction = [](auto callback) {
        callback();
      };
      sharedBridge->evaluateJavaScriptFunction = [context] (const auto source) {
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
    IPC::Message* message
  ) {
    auto context = jsc_value_get_context(resolve);
    auto bridge = getSharedBridge(context);
    auto app = App::sharedApplication();

    auto routed = bridge->route(message->str(), message->buffer.bytes, message->buffer.size, [=](auto result) {
      app->dispatch([=] () {
        if (result.post.body != nullptr) {
          auto array = jsc_value_new_typed_array(
            context,
            JSC_TYPED_ARRAY_UINT8,
            result.post.length
          );

          gsize size = 0;
          auto bytes = jsc_value_typed_array_get_data(array, &size);
          memcpy(bytes, result.post.body.get(), size);

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
    auto message = new IPC::Message(source);

    if (jsc_value_is_typed_array(value)) {
      auto bytes = jsc_value_typed_array_get_data(value, &message->buffer.size);
      message->buffer.bytes = std::make_shared<char[]>(message->buffer.size);
      memcpy(
        message->buffer.bytes.get(),
        bytes,
        message->buffer.size
      );
    }

    if (message->get("__sync__") == "true") {
      auto bridge = getSharedBridge(context);
      auto app = App::sharedApplication();
      auto semaphore = new BinarySemaphore(0);

      IPC::Result returnResult;

      const auto routed = bridge->route(
        message->str(),
        message->buffer.bytes,
        message->buffer.size,
        [&returnResult, &semaphore] (auto result) mutable {
          returnResult = std::move(result);
          semaphore->release();
        }
      );

      semaphore->acquire();

      delete semaphore;
      delete message;

      if (routed) {
        if (returnResult.post.body != nullptr) {
          auto array = jsc_value_new_typed_array(
            context,
            JSC_TYPED_ARRAY_UINT8,
            returnResult.post.length
          );

          gsize size = 0;
          auto bytes = jsc_value_typed_array_get_data(array, &size);
          memcpy(bytes, returnResult.post.body.get(), size);
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

    if (!context.config.bytes) {
      context.config.size = g_variant_get_size(const_cast<GVariant*>(userData));
      if (context.config.size) {
        context.config.bytes = reinterpret_cast<char*>(new unsigned char[context.config.size]{0});

        memcpy(
          context.config.bytes,
          g_variant_get_data(const_cast<GVariant*>(userData)),
          context.config.size
        );
      }
    }

    Runtime::Options options;
    options.dedicatedLoopThread = true;

    options.features.usePlatform = true;
    options.features.useTimers = true;
    options.features.useFS = true;

    options.features.useNotifications = false;
    options.features.useNetworkStatus = false;
    options.features.usePermissions = false;
    options.features.useGeolocation = false;
    options.features.useConduit = false;
    options.features.useUDP = false;
    options.features.useDNS = false;
    options.features.useAI = false;

    auto userConfig = getUserConfig();
    auto cwd = userConfig["web-process-extension_cwd"];

    if (cwd.size() > 0) {
      setcwd(cwd);
      uv_chdir(cwd.c_str());
    }

    static App app(
      App::DEFAULT_INSTANCE_ID,
      std::move(std::make_shared<Runtime>(options))
    );
  }

  const unsigned char* socket_runtime_init_get_user_config_bytes () {
    return reinterpret_cast<const unsigned char*>(context.config.bytes);
  }

  unsigned int socket_runtime_init_get_user_config_bytes_size () {
    return context.config.size;
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
