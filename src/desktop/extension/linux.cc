#define SOCKET_RUNTIME_DESKTOP_EXTENSION 1
#include "../../platform/platform.hh"
#include "../../core/resource.hh"
#include "../../core/debug.hh"
#include "../../core/trace.hh"
#include "../../core/url.hh"
#include "../../app/app.hh"

#include "../extension.hh"

using namespace SSC;

#if SOCKET_RUNTIME_PLATFORM_LINUX
#if defined(__cplusplus)
extern "C" {
#endif
  static WebExtensionContext context;
  static SharedPointer<IPC::Bridge> bridge = nullptr;
  static bool isMainApplicationDebugEnabled = false;

  static void onMessageResolver (
    JSCValue* resolve,
    JSCValue* reject,
    IPC::Message* message
  ) {
    auto context = jsc_value_get_context(resolve);
    auto app = App::sharedApplication();

    if (bridge == nullptr) {
      g_object_ref(context);
      bridge = std::make_shared<IPC::Bridge>(app->core, app->userConfig);
      bridge->evaluateJavaScriptFunction = [context] (const auto source) {
        auto _ = jsc_context_evaluate(context, source.c_str(), source.size());
      };
      bridge->init();
    }

    auto routed = bridge->route(message->str(), message->buffer.bytes, message->buffer.size, [=](auto result) {
      if (result.post.body != nullptr) {
        auto array = jsc_value_new_typed_array(
          context,
          JSC_TYPED_ARRAY_UINT8,
          result.post.length
        );

        gsize size = 0;
        auto bytes = jsc_value_typed_array_get_data(array, &size);
        memcpy(bytes, result.post.body.get(), size);

        auto _ = jsc_value_function_call(
          resolve,
          JSC_TYPE_VALUE,
          array,
          G_TYPE_NONE
        );
      } else {
        auto json = result.json().str();
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

    if (routed) {
      g_object_ref(context);
      g_object_ref(resolve);
      g_object_ref(reject);
    } else {
      auto json = JSON::Object::Entries {
        {"err", JSON::Object::Entries {
          {"message", "Not found"},
          {"type", "NotFoundError"},
          {"source", message->name}
        }}
      };

      auto _ = jsc_value_function_call(
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

    if (jsc_value_is_typed_array(value)) {
      auto bytes = jsc_value_typed_array_get_data(value, &message->buffer.size);
      message->buffer.bytes = std::make_shared<char[]>(message->buffer.size);
      memcpy(
        message->buffer.bytes.get(),
        bytes,
        message->buffer.size
      );
    }

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
    auto GlobalIPCExtensionPostMessage = jsc_value_new_function(
      context,
      "GlobalIPCExtensionPostMessage",
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
      "GlobalIPCExtensionPostMessage",
      GlobalIPCExtensionPostMessage
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
        context.config.bytes = new char[context.config.size]{0};

        memcpy(
          context.config.bytes,
          g_variant_get_data(const_cast<GVariant*>(userData)),
          context.config.size
        );
      }
    }

    static App app(0);
    auto userConfig = getUserConfig();
    auto cwd = userConfig["web-process-extension_cwd"];
    if (cwd.size() > 0) {
      setcwd(cwd);
      uv_chdir(cwd.c_str());
    }
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
