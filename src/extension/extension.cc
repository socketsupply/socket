#include "extension.hh"

namespace SSC {
  static Extension::Map extensions = {};
  static Mutex mutex;

  static String getcwd () {
    String cwd;
  #if defined(__linux__) && !defined(__ANDROID__)
    auto canonical = fs::canonical("/proc/self/exe");
    cwd = fs::path(canonical).parent_path().string();
  #elif defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    auto fileManager = [NSFileManager defaultManager];
    auto currentDirectory = [fileManager currentDirectoryPath];
    cwd = String([currentDirectory UTF8String]);
  #elif defined(__APPLE__)
    NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
    cwd = String([[resourcePath stringByAppendingPathComponent: @"ui"] UTF8String]);

  #elif defined(_WIN32)
    wchar_t filename[MAX_PATH];
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    auto path = fs::path { filename }.remove_filename();
    cwd = path.string();
    size_t last_pos = 0;
    while ((last_pos = cwd.find('\\', last_pos)) != std::string::npos) {
      cwd.replace(last_pos, 1, "\\\\");
      last_pos += 2;
    }
  #endif

  #ifndef _WIN32
    std::replace(cwd.begin(), cwd.end(), '\\', '/');
  #endif

    return cwd;
  }

  // explicit template instantiations
  template char* SSC::Extension::Context::Memory::alloc<char> (size_t);
  template sapi_context_t*
  SSC::Extension::Context::Memory::alloc<sapi_context_t> ();

  template SSC::IPC::Router::ReplyCallback*
  SSC::Extension::Context::Memory::alloc<SSC::IPC::Router::ReplyCallback> (
    SSC::IPC::Router::ReplyCallback
  );

  template sapi_ipc_result_t*
  SSC::Extension::Context::Memory::alloc<sapi_ipc_result_t> (sapi_context_t*);

  template sapi_process_exec_t*
  SSC::Extension::Context::Memory::alloc<sapi_process_exec_t> (
    SSC::ExecOutput&
  );

  template sapi_json_object_t*
  SSC::Extension::Context::Memory::alloc<sapi_json_object_t> (
    sapi_context_t*
  );

  template sapi_json_array_t*
  SSC::Extension::Context::Memory::alloc<sapi_json_array_t> (
    sapi_context_t*
  );

  template sapi_json_string_t*
  SSC::Extension::Context::Memory::alloc<sapi_json_string_t> (
    sapi_context_t*,
    const char*
  );

  template sapi_json_boolean_t*
  SSC::Extension::Context::Memory::alloc<sapi_json_boolean_t> (
    sapi_context_t*,
    const bool
  );

  template sapi_json_number_t*
  SSC::Extension::Context::Memory::alloc<sapi_json_number_t> (
    sapi_context_t*,
    int64_t
  );

  Extension::Context::Context (const Extension* extension) {
    this->extension = extension;
    this->router = extension->context.router;
    this->config = extension->context.config;
    this->data = extension->context.data;
  }

  Extension::Context::Context (const Context& context) {
    this->extension = context.extension;
    this->router = context.router;
    this->config = context.config;
    this->data = context.data;
  }

  Extension::Context::Context (const Context* context) {
    this->extension = context->extension;
    this->router = context->router;
    this->config = context->config;
    this->data = context->data;
  }

  Extension::Context::Context (IPC::Router* router) : Context() {
    this->router = router;
  }

  void Extension::Context::retain () {
    this->retained = true;
  }

  void Extension::Context::release () {
    this->retained = false;
    this->memory.release();
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
    auto cwd = getcwd();
  #if defined(_WIN32)
    return cwd + "\\socket\\extensions\\" + name + "\\";
  #else
    return cwd + "/socket/extensions/" + name + "/";
  #endif
  }

  void Extension::Context::Memory::push (std::function<void()> callback) {
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

  bool Extension::isLoaded (const String& name) {
    Lock lock(mutex);
    return extensions.contains(name) && extensions.at(name) != nullptr;
  }

  bool Extension::load (const String& name) {
    Lock lock(mutex);

    static auto userConfig = SSC::getUserConfig();

  #if defined(_WIN32)
    // TODO LoadLibrary, GetProcAddress, CloseLibrary
    return false;
  #else
    // check if extension is already known
    if (isLoaded(name)) return true;

    auto path = getExtensionsDirectory(name) + (name + ".so");
    #if defined(__ANDROID__)
    auto handle = dlopen(String("libextension-" + name + ".so").c_str(), RTLD_NOW | RTLD_LOCAL);
    #else
    auto handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    #endif

    if (handle == nullptr) return false;

    auto __sapi_extension_init = (sapi_extension_registration_entry) dlsym(handle, "__sapi_extension_init");
    if (!__sapi_extension_init) return false;

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
        return sapi_extension_register(registration);
      }
    }
  #endif
    return false;
  }

  bool Extension::unload (const String& name) {
    Lock lock(mutex);

    if (Extension::isLoaded(name)) {
      auto extension = Extension::get(name);
      auto ctx = const_cast<Context*>(&extension->context);

      if (extension->deinitializer(ctx, ctx->data)) {
        return true;
      }
    }

    if (extensions.contains(name)) {
      extensions.erase(name);
    }

    return false;
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

    auto extension = std::shared_ptr<Extension>(new Extension(name, initializer));
    extensions.insert_or_assign(name, extension);
  }

  bool Extension::initialize (
    const Context* ctx,
    const String& name,
    const void* data
  ) {
    Lock lock(mutex);
    // return early if not exists
    if (!isLoaded(name)) {
      return false;
    }

    auto extension = extensions.at(name);
    if (extension != nullptr && extension->initializer != nullptr) {
      debug("Initializing loaded extension: %s", name.c_str());
      extension->context.router = ctx->router;
      extension->context.data = data;
      return extension->initializer(&extension->context, data);
    }

    return false;
  }
}

bool sapi_extension_register (
  const sapi_extension_registration_t* registration
) {
  SSC::Lock lock(SSC::mutex);

  if (registration == nullptr) return false;
  if (registration->abi == 0) return false;
  if (registration->initializer == nullptr) return false;

  SSC::Extension::create(registration->name, [registration](
    auto ctx,
    auto data
  ) mutable {
    return registration->initializer(reinterpret_cast<sapi_context_t*>(ctx), data);
  });

  // should exist if `create()` above succeeded
  if (SSC::Extension::isLoaded(registration->name)) {
    auto extension = SSC::extensions.at(registration->name);

    extension->abi = registration->abi;
    extension->deinitializer = [registration] (auto ctx, auto data) {
      auto deinitializer = registration->deinitializer;
      if (deinitializer != nullptr) {
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

    return true;
  }

  return false;
}

const sapi_extension_registration_t* sapi_extension_get (
  const sapi_context_t* context,
  const char *name
) {
  if (!SSC::Extension::isLoaded(name)) {
    return nullptr;
  }

  auto pointer = SSC::Extension::get(name).get();
  return reinterpret_cast<const sapi_extension_registration_t*>(pointer);
}

bool sapi_extension_load (
    sapi_context_t* context,
    const char *name,
    const void* data
    ) {
  if (!SSC::Extension::load(name)) {
    return false;
  }

  if (!SSC::Extension::initialize(context, name, data)) {
    return false;
  }

  return true;
}

bool sapi_extension_unload (sapi_context_t* context, const char *name) {
  if (SSC::Extension::isLoaded(name)) {
    return false;
  }

  if (!SSC::Extension::unload(name)) {
    return false;
  }

  return true;
}

void sapi_log (const sapi_context_t* ctx, const char* message) {
  if (message == nullptr) return;

  SSC::String output;

  if (ctx && ctx->extension && ctx->extension->name.size() > 0) {
    auto extension = ctx->extension;
    output = "[" + extension->name + "] " + message;
  } else {
    output = message;
  }

#if defined(__linux__) && defined(__ANDROID__)
  __android_log_print(ANDROID_LOG_INFO, "Console", "%s", message);
#else
  SSC::stdWrite(output, false);
#endif

#if defined(__APPLE__)
  static auto userConfig = SSC::getUserConfig();
  static auto bundleIdentifier = userConfig["meta_bundle_identifier"];
  static auto SSC_OS_LOG_BUNDLE = os_log_create(bundleIdentifier.c_str(),
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      "socket.runtime.mobile"
#else
      "socket.runtime.desktop"
#endif
      );
  os_log_with_type(SSC_OS_LOG_BUNDLE, OS_LOG_TYPE_INFO, "%{public}s", output.c_str());
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
  return SSC::rand64();
}
