#include "../runtime/crypto.hh"
#include "../runtime/cwd.hh"
#include "../runtime/io.hh"

#include "extension.hh"

using ssc::runtime::config::getUserConfig;
using ssc::runtime::string::replace;
using ssc::runtime::string::split;
using ssc::runtime::string::trim;

namespace ssc::extension {
  static Extension::Map extensions = {};
  static Vector<String> initializedExtensions;
  static Mutex mutex;

  // explicit template instantiations
  template char* ssc::extension::Extension::Context::Memory::alloc<char> (size_t);
  template sapi_context_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_context_t> ();

  template ssc::runtime::ipc::Router::ReplyCallback*
  ssc::extension::Extension::Context::Memory::alloc<ssc::runtime::ipc::Router::ReplyCallback> (
    ssc::runtime::ipc::Router::ReplyCallback
  );

  template sapi_ipc_result_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_ipc_result_t> (sapi_context_t*);

  template sapi_process_exec_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_process_exec_t> (
    ssc::runtime::process::ExecOutput&
  );

  template sapi_json_object_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_json_object_t> (
    sapi_context_t*
  );

  template sapi_json_array_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_json_array_t> (
    sapi_context_t*
  );

  template sapi_json_string_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_json_string_t> (
    sapi_context_t*,
    const char*
  );

  template sapi_json_boolean_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_json_boolean_t> (
    sapi_context_t*,
    bool
  );

  template sapi_json_number_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_json_number_t> (
    sapi_context_t*,
    int64_t
  );

  template sapi_json_raw_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_json_raw_t> (
    sapi_context_t*,
    const char*
  );

  template sapi_ipc_message_t*
  ssc::extension::Extension::Context::Memory::alloc<sapi_ipc_message_t> (
    sapi_ipc_message_t
  );

  Extension::Context::Context (const Extension* extension) {
    this->extension = extension;
    this->router = extension->context.router;
    this->config = extension->context.config;
    this->data = extension->context.data;
    this->policies = extension->context.policies;
  }

  Extension::Context::Context (const Context& context) {
    this->extension = context.extension;
    this->router = context.router;
    this->config = context.config;
    this->data = context.data;
    this->policies = context.policies;
    this->internal = context.internal;
  }

  Extension::Context::Context (const Context* context) {
    if (context != nullptr) {
      this->extension = context->extension;
      this->router = context->router;
      this->config = context->config;
      this->data = context->data;
      this->policies = context->policies;
      this->internal = context->internal;
    }
  }

  Extension::Context::Context (
    const Context& context,
    ipc::Router* router
  ) : Context(router) {
      this->extension = context.extension;
      this->router = router;
      this->config = context.config;
      this->data = context.data;
      this->policies = context.policies;
      this->internal = context.internal;
    }

  Extension::Context::Context (ipc::Router* router) : Context() {
    this->router = router;
  }

  void Extension::Context::retain () {
    this->retain_count++;
  }

  bool Extension::Context::release () {
    if (this->retain_count == 0) {
      debug("WARN - Double release of runtime extension context");
      return false;
    }
    if (--this->retain_count == 0) {
      this->memory.release();
      return true;
    }
    return false;
  }

  Extension::Context* Extension::getContext (const String& name) {
    return &extensions.at(name)->context;
  }

  void Extension::Context::setPolicy (const String& name, bool allowed) {
    if (name.size() == 0) return;
    if (this->hasPolicy(name)) {
      auto policy = this->getPolicy(name);
      policy.allowed = allowed;
      policy.name = name;
    } else {
      this->policies.insert_or_assign(name, Policy { name, allowed });
    }
  }

  const Extension::Context::Policy& Extension::Context::getPolicy (
    const String& name
  ) const {
    return this->policies.at(name);
  }

  bool Extension::Context::hasPolicy (const String& name) const {
    return this->policies.contains(name);
  }

  bool Extension::Context::isAllowed (const String& name) const {
    if (this->policies.size() == 0) return true;
    auto names = split(name, ',');

    // parse each comma (',') separated policy
    for (const auto& value: names) {
      auto parts = split(trim(value), '_');
      String current = "";
      // try each part of the policy (ipc, ipc_router, ipc_router_map)
      for (const auto& part : parts) {
        current += part;
        if (this->hasPolicy(current) && this->getPolicy(current).allowed) {
          return true;
        }

        current += "_";
      }
    }

    return false;
  }

  Extension::Context::Memory::~Memory () {
    this->release();
  }

  void Extension::Context::Memory::release () {
    Lock lock(this->mutex);
    if (this->pool.size() > 0) {
      for (const auto& releaseCallback: this->pool) {
        releaseCallback();
      }

      this->pool.clear();
    }
  }

  String Extension::getExtensionsDirectory (const String& name) {
    auto cwd = runtime::getcwd();
  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    return cwd + "\\socket\\extensions\\" + name + "\\";
  #else
    return cwd + "/socket/extensions/" + name + "/";
  #endif
  }

  void Extension::Context::Memory::push (Function<void()> callback) {
    Lock lock(this->mutex);
    this->pool.push_back(callback);
  }

  Extension::Extension (const String& name, const Initializer initializer)
    : name(name), initializer(initializer)
  {
    static auto userConfig = getUserConfig();
    this->context.extension = this;
    for (const auto& tuple : userConfig) {
      auto& key = tuple.first;
      auto& value = tuple.second;
      auto prefix = "extensions_" + name + "_";
      if (key.starts_with(prefix)) {
        auto name = replace(key, prefix, "");
        this->context.config[name] = value;
      }
    }
  }

  Extension::Extension (Extension& extension) {
    this->name = extension.name;
    this->context.config = extension.context.config;
    this->initializer = extension.initializer;
  }

  const Extension::Map& Extension::all () {
    return extensions;
  }

  const Extension::Entry Extension::get (const String& name) {
    Lock lock(mutex);
    return extensions.at(name);
  }

  bool Extension::setHandle (const String& name, void* handle) {
    if (!extensions.contains(name)) {
      runtime::io::write("WARN - extensions does not contain " + name);
      return false;
    }

    runtime::io::write("Registering extension handle " + name);
    extensions.at(name)->handle = handle;
    return true;
  }

  void Extension::setRouterContext (
    const String& name,
    ipc::Router* router,
    Context* context
  ) {
    if (!extensions.contains(name)) return;
    auto extension = extensions.at(name);
    extension->contexts[router] = context;
  }

  Extension::Context* Extension::getRouterContext (
    const String& name,
    ipc::Router* router
  ) {
    if (!extensions.contains(name)) return nullptr;
    return extensions.at(name)->contexts[router];
  }

  void Extension::removeRouterContext (
    const String& name,
    ipc::Router* router
  ) {
    if (!extensions.contains(name)) return;
    extensions.at(name)->contexts.erase(router);
  }

  bool Extension::isLoaded (const String& name) {
    Lock lock(mutex);
    return extensions.contains(name) && extensions.at(name) != nullptr;
  }

  String Extension::getExtensionType (const String& name) {
    const auto libraryPath = getExtensionsDirectory(name) + (name + SOCKET_RUNTIME_EXTENSION_FILENAME_EXTNAME);
    const auto wasmPath = getExtensionsDirectory(name) + (name + ".wasm");
    if (fs::exists(wasmPath)) {
      return "wasm32";
    }

    if (fs::exists(libraryPath)) {
      return "shared";
    }

    return "unknown";
  }

  String Extension::getExtensionPath (const String& name) {
    const auto type = getExtensionType(name);
    if (type == "wasm32") {
      return getExtensionsDirectory(name) + (name + ".wasm");
    }

    if (type == "shared") {
      return getExtensionsDirectory(name) + (name + SOCKET_RUNTIME_EXTENSION_FILENAME_EXTNAME);
    }

    return "";
  }

  bool Extension::load (const String& name) {
    Lock lock(mutex);

    static auto userConfig = getUserConfig();
    // check if extension is already known
    if (isLoaded(name)) return true;

    auto path = getExtensionsDirectory(name) + (name + SOCKET_RUNTIME_EXTENSION_FILENAME_EXTNAME);

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    auto handle = LoadLibrary(path.c_str());
    if (handle == nullptr) return false;
    auto __sapi_extension_init = (sapi_extension_registration_entry) GetProcAddress(handle, "__sapi_extension_init");
    if (!__sapi_extension_init) return false;
  #else
  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    auto handle = dlopen(String("libextension-" + name + SOCKET_RUNTIME_EXTENSION_FILENAME_EXTNAME).c_str(), RTLD_NOW | RTLD_LOCAL);
  #else
    auto handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  #endif

    if (handle == nullptr) return false;

    auto __sapi_extension_init = (sapi_extension_registration_entry) dlsym(handle, "__sapi_extension_init");
    if (!__sapi_extension_init) return false;
  #endif

    if (__sapi_extension_init != nullptr) {
      sapi_extension_registration_t* registration = __sapi_extension_init();
      if (registration != nullptr) {
        auto abi = registration->abi;
        if (abi != SOCKET_RUNTIME_EXTENSION_ABI_VERSION) {
          if (userConfig["build_extensions_abi_strict"] != "false") {
            debug(
              "Failed to load extension: %s - Extension ABI %lu does not match runtime ABI %lu",
              registration->name,
              abi,
              (unsigned long) SOCKET_RUNTIME_EXTENSION_ABI_VERSION
            );
            return false;
          } else if (abi < SOCKET_RUNTIME_EXTENSION_ABI_VERSION) {
            debug(
              "WARNING: Extension %s ABI %lu does not match runtime ABI %lu - trying to load anyways.\n"
              "Remove `[build.extensions.abi] strict = false' to disable this",
              registration->name,
              abi,
              (unsigned long) SOCKET_RUNTIME_EXTENSION_ABI_VERSION
            );
          } else {
            debug(
              "Refusing to load extension: %s - "
              "Extension ABI %lu is incompatible with runtime ABI %lu.\n"
              "Rebuild the extension to fix this",
              registration->name,
              abi,
              (unsigned long) SOCKET_RUNTIME_EXTENSION_ABI_VERSION
            );
            return false;
          }
        }

        debug("Registering loaded extension: %s", registration->name);
        if (sapi_extension_register(registration)) {
          return Extension::setHandle(name, reinterpret_cast<void*>(handle));
        }
      }
    }

    return false;
  }

  bool Extension::unload (Context* ctx, const String& name, bool shutdown) {
    Lock lock(mutex);

    if (!Extension::isLoaded(name)) {
      return false;
    }

    auto extension = Extension::get(name);

    if (!extension->deinitializer(ctx, ctx->data)) {
      return false;
    }

    if (!shutdown) {
      return true;
    }

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    if (!FreeLibrary(reinterpret_cast<HMODULE>(extension->handle))) {
      return false;
    }
  #else
    if (dlclose(extension->handle)) {
      return false;
    }
  #endif

    if (extensions.contains(name)) {
      extensions.erase(name);
    }

    return true;
  }

  void Extension::create (
    const String& name,
    const Initializer initializer
  ) {
    Lock lock(mutex);
    // return early if exists
    if (isLoaded(name)) {
      return;
    }

    auto extension = std::make_shared<Extension>(name, initializer);
    extensions.insert_or_assign(name, extension);
  }

  bool Extension::initialize (
    Context* ctx,
    const String& name,
    const void* data
  ) {
    Lock lock(mutex);
    // return early if not exists
    if (!isLoaded(name)) {
      return false;
    }

    if (std::count(initializedExtensions.begin(), initializedExtensions.end(), name) > 0) {
      debug("Extension '%s' already loaded", name.c_str());
      // already initialized
      return true;
    }

    auto extension = extensions.at(name);
    if (extension != nullptr && extension->initializer != nullptr) {
      debug("Initializing loaded extension: %s", name.c_str());
      extension->context.data = data;
      auto didInitialize = extension->initializer(ctx, data);
      if (didInitialize) {
        initializedExtensions.push_back(name);
      }
      return didInitialize;
    }

    return false;
  }
}

bool sapi_extension_register (
  const sapi_extension_registration_t* registration
) {
  ssc::runtime::Lock lock(ssc::extension::mutex);

  if (registration == nullptr) return false;
  if (registration->abi == 0) return false;
  if (registration->initializer == nullptr) return false;

  ssc::extension::Extension::create(registration->name, [registration](
    auto ctx,
    auto data
  ) mutable {
    return registration->initializer(reinterpret_cast<sapi_context_t*>(ctx), data);
  });

  // should exist if `create()` above succeeded
  if (ssc::extension::Extension::isLoaded(registration->name)) {
    auto extension = ssc::extension::extensions.at(registration->name);

    extension->abi = registration->abi;
    extension->deinitializer = [registration] (auto ctx, auto data) {
      auto deinitializer = registration->deinitializer;
      if (deinitializer != nullptr) {
        debug("Unloading extension: %s", registration->name);
        auto initializedExtensionsCursor = std::find(
          ssc::extension::initializedExtensions.begin(),
          ssc::extension::initializedExtensions.end(),
          ssc::extension::String(registration->name)
         );

        if (initializedExtensionsCursor != ssc::extension::initializedExtensions.end()) {
          ssc::extension::initializedExtensions.erase(initializedExtensionsCursor);
        }

        return deinitializer(reinterpret_cast<sapi_context_t*>(ctx), data);
      }

      return true;
    };

    if (registration->description != nullptr) {
      extension->description = registration->description;
    }

    if (registration->version != nullptr) {
      extension->version = registration->version;
    }

    extension->registration = registration;

    return true;
  }

  return false;
}

bool sapi_extension_is_allowed (sapi_context_t* context, const char *allowed) {
  if (context == nullptr) return false;
  if (allowed == nullptr) return false;
  return context->isAllowed(allowed);
}

const sapi_extension_registration_t* sapi_extension_get (
  const sapi_context_t* context,
  const char *name
) {
  if (!ssc::extension::Extension::isLoaded(name)) {
    return nullptr;
  }

  auto extension = ssc::extension::Extension::get(name).get();
  if (extension) {
    return reinterpret_cast<const sapi_extension_registration_t*>(
      extension->registration
    );
  }

  return nullptr;
}

bool sapi_extension_load (
    sapi_context_t* context,
    const char *name,
    const void* data
    ) {
  if (!ssc::extension::Extension::load(name)) {
    return false;
  }

  if (!ssc::extension::Extension::initialize(context, name, data)) {
    return false;
  }

  return true;
}

bool sapi_extension_unload (sapi_context_t* context, const char *name) {
  if (ssc::extension::Extension::isLoaded(name)) {
    return false;
  }

  if (!ssc::extension::Extension::unload(context, name, true)) {
    return false;
  }

  return true;
}

void sapi_log (const sapi_context_t* ctx, const char* message) {
  if (message == nullptr) return;

  ssc::runtime::String output;

  if (ctx && ctx->extension && ctx->extension->name.size() > 0) {
    auto extension = ctx->extension;
    output = "[" + extension->name + "] " + message;
  } else {
    output = message;
  }

  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    __android_log_print(ANDROID_LOG_INFO, "Console", "%s", message);
  #else
    ssc::runtime::io::write(output, false);
  #endif

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    static auto userConfig = ssc::runtime::config::getUserConfig();
    static auto bundleIdentifier = userConfig["meta_bundle_identifier"];
    static auto SOCKET_RUNTIME_OS_LOG_INFO = os_log_create(
      bundleIdentifier.c_str(),
      "socket.runtime"
    );
    os_log_with_type(SOCKET_RUNTIME_OS_LOG_INFO, OS_LOG_TYPE_INFO, "%{public}s", output.c_str());
  #endif
}

void sapi_debug (const sapi_context_t* ctx, const char* message) {
  if (message == nullptr) return;
  if (ctx && ctx->extension && ctx->extension->name.size() > 0) {
    auto extension = ctx->extension;
    debug("[%s] %s", extension->name.c_str(), message);
  } else {
    debug("%s", message);
  }
}

uint64_t sapi_rand64 () {
  return ssc::runtime::crypto::rand64();
}
