#include "android.hh"
#include "uv.h"

// clang-format off

#pragma NativeString

NativeString::NativeString (JNIEnv *env) {
  this->env = env;
  this->ref = 0;
  this->length = 0;
  this->string = 0;
  this->needsRelease = false;
}

NativeString::NativeString (const NativeString &copy)
  : env(copy.env)
  , ref(copy.ref)
  , length(copy.length)
  , string(copy.string)
  , needsRelease(false)
{
  // noop copy constructor
}

NativeString::NativeString (JNIEnv *env, jstring ref)
  : NativeString(env)
{
  if (ref) {
    this->Set(ref);
  }
}

NativeString::NativeString (JNIEnv *env, std::string string)
  : NativeString(env)
{
  if (string.size() > 0) {
    this->Set(string);
  }
}

NativeString::NativeString (JNIEnv *env, const char *string)
  : NativeString(env)
{
  if (string) {
    this->Set(string);
  }
}

NativeString::~NativeString () {
  this->Release();
}

void NativeString::Set (std::string string) {
  this->Set(string.c_str());
}

void NativeString::Set (const char *string) {
  this->Set(this->env->NewStringUTF(string));
}

void NativeString::Set (jstring ref) {
  if (ref) {
    this->ref = ref;
    this->string = this->env->GetStringUTFChars(ref, &this->needsRelease);
    this->length = this->env->GetStringUTFLength(this->ref);
  }
}

void NativeString::Release () {
  if (this->ref && this->string && this->needsRelease) {
    this->env->ReleaseStringUTFChars(this->ref, this->string);
  }

  this->ref = 0;
  this->length = 0;
  this->string = 0;
  this->needsRelease = false;
}

const char * NativeString::c_str () {
  return this->string;
}

const std::string NativeString::str () {
  std::string value;

  if (this->string) {
    value.assign(this->string);
  }

  return value;
}

const jstring NativeString::jstr () {
  return this->ref;
}

const size_t NativeString::size () {
  if (!this->string || !this->ref) {
    return 0;
  }

  return this->length;
}

#pragma NativeCoreRefs

void NativeCoreRefs::Release () {
  this->env->DeleteGlobalRef(this->core);
}

#pragma NativeCore

NativeCore::NativeCore (JNIEnv *env, jobject core)
  : Core()
  , refs(env)
  , config()
  , rootDirectory(env)
  , environmentVariables()
  , javaScriptPreloadSource("")
{
  this->env = env;
  this->env->GetJavaVM(&this->jvm);
  this->refs.core = this->env->NewGlobalRef(core);
  this->self = this->refs.core;
}

NativeCore::~NativeCore () {
  this->rootDirectory.Release();
  this->refs.Release();

  this->env = 0;
  this->jvm = 0;
}

jboolean NativeCore::ConfigureEnvironment () {
  using SSC::createPreload;
  using SSC::decodeURIComponent;
  using SSC::encodeURIComponent;
  using SSC::getEnv;
  using SSC::parseConfig;
  using SSC::split;
  using SSC::trim;

  std::stringstream stream;

  // `NativeCore::getRootDirectory()`
  this->rootDirectory.Set((jstring) CallNativeCoreMethodFromEnvironment(
    this->env, this->refs.core, "getRootDirectory", "()Ljava/lang/String;"
  ));

  if (this->rootDirectory.size() == 0) {
    return false;
  }

  this->config = parseConfig(decodeURIComponent(STR_VALUE(SETTINGS)));

  for (auto const &tuple : this->config) {
    auto key = tuple.first;
    auto value = tuple.second;

    debug("AppConfig: %s = %s", key.c_str(), value.c_str());
  }

  for (auto const &var : split(this->config["env"], ',')) {
    auto key = trim(var);
    auto value = getEnv(key.c_str());

    if (value.size() > 0) {
      stream << key << "=" << encodeURIComponent(value) << "&";
      environmentVariables[key] = value;
      debug("EnvironmentVariable: %s=%s", key.c_str(), value.c_str());
    }
  }

  windowOptions.executable = this->config["executable"];
  windowOptions.headless = this->config["headless"] == "true";
  windowOptions.version = this->config["version"];
  windowOptions.preload = gPreloadMobile;
  windowOptions.title = this->config["title"];
  windowOptions.debug = DEBUG ? true : false;
  windowOptions.env = stream.str();

  this->javaScriptPreloadSource.assign(
    "console.error = console.debug = console.warn = console.log; \n"
    "                                                            \n"
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
    + createPreload(windowOptions)
    + "//# sourceURL=preload.js                                  \n"
  );

  stream.str(""); // clear stream

  if (windowOptions.debug) {
    auto Class = GetNativeCoreBindingClass(this->env);
    auto isDebugEnabledField =
      this->env->GetFieldID(Class, "isDebugEnabled", "Z");
    this->env->SetBooleanField(self, isDebugEnabledField, true);
  }

  return true;
}

jboolean NativeCore::ConfigureWebViewWindow () {
  return true;
}

void * NativeCore::GetPointer () const {
  return (void *) this;
}

JavaVM * NativeCore::GetJavaVM () {
  return this->jvm;
}

AppConfig & NativeCore::GetAppConfig () {
  return this->config;
}

const NativeCoreRefs & NativeCore::GetRefs () const {
  return this->refs;
}

const EnvironmentVariables & NativeCore::GetEnvironmentVariables () const {
  return this->environmentVariables;
}

const NativeString & NativeCore::GetRootDirectory () const {
  return this->rootDirectory;
}

const std::string NativeCore::GetNetworkInterfaces() const {
  return this->getNetworkInterfaces();
}

AAssetManager * NativeCore::GetAssetManager () const {
  // `NativeCore::getAssetManager()`
  auto ref = CallNativeCoreMethodFromEnvironment(
    this->env,
    this->refs.core,
    "getAssetManager",
    "()Landroid/content/res/AssetManager;"
  );

  if (ref) {
    return AAssetManager_fromJava(this->env, ref);
  }

  return nullptr;
}

const char * NativeCore::GetJavaScriptPreloadSource () const {
  if (this->javaScriptPreloadSource.size() == 0) {
    return nullptr;
  }

  return this->javaScriptPreloadSource.c_str();
}

void NativeCore::UpdateOpenDescriptorsInEnvironment () {
  std::lock_guard<std::recursive_mutex> guard(SSC::descriptorsMutex);
  auto refs = this->GetRefs();
  auto jvm = this->GetJavaVM();
  JNIEnv *env = 0;

  auto attached = jvm->AttachCurrentThread(&env, 0);

  std::stringstream js;

  js << "window.process.openFds.clear(false);";

  for (auto const &tuple : SSC::descriptors) {
    auto desc = tuple.second;

    if (desc == nullptr || (desc->fd == 0 && desc->dir == nullptr)) {
      continue;
    }

    auto id = std::to_string(desc->id);

    js << SSC::format(R"JS(
        window.process.openFds.set("$S", {
          id: "$S",
          fd: "$S",
          type: "$S"
        });
      )JS",
      id,
      id,
      desc->dir != nullptr ? id : std::to_string(desc->fd),
      std::string(desc->dir != nullptr ? "directory": "file")
    );
  }

  EvaluateJavaScriptInEnvironment(
    env,
    this->refs.core,
    env->NewStringUTF(js.str().c_str())
  );

  if (attached) {
    jvm->DetachCurrentThread();
  }
}

#pragma NativeFileSystem

NativeFileSystem::NativeFileSystem (JNIEnv *env, NativeCore *core) {
  this->env = env;
  this->core = core;
}

NativeFileSystemRequestContext * NativeFileSystem::CreateRequestContext (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = new NativeFileSystemRequestContext();

  context->callback = callback;
  context->core = this->core;
  context->seq = seq;
  context->fs = this;
  context->id = id;

  return context;
}

const std::string NativeFileSystem::CreateJSONError (NativeCoreID id, const std::string message)
  const {
  return SSC::format(
    R"MSG({"err":{"id":"$S","message":"$S"}})MSG",
    std::to_string(id),
    message
  );
}

void NativeFileSystem::CallbackAndFinalizeContext (
  NativeFileSystemRequestContext *context,
  std::string data
) const {
  auto refs = context->core->GetRefs();
  auto jvm = context->core->GetJavaVM();
  JNIEnv *env = 0;

  auto attached = jvm->AttachCurrentThread(&env, 0);

  CallNativeCoreVoidMethodFromEnvironment(
    env,
    refs.core,
    "callback",
    "(Ljava/lang/String;Ljava/lang/String;)V",
    context->callback,
    env->NewStringUTF(data.c_str())
  );

  delete context;

  if (attached) {
    jvm->DetachCurrentThread();
  }
}

void NativeFileSystem::CallbackWithPostAndFinalizeContext (
  NativeFileSystemRequestContext *context,
  std::string data,
  SSC::Post post
) const {
  auto refs = context->core->GetRefs();
  auto jvm = context->core->GetJavaVM();
  auto js = context->core->createPost(context->seq, "", post);

  JNIEnv *env = 0;

  auto attached = jvm->AttachCurrentThread(&env, 0);

  if (post.body != 0) {
    EvaluateJavaScriptInEnvironment(
      env,
      refs.core,
      env->NewStringUTF(js.c_str())
    );
  } else {
    CallNativeCoreVoidMethodFromEnvironment(
      env,
      refs.core,
      "callback",
      "(Ljava/lang/String;Ljava/lang/String;)V",
      context->callback,
      env->NewStringUTF(data.c_str())
    );
  }

  delete context;

  if (attached) {
    jvm->DetachCurrentThread();
  }
}

void NativeFileSystem::Access (
  NativeCoreSequence seq,
  std::string path,
  int mode,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, 0, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);

  core->fsAccess(seq, path, mode, [context](auto seq, auto data, auto post) {
    context->fs->CallbackAndFinalizeContext(context, data);
  });
}

void NativeFileSystem::Chmod (
  NativeCoreSequence seq,
  std::string path,
  int mode,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, 0, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);

  core->fsChmod(seq, path, mode, [context](auto seq, auto data, auto post) {
    context->fs->CallbackAndFinalizeContext(context, data);
  });
}

void NativeFileSystem::Close (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, id, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);

  core->fsClose(seq, id, [this, context](auto seq, auto data, auto post) {
    context->fs->CallbackAndFinalizeContext(context, data);
  });
}

void NativeFileSystem::FStat (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, id, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);

  core->fsFStat(seq, id, [context](auto seq, auto data, auto post) {
    context->fs->CallbackAndFinalizeContext(context, data);
  });
}

void NativeFileSystem::Open (
  NativeCoreSequence seq,
  NativeCoreID id,
  std::string path,
  int flags,
  int mode,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, id, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);

  core->fsOpen(seq, id, path, flags, mode, [this, context](auto seq, auto data, auto post) {
    context->fs->CallbackAndFinalizeContext(context, data);
  });
}

void NativeFileSystem::Read (
  NativeCoreSequence seq,
  NativeCoreID id,
  int len,
  int offset,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, id, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);

  core->fsRead(seq, id, len, offset, [context](auto seq, auto data, auto post) {
    context->fs->CallbackWithPostAndFinalizeContext(context, data, post);
  });
}

void NativeFileSystem::Stat (
  NativeCoreSequence seq,
  std::string path,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, 0, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);

  core->fsStat(seq, path, [context](auto seq, auto data, auto post) {
    context->fs->CallbackAndFinalizeContext(context, data);
  });
}

void NativeFileSystem::Write (
  NativeCoreSequence seq,
  NativeCoreID id,
  std::string data,
  int16_t offset,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, id, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);

  core->fsWrite(seq, id,  data, offset, [context](auto seq, auto data, auto post) {
    context->fs->CallbackAndFinalizeContext(context, data);
  });
}

// clang-format on

#pragma Bindings

/**
 * `NativeCore` JNI/NDK bindings.
 */
extern "C" {
  /**
   * `NativeCore::createPointer()` binding.
   * @return A pointer to a `NativeCore` instance
   */
  jlong exports(NativeCore, createPointer)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = new NativeCore(env, self);
    auto pointer = core->GetPointer();
    debug("Core::createPointer(%p)", pointer);
    return (jlong) core->GetPointer();
  }

  /**
   * `NativeCore::destroyPointer()` binding.
   * @param pointer Pointer to `NativeCore`
   */
  void exports(NativeCore, destroyPointer)(
    JNIEnv *env,
    jobject self,
    jlong pointer
  ) {
    if (pointer) {
      debug("Core::destroyPointer(%p)", (void *) pointer);
      delete (NativeCore *) pointer;
    }
  }

  /**
   * `NativeCore::verifyPointer()` binding.
   * @param pointer Pointer to verify
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyPointer)(
    JNIEnv *env,
    jobject self,
    jlong pointer
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!pointer || !core) {
      return false;
    }

    if ((void *) pointer != (void *) core) {
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyNativeExceptions()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  void exports(NativeCore, verifyNativeExceptions)(
    JNIEnv *env,
    jobject self
  ) {
    Throw(env, ExceptionCheckException);
  }

  /**
   * `NativeCore::verifyRootDirectory()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyRootDirectory)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    auto rootDirectory = core->GetRootDirectory();

    if (!rootDirectory.size()) {
      Throw(env, RootDirectoryIsNotReachableException);
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyAssetManager()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyAssetManager)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    auto assetManager = core->GetAssetManager();

    if (!assetManager) {
      Throw(env, AssetManagerIsNotReachableException);
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyLoop()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyLoop)(
    JNIEnv *env,
    jobject self
  ) {
    if (!uv_default_loop()) {
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyEnvironment()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyEnvironment)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    // @TODO
    return true;
  }

  /**
   * `NativeCore::verifyJavaVM()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyJavaVM)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    if (!core->GetJavaVM()) {
      Throw(env, NativeCoreJavaVMNotInitializedException);
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyRefs()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyRefs)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    auto refs = core->GetRefs();

    if (!refs.core) {
      Throw(env, NativeCoreRefsNotInitializedException);
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::verifyFileSystem()` binding.
   * @return `true` if verification passes, otherwise `false`.
   */
  jboolean exports(NativeCore, verifyFileSystem)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);
    int err = 0;

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    auto loop = uv_default_loop();

    struct {
      uv_fs_t opendir, closedir;
    } requests = {0};

    if (!loop) {
      Throw(env, UVLoopNotInitializedException);
      return false;
    }

    auto rootDirectory = core->GetRootDirectory();

    err = uv_fs_opendir(loop, &requests.opendir, rootDirectory.c_str(), 0);

    if (err != 0) {
      Throw(env, UVException(err));
      return false;
    }

    err = uv_run(loop, UV_RUN_DEFAULT);

    if (err != 0) {
      Throw(env, UVException(err));
      return false;
    }

    auto dir = (uv_dir_t *) requests.opendir.ptr;
    err = uv_fs_closedir(loop, &requests.closedir, dir, 0);

    if (err != 0) {
      Throw(env, UVException(err));
      return false;
    }

    return true;
  }

  /**
   * `NativeCore::configureEnvironment()` binding.
   * @return `true` if configuration was succcessful, otherwise `false`.
   */
  jboolean exports(NativeCore, configureEnvironment)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    return core->ConfigureEnvironment();
  }

  /**
   * `NativeCore::configureWebViewWindow()` binding.
   * @return `true` if configuration was succcessful, otherwise `false`.
   */
  jboolean exports(NativeCore, configureWebViewWindow)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return false;
    }

    return core->ConfigureWebViewWindow();
  }

  /**
   * `NativeCore::getPathToIndexHTML()` binding.
   * @return Path relative to `assets/` directory where `index.html` lives.
   */
  jstring exports(NativeCore, getPathToIndexHTML)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto config = core->GetAppConfig();
    auto index = config["android_index_html"];

    if (index.size() > 0) {
      return env->NewStringUTF(index.c_str());
    }

    return env->NewStringUTF("index.html");
  }

  /**
   * `NativeCore::getJavaScriptPreloadSource()` binding.
   * @return JavaScript preload source code injected into WebView.
   */
  jstring exports(NativeCore, getJavaScriptPreloadSource)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto preload = core->GetJavaScriptPreloadSource();

    if (!preload) {
      Throw(env, JavaScriptPreloadSourceNotInitializedException);
      return env->NewStringUTF("");
    }

    return env->NewStringUTF(preload);
  }

  /**
   * `NativeCore::getResolveToRenderProcessJavaScript()` binding.
   * @return JavaScript source code injected into WebView that performs an IPC
   * resolution
   */
  jstring exports(NativeCore, getResolveToRenderProcessJavaScript)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring state,
    jstring value,
    jboolean shouldEncodeValue
  ) {
    using SSC::encodeURIComponent;
    using SSC::resolveToRenderProcess;

    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto resolved = NativeString(env, value).str();

    if (shouldEncodeValue) {
      resolved = encodeURIComponent(resolved);
    }

    auto javascript = resolveToRenderProcess(
      NativeString(env, seq).str(),
      NativeString(env, state).str(),
      resolved
    );

    return env->NewStringUTF(javascript.c_str());
  }

  /**
   * `NativeCore::getEmitToRenderProcessJavaScript()` binding.
   * @return JavaScript source code injected into WebView that performs an IPC
   * event emission.
   */
  jstring exports(NativeCore, getEmitToRenderProcessJavaScript)(
    JNIEnv *env,
    jobject self,
    jstring event,
    jstring value
  ) {
    using SSC::emitToRenderProcess;

    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto javascript = emitToRenderProcess(
      NativeString(env, event).str(),
      NativeString(env, value).str()
    );

    return env->NewStringUTF(javascript.c_str());
  }

  /**
   * `NativeCore::getStreamToRenderProcessJavaScript()` binding.
   * @return JavaScript source code injected into WebView that performs an IPC
   * stream callback.
   */
  jstring exports(NativeCore, getStreamToRenderProcessJavaScript)(
    JNIEnv *env,
    jobject self,
    jstring id,
    jstring value
  ) {
    using SSC::streamToRenderProcess;

    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto javascript = streamToRenderProcess(
      NativeString(env, id).str(),
      NativeString(env, value).str()
    );

    return env->NewStringUTF(javascript.c_str());
  }

  /**
   * `NativeCore::getNetworkInterfaces()` binding.
   * @return Network interfaces in JSON format
   */
  jstring exports(NativeCore, getNetworkInterfaces)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    return env->NewStringUTF(core->GetNetworkInterfaces().c_str());
  }

  /**
   * `NativeCore::getPlatformArch()` binding.
   * @return Network interfaces in JSON format
   */
  jstring exports(NativeCore, getPlatformArch)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto msg = SSC::format(R"JSON({"data":"$S"})JSON", SSC::platform.arch);

    return env->NewStringUTF(msg.c_str());
  }

  /**
   * `NativeCore::getPlatformType()` binding.
   * @return Network interfaces in JSON format
   */
  jstring exports(NativeCore, getPlatformType)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto msg = SSC::format(R"JSON({"data":"$S"})JSON", std::string("linux"));

    return env->NewStringUTF(msg.c_str());
  }

  /**
   * `NativeCore::getPlatformOS()` binding.
   * @return Network interfaces in JSON format
   */
  jstring exports(NativeCore, getPlatformOS)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto msg = SSC::format(R"JSON({"data":"$S"})JSON", std::string("android"));

    return env->NewStringUTF(msg.c_str());
  }

  jstring exports(NativeCore, getEncodedFSConstants)(
    JNIEnv *env,
    jobject self,
    jstring seq
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto constants = NativeFileSystem::GetEncodedConstants();
    return env->NewStringUTF(constants.c_str());
  }

  jstring exports(NativeCore, getFSConstants)(
    JNIEnv *env,
    jobject self,
    jstring seq
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return env->NewStringUTF("");
    }

    auto constants = reinterpret_cast<SSC::Core *>(core)->getFSConstants();
    return env->NewStringUTF(constants.c_str());
  }

  jstring exports(NativeCore, createPost)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring params,
    jstring headers,
    jbyteArray bytes
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return nullptr;
    }

    auto size = env->GetArrayLength(bytes);
    auto body = new char[size];
    env->GetByteArrayRegion(bytes, 0, size, (jbyte *) body);

    auto js = core->createPost(
      NativeString(env, seq).str(),
      NativeString(env, params).str(),
      SSC::Post{
        .id = 0,
        .ttl = 0,
        .body = body,
        .length = size,
        .headers = NativeString(env, headers).str(),
        .bodyNeedsFree = true
      }
    );

    return env->NewStringUTF(js.c_str());
  }

  jbyteArray exports(NativeCore, getPostData)(
    JNIEnv *env,
    jobject self,
    jstring id
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return nullptr;
    }

    auto postId = GetNativeCoreIDFromJString(env, id);
    auto post = reinterpret_cast<SSC::Core *>(core)->getPost(postId);
    auto bytes = env->NewByteArray(post.length);

    env->SetByteArrayRegion(bytes, 0, post.length, (jbyte *) post.body);

    return bytes;
  }

  void exports(NativeCore, freePostData)(
    JNIEnv *env,
    jobject self,
    jstring id
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto postId = GetNativeCoreIDFromJString(env, id);
    auto post = reinterpret_cast<SSC::Core *>(core)->getPost(postId);

    reinterpret_cast<SSC::Core *>(core)->removePost(postId);
  }

  void exports(NativeCore, freeAllPostData)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    reinterpret_cast<SSC::Core *>(core)->removeAllPosts();
  }

  void exports(NativeCore, expirePostData)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    reinterpret_cast<SSC::Core *>(core)->expirePosts();
  }

  void exports(NativeCore, updateOpenDescriptorsInEnvironment)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    core->UpdateOpenDescriptorsInEnvironment();
  }

  void exports(NativeCore, fsAccess)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path,
    jint mode,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);

    fs.Access(
      NativeString(env, seq).str(),
      NativeString(env, path).str(),
      (int) mode,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, fsChmod)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path,
    jint mode,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);

    fs.Chmod(
      NativeString(env, seq).str(),
      NativeString(env, path).str(),
      (int) mode,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, fsClose)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);

    fs.Close(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, fsFStat)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);

    fs.FStat(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, fsOpen)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    jstring path,
    jint flags,
    jint mode,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);

    fs.Open(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      NativeString(env, path).str(),
      (int) flags,
      (int) mode,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, fsRead)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    int len,
    int offset,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);

    fs.Read(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      len,
      offset,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, fsStat)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring path,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);

    fs.Stat(
      NativeString(env, seq).str(),
      NativeString(env, path).str(),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, fsWrite)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    jstring data,
    int64_t offset,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);

    fs.Write(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      NativeString(env, data).str(),
      offset,
      (NativeCallbackID) callback
    );
  }
}
