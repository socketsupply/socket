#include "android.hh"

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
  : NativeString(env) {
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
  windowOptions.version = this->config["version"];
  windowOptions.preload = gPreloadMobile;
  windowOptions.title = this->config["title"];
  windowOptions.debug = DEBUG ? true : false;
  windowOptions.env = stream.str();

  this->javaScriptPreloadSource.assign(
    "console.error = console.warn = console.log;          \n"
    "                                                     \n"
    "window.addEventListener('unhandledrejection', e => { \n"
    "  console.log(e.reason || e.message || e);           \n"
    "});                                                  \n"
    "                                                     \n"
    "window.addEventListener('error', e => {              \n"
    "  const message = e.reason || e.message || e;        \n"
    "  if (!/debug-evaluate/.test(message)) {             \n"
    "    console.log(message);                            \n"
    "  }                                                  \n"
    "});                                                  \n"
    "                                                     \n"
    + createPreload(windowOptions)
    + "//# sourceURL=preload.js                           \n"
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

NativeString NativeCore::GetPlatformType () const {
  return NativeString(this->env, "linux");
}

NativeString NativeCore::GetPlatformOS () const {
  return NativeString(this->env, "android");
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
    R"MSG({"value":{"err":{"id": "$S", "message": "$S" }}})MSG",
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

  jvm->AttachCurrentThread(&env, 0);

  CallNativeCoreVoidMethodFromEnvironment(
    env,
    refs.core,
    "callback",
    "(JLjava/lang/String;)V",
    context->callback,
    env->NewStringUTF(data.c_str())
  );

  delete context;
}

void NativeFileSystem::CallbackWithPostAndFinalizeContext (
  NativeFileSystemRequestContext *context,
  std::string data,
  SSC::Post post
) const {
  auto params = SSC::format(R"QS(seq=$S)QS", context->seq);
  auto refs = context->core->GetRefs();
  auto jvm = context->core->GetJavaVM();
  auto js = context->core->createPost(params, post);

  JNIEnv *env = 0;

  jvm->AttachCurrentThread(&env, 0);

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
      "(JLjava/lang/String;)V",
      context->callback,
      env->NewStringUTF(data.c_str())
    );
  }

  delete context;
}

void NativeFileSystem::Open (
  NativeCoreSequence seq,
  NativeCoreID id,
  std::string path,
  int flags,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, id, callback);
  auto core = reinterpret_cast<SSC::Core *>(this->core);
  auto mode = S_IRUSR | S_IWUSR;

  core->fsOpen(seq, id, path, flags, mode, [context](auto seq, auto data, auto post) {
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

  core->fsClose(seq, id, [context](auto seq, auto data, auto post) {
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

void NativeFileSystem::Write (
  NativeCoreSequence seq,
  NativeCoreID id,
  std::string data,
  int16_t offset
) const {
}

void NativeFileSystem::Stat (
  NativeCoreSequence seq,
  std::string path
) const {
}

void NativeFileSystem::Unlink (
  NativeCoreSequence seq, std::string path
) const {
}

void NativeFileSystem::Rename (
  NativeCoreSequence seq,
  std::string from,
  std::string to
) const {
}

void NativeFileSystem::CopyFile (
  NativeCoreSequence seq,
  std::string from,
  std::string to,
  int flags
) const {
}

void NativeFileSystem::RemoveDirectory (
  NativeCoreSequence seq,
  std::string path
) const {
}

void NativeFileSystem::MakeDirectory (
  NativeCoreSequence seq,
  std::string path,
  int mode
) const {
}

void NativeFileSystem::ReadDirectory (
  NativeCoreSequence seq,
  std::string path
) const {
}

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
 * `NativeCore::verifyPlatform()` binding.
 * @return `true` if verification passes, otherwise `false`.
 */
jboolean exports(NativeCore, verifyPlatform)(
  JNIEnv *env,
  jobject self
) {
  auto core = GetNativeCoreFromEnvironment(env);

  if (!core) {
    Throw(env, NativeCoreNotInitializedException);
    return false;
  }

  if (core->GetPlatformOS().str() != "android") {
    return false;
  }

  if (core->GetPlatformType().str() != "linux") {
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
  jboolean ok = true;
  int err = 0;

  if (!core) {
    Throw(env, NativeCoreNotInitializedException);
    return false;
  }

  auto loop = uv_default_loop();

  struct {
    uv_fs_t opendir, readdir;
  } requests = { 0 };

  if (!loop) {
    Throw(env, UVLoopNotInitializedException);
    return false;
  }

  auto rootDirectory = core->GetRootDirectory();

  err = uv_fs_opendir(loop, &requests.opendir, rootDirectory.c_str(), nullptr);

  if (err != 0) {
    Throw(env, UVException(err));
    return false;
  }

  // TODO(jwerle):
  // - libuv filesystem readdir (check for files/ & cache/)
  // - libuv filesystem write OK to files/`VITAL_CHECK_FILE`
  // - libuv filesystem read OK from files/`VITAL_CHECK_FILE`
  // - AssetManager `VITAL_CHECK_FILE` check

  err = uv_run(loop, UV_RUN_DEFAULT);

  if (err != 0) {
    Throw(env, UVException(err));
    return false;
  }

  return ok;
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
    NativeString(env, seq).str(), NativeString(env, state).str(), resolved
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
    NativeString(env, event).str(), NativeString(env, value).str()
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
    NativeString(env, id).str(), NativeString(env, value).str()
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

  return env->NewStringUTF(core->getNetworkInterfaces().c_str());
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

  auto post = reinterpret_cast<SSC::Core *>(core)->getPost(std::stoull(NativeString(env, id).str()));
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

  auto post = reinterpret_cast<SSC::Core *>(core)->getPost(std::stoull(NativeString(env, id).str()));

  if (post.body && post.bodyNeedsFree) {
    // @TODO(jwerle): determine if this should be in `SSC::Core`
    delete post.body;
  }

  reinterpret_cast<SSC::Core *>(core)->removePost(std::stoull(NativeString(env, id).str()));
}

jstring exports(NativeCore, fsConstants)(
  JNIEnv *env,
  jobject self,
  jstring seq
) {
  auto core = GetNativeCoreFromEnvironment(env);

  if (!core) {
    Throw(env, NativeCoreNotInitializedException);
    return env->NewStringUTF("");
  }

  // auto fs = NativeFileSystem(env, core);
}

void exports(NativeCore, fsOpen)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  jlong id,
  jstring path,
  jint flags,
  jlong callback
) {
  auto core = GetNativeCoreFromEnvironment(env);

  if (!core) {
    return Throw(env, NativeCoreNotInitializedException);
  }

  auto fs = NativeFileSystem(env, core);

  fs.Open(
    NativeString(env, seq).str(),
    (NativeCoreID) id,
    NativeString(env, path).str(),
    (int) flags,
    (NativeCallbackID) callback
  );
}

void exports(NativeCore, fsClose)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  NativeCoreID id
) {
  // @TODO(jwerle): Core::fsClose
}

void exports(NativeCore, fsRead)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  NativeCoreID id,
  int len,
  int offset,
  NativeCallbackID callback
) {
  auto core = GetNativeCoreFromEnvironment(env);

  if (!core) {
    return Throw(env, NativeCoreNotInitializedException);
  }

  auto fs = NativeFileSystem(env, core);

  fs.Read(
    NativeString(env, seq).str(),
    (NativeCoreID) id,
    len,
    offset,
    callback
  );
}

void exports(NativeCore, fsWrite)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  NativeCoreID id,
  jstring data,
  int64_t offset
) {
  // @TODO(jwerle): Core::fsWrite
}

void exports(NativeCore, fsStat)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  jstring path
) {
  // @TODO(jwerle): Core::fsStat
}

void exports(NativeCore, fsUnlink)(
  jstring seq,
  jstring path
) {
  // @TODO(jwerle): Core::fsUnlink
}

void exports(NativeCore, fsRename)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  jstring pathA,
  jstring pathB
) {
  // @TODO(jwerle): Core::fsRename
}

void exports(NativeCore, fsCopyFile)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  jstring pathA,
  jstring pathB,
  int flags
) {
  // @TODO(jwerle): Core::fsCopyFile
}

void exports(NativeCore, fsRmDir)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  jstring path
) {
  // @TODO(jwerle): Core::fsRmDir
}

void exports(NativeCore, fsMkDir)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  jstring path,
  int mode
) {
  // @TODO(jwerle): Core::fsMkDir
}

void exports(NativeCore, fsReadDir)(
  JNIEnv *env,
  jobject self,
  jstring seq,
  jstring path
) {
  // @TODO(jwerle): Core::fsReadDir
}
}
