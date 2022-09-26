#include "android.hh"
#include "uv.h"

// clang-format off

#pragma NativeString

NativeString::NativeString (JNIEnv *env) {
  this->env = env;
  this->ref = nullptr;
  this->length = 0;
  this->string = nullptr;
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

  this->ref = nullptr;
  this->length = 0;
  this->string = nullptr;
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

#pragma NativeCallbackRef
NativeCallbackRef::NativeCallbackRef (
  NativeCore *core,
  NativeCallbackID id,
  std::string name,
  std::string signature
) {
  Lock lock(core->mutex);
  NativeCore::JNIEnvAttachment ctx(core);

  this->signature = signature;
  this->name = name;
  this->core = core;
  this->ref = ctx.env->NewGlobalRef((jobject) id);
  this->id = id;
}

NativeCallbackRef::~NativeCallbackRef () {
  Lock lock(this->core->mutex);
  if (this->ref) {
    NativeCore::JNIEnvAttachment ctx(this->core);
    ctx.env->DeleteGlobalRef((jobject) this->ref);
  }

  this->signature = "";
  this->name = "";
  this->core = nullptr;
  this->ref = nullptr;
  this->id = 0;
}

template <typename ...Args> void NativeCallbackRef::Call (Args ...args) {
  Lock lock(this->core->mutex);
  NativeCore::JNIEnvAttachment ctx(this->core);

  auto refs = this->core->GetRefs();

  if (!ctx.HasException()) {
    CallNativeCoreVoidMethodFromEnvironment(
      ctx.env,
      refs.core,
      this->name.c_str(),
      this->signature.c_str(),
      this->ref,
      args...
    );
  }
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
  this->jniVersion = env->GetVersion();
  this->semaphore.release();
}

NativeCore::~NativeCore () {
  this->rootDirectory.Release();
  this->semaphore.release();
  this->refs.Release();

  this->env = nullptr;
  this->jvm = nullptr;
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
    this->env,
    this->refs.core,
    "getRootDirectory",
    "()Ljava/lang/String;"
  ));

  if (this->rootDirectory.size() == 0) {
    return false;
  }

  this->config = parseConfig(decodeURIComponent(STR_VALUE(SETTINGS)));

  if (DEBUG) {
    for (auto const &tuple : this->config) {
      auto key = tuple.first;
      auto value = tuple.second;

      debug("AppConfig: %s = %s", key.c_str(), value.c_str());
    }
  }

  for (auto const &var : split(this->config["env"], ',')) {
    auto key = trim(var);
    auto value = getEnv(key.c_str());

    if (value.size() > 0) {
      stream << key << "=" << encodeURIComponent(value) << "&";
      environmentVariables[key] = value;
      if (DEBUG) {
        debug("EnvironmentVariable: %s=%s", key.c_str(), value.c_str());
      }
    }
  }

  windowOptions.executable = this->config["executable"];
  windowOptions.headless = this->config["headless"] == "true";
  windowOptions.version = "v" + this->config["version"];
  windowOptions.preload = gPreloadMobile;
  windowOptions.title = this->config["title"];
  windowOptions.debug = DEBUG ? true : false;
  windowOptions.env = stream.str();
  windowOptions.cwd = this->rootDirectory.str();

  this->javaScriptPreloadSource.assign(
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

NativeRequestContext * NativeCore::CreateRequestContext (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = new NativeRequestContext();

  context->callback = new NativeCallbackRef(
    (NativeCore *) this,
    callback,
    "callback",
    "(Ljava/lang/String;Ljava/lang/String;)V"
  );

  context->core = (NativeCore *) this;
  context->seq = seq;
  context->id = id;

  return context;
}

void NativeCore::EvaluateJavaScript (std::string js) {
  return this->EvaluateJavaScript(js.c_str());
}

void NativeCore::EvaluateJavaScript (const char *js) {
  Lock lock(this->mutex);
  NativeCore::JNIEnvAttachment ctx(this);
  auto refs = this->GetRefs();

  if (!ctx.HasException()) {
    EvaluateJavaScriptInEnvironment(
      ctx.env,
      refs.core,
      ctx.env->NewStringUTF(js)
    );
  }
}

void * NativeCore::GetPointer () const {
  return (void *) this;
}

JavaVM * NativeCore::GetJavaVM () const {
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

const int NativeCore::GetJNIVersion () const {
  return this->jniVersion;
}

const char * NativeCore::GetJavaScriptPreloadSource () const {
  if (this->javaScriptPreloadSource.size() == 0) {
    return nullptr;
  }

  return this->javaScriptPreloadSource.c_str();
}

void NativeCore::SendBufferSize (
  NativeCoreSequence seq,
  NativeCoreID id,
  int size,
  NativeCallbackID callback
) {
  auto context = this->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this, [context, hostname, family](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);
    core->sendBufferSize(seq, id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeCore::RecvBufferSize (
  NativeCoreSequence seq,
  NativeCoreID id,
  int size,
  NativeCallbackID callback
) {
  auto context = this->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this, [context, hostname, family](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);
    core->recvBufferSize(seq, id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeCore::DNSLookup (
  NativeCoreSequence seq,
  std::string hostname,
  int family,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, 0, callback);
  NativeThreadContext::Dispatch(this, [context, hostname, family](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->dnsLookup(context->seq, hostname, family, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

#pragma NativeFileSystem

NativeFileSystem::NativeFileSystem (JNIEnv *env, NativeCore *core) {
  this->env = env;
  this->core = core;
}

void NativeFileSystem::Access (
  NativeCoreSequence seq,
  std::string path,
  int mode,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, 0, callback);
  NativeThreadContext::Dispatch(this->core, [context, path, mode](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsAccess(context->seq, path, mode, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeFileSystem::Chmod (
  NativeCoreSequence seq,
  std::string path,
  int mode,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, 0, callback);
  NativeThreadContext::Dispatch(this->core, [context, path, mode](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsChmod(context->seq, path, mode, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeFileSystem::Close (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsClose(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeFileSystem::FStat (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsFStat(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
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
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context, path, flags, mode](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsOpen(context->seq, context->id, path, flags, mode, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeFileSystem::Read (
  NativeCoreSequence seq,
  NativeCoreID id,
  int len,
  int offset,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context, len, offset](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsRead(context->seq, context->id, len, offset, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data, post);
    });
  });
}

void NativeFileSystem::Stat (
  NativeCoreSequence seq,
  std::string path,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, 0, callback);
  NativeThreadContext::Dispatch(this->core, [context, path](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsStat(context->seq, path, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeFileSystem::Write (
  NativeCoreSequence seq,
  NativeCoreID id,
  std::string data,
  int16_t offset,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context, data, offset](auto thread, auto _) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsWrite(context->seq, context->id,  data, offset, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

#pragma NativeUDP

NativeUDP::NativeUDP (JNIEnv *env, NativeCore *core) {
  this->env = env;
  this->core = core;
}

void NativeRequestContext::Send (
  std::string data,
  SSC::Post post
) const {
  this->Send(this->seq, data, post);
}

void NativeRequestContext::Send (
  NativeCoreSequence seq,
  std::string data,
  SSC::Post post
) const {
  if (post.body != 0 || this->seq == "-1") {
    auto js = this->core->createPost(this->seq, data, post);
    this->core->EvaluateJavaScript(js);
  } else if (this->seq.size() > 0 && this->seq != "-1") {
    Lock lock(this->core->mutex);
    NativeCore::JNIEnvAttachment ctx(this->core);
    this->callback->Call(ctx.env->NewStringUTF(data.c_str()));
  }
}

void NativeRequestContext::Finalize (
  NativeCoreSequence seq,
  std::string data,
  SSC::Post post
) const {
  this->Send(seq, data, post);
  delete this;
}

void NativeRequestContext::Finalize (
  std::string data,
  SSC::Post post
) const {
  this->Finalize(this->seq, data, post);
}

void NativeRequestContext::Finalize (
  NativeCoreSequence seq,
  std::string data
) const {
  this->Finalize(seq, data, SSC::Post{});
}

void NativeRequestContext::Finalize (
  std::string data
) const {
  this->Finalize(this->seq, data, SSC::Post{});
}

void NativeRequestContext::Finalize (
  SSC::Post post
) const {
  this->Finalize(this->seq, "{}", post);
}

void NativeUDP::Bind (
  NativeCoreSequence seq,
  NativeCoreID id,
  std::string address,
  int port,
  bool reuseAddr,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context, address, port, reuseAddr](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpBind(context->seq, context->id, address, port, reuseAddr, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeUDP::Close (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->close(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeUDP::Connect (
  NativeCoreSequence seq,
  NativeCoreID id,
  std::string ip,
  int port,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context, ip, port](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpConnect(context->seq, context->id, ip, port, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeUDP::Disconnect (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpDisconnect(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeUDP::GetPeerName (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpGetPeerName(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeUDP::GetSockName (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpGetSockName(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeUDP::GetState (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpGetState(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

void NativeUDP::ReadStart (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context](auto thread, auto data) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpReadStart(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Send(seq, data, post);
    });
  });
}

void NativeUDP::Send (
  NativeCoreSequence seq,
  NativeCoreID id,
  std::string data,
  int16_t size,
  std::string address,
  int port,
  bool ephemeral,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  NativeThreadContext::Dispatch(this->core, [context, data, size, address, port, ephemeral](auto thread, auto _) {
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpSend(context->seq, context->id, (char *) data.data(), size, port, address, ephemeral, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
  });
}

#pragma NativeThreadContext

NativeThreadContext::NativeThreadContext (
  const NativeCore *core,
  NativeID contextId,
  const void *data,
  Function function
) {
  this->core = core;
  this->data = data;
  this->Set(contextId, function);
  this->semaphore.release();
}

NativeThreadContext::~NativeThreadContext () {
  if (this->thread != nullptr) {
    this->Cancel();
    delete this->thread;
    this->thread = nullptr;
  }
}

void NativeThreadContext::Set (
  NativeID contextId,
  NativeThreadContext::Function function
) {
  this->id = contextId;
  this->function = function;

  if (contextId == 0) {
    this->id = SSC::rand64();
    this->shouldAutoRelease = true;
  }
}

bool NativeThreadContext::Dispatch () {
  Lock lock(this->mutex);

  if (
    this->thread != nullptr ||
    this->isDispatched ||
    this->isInvoked ||
    this->isReleasing ||
    this->isRunning
  ) {
    return false;
  }

  this->thread = new std::thread(&NativeThreadContext::StartThread, this);
  this->isDispatched = true;
  return true;
}

bool NativeThreadContext::Release () {
  if (this->isReleasing) {
    return false;
  }

  this->isReleasing = true;
  if (!this->isCancelled) {
    this->Wait();
  }
  return NativeThreadContext::ReleaseContext(this->id);
}

void NativeThreadContext::Wait () {
  if (this->thread != nullptr) {
    auto thread = this->thread;
    this->thread = nullptr;

    if (thread->joinable()) {
      thread->join();
    }

    Lock lock(this->mutex);
    delete thread;
  }
}

void NativeThreadContext::Stop () {
  this->isRunning = false;
}

void NativeThreadContext::Cancel () {
  // atomic
  this->isCancelled = true;
  this->isRunning = false;
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
   * Run the core event loop.
   */
  void exports(NativeCore, runEventLoop)(
    JNIEnv *env,
    jobject self
  ) {
    SSC::runEventLoop();
  }

  /**
   * Signal a stop of the core event loop.
   */
  void exports(NativeCore, stopEventLoop)(
    JNIEnv *env,
    jobject self
  ) {
    SSC::stopEventLoop();
  }

  /**
   * Resume all bound/connected peers
   */
  void exports(NativeCore, resumeAllPeers)(
    JNIEnv *env,
    jobject self
  ) {
    SSC::Peer::resumeAll();
  }

  /**
   * Pause all bound/connected peers
   */
  void exports(NativeCore, pauseAllPeers)(
    JNIEnv *env,
    jobject self
  ) {
    SSC::Peer::pauseAll();
  }

  /**
   * Starts the dispatch thread.
   */
  void exports(NativeCore, startDispatchThread)(
    JNIEnv *env,
    jobject self
  ) {
    NativeThreadContext::KickDispatchThread();
  }

  /**
   * Stops the dispatch thread.
   */
  void exports(NativeCore, stopDispatchThread)(
    JNIEnv *env,
    jobject self
  ) {
    NativeThreadContext::StopDispatchThread();
  }

  /**
   * Starts the release thread.
   */
  void exports(NativeCore, startReleaseThread)(
    JNIEnv *env,
    jobject self
  ) {
    NativeThreadContext::KickReleaseThread();
  }

  /**
   * Stop the release thread.
   */
  void exports(NativeCore, stopReleaseThread)(
    JNIEnv *env,
    jobject self
  ) {
    NativeThreadContext::StopReleaseThread();
  }

  /**
   * Cancel all dispatch thread contexts.
   */
  void exports(NativeCore, cancelAllDispatchThreads)(
    JNIEnv *env,
    jobject self
  ) {
    NativeThreadContext::CancelAll();
  }

  /**
   * Stop all dispatch thread contexts.
   */
  void exports(NativeCore, stopAllDispatchThreads)(
    JNIEnv *env,
    jobject self
  ) {
    NativeThreadContext::StopAll();
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

  void exports(NativeCore, sendBufferSize)(
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

    core->SendBufferSize(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (int) size,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, RecvBufferSize)(
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

    core->RecvBufferSize(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (int) size,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, dnsLookup)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring hostname,
    jint family,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    core->DNSLookup(
      NativeString(env, seq).str(),
      NativeString(env, hostname).str(),
      (int) family,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpClose)(
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

    auto udp = NativeUDP(env, core);

    udp.Close(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpBind)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    jstring address,
    jint port,
    jboolean reuseAddr,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto udp = NativeUDP(env, core);

    udp.Bind(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      NativeString(env, address).str(),
      (int) port,
      (bool) reuseAddr,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpConnect)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    jstring address,
    jint port,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto udp = NativeUDP(env, core);

    udp.Connect(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      NativeString(env, address).str(),
      (int) port,
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpDisconnect)(
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

    auto udp = NativeUDP(env, core);

    udp.Disconnect(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpGetPeerName)(
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

    auto udp = NativeUDP(env, core);

    udp.GetPeerName(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpGetSockName)(
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

    auto udp = NativeUDP(env, core);

    udp.GetSockName(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpGetState)(
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

    auto udp = NativeUDP(env, core);

    udp.GetState(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpReadStart)(
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

    auto udp = NativeUDP(env, core);

    udp.ReadStart(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (NativeCallbackID) callback
    );
  }

  void exports(NativeCore, udpSend)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    jstring data,
    jint size,
    jstring address,
    jint port,
    jboolean ephemeral,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto udp = NativeUDP(env, core);

    udp.Send(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      NativeString(env, data).str(),
      (int) size,
      NativeString(env, address).str(),
      (int) port,
      (bool) ephemeral,
      (NativeCallbackID) callback
    );
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
