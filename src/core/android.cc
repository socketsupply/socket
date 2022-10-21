#include "android.hh"
#include <uv.h>

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

NativeString::NativeString (JNIEnv *env, SSC::String string)
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

void NativeString::Set (SSC::String string) {
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

const SSC::String NativeString::str () {
  SSC::String value;

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
  SSC::String name,
  SSC::String signature
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
      this->ref, // ref of `id`
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
  , jsPreloadSource("")
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

  SSC::StringStream stream;

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

  this->config = parseConfig(decodeURIComponent(STR_VALUE(SSC_SETTINGS)));

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
  windowOptions.title = this->config["title"];
  windowOptions.debug = DEBUG ? true : false;
  windowOptions.env = stream.str();
  windowOptions.cwd = this->rootDirectory.str();

  this->jsPreloadSource.assign(
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
    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"
  );

  context->core = (NativeCore *) this;
  context->seq = seq;
  context->id = id;

  return context;
}

void NativeCore::EvaluateJavaScript (SSC::String js) {
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

const SSC::String NativeCore::GetNetworkInterfaces() const {
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
  if (this->jsPreloadSource.size() == 0) {
    return nullptr;
  }

  return this->jsPreloadSource.c_str();
}

void NativeCore::BufferSize (
  NativeCoreSequence seq,
  NativeCoreID id,
  int size,
  int buffer,
  NativeCallbackID callback
) {
  auto context = this->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);
    core->bufferSize(context->seq, context->id, size, buffer, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeCore::DNSLookup (
  NativeCoreSequence seq,
  SSC::String hostname,
  int family,
  NativeCallbackID callback
) const {
  auto context = this->CreateRequestContext(seq, 0, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->dnsLookup(context->seq, hostname, family, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

#pragma NativeFileSystem

NativeFileSystem::NativeFileSystem (JNIEnv *env, NativeCore *core) {
  this->env = env;
  this->core = core;
}

void NativeFileSystem::Access (
  NativeCoreSequence seq,
  SSC::String path,
  int mode,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, 0, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsAccess(context->seq, path, mode, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeFileSystem::Chmod (
  NativeCoreSequence seq,
  SSC::String path,
  int mode,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, 0, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsChmod(context->seq, path, mode, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeFileSystem::Close (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsClose(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeFileSystem::FStat (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsFStat(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeFileSystem::Open (
  NativeCoreSequence seq,
  NativeCoreID id,
  SSC::String path,
  int flags,
  int mode,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsOpen(context->seq, context->id, path, flags, mode, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
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
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsRead(context->seq, context->id, len, offset, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data, post);
    });
}

void NativeFileSystem::Stat (
  NativeCoreSequence seq,
  SSC::String path,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, 0, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->fsStat(context->seq, path, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeFileSystem::Write (
  NativeCoreSequence seq,
  NativeCoreID id,
  char *bytes,
  int16_t size,
  int16_t offset,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  auto core = reinterpret_cast<SSC::Core *>(context->core);
  auto data = SSC::String();

  data.assign(bytes, size);

  core->fsWrite(context->seq, context->id, data, offset, [context, bytes](auto seq, auto data, auto post) {
    context->Finalize(seq, data);
  });
}

#pragma NativeUDP

NativeUDP::NativeUDP (JNIEnv *env, NativeCore *core) {
  this->env = env;
  this->core = core;
}

void NativeRequestContext::Send (
  SSC::String data,
  SSC::Post post
) const {
  this->Send(this->seq, data, post);
}

void NativeRequestContext::Send (
  NativeCoreSequence seq,
  SSC::String data,
  SSC::Post post
) const {
  if (post.id != 0) {
    this->core->putPost(post.id, post);
  }

  if (seq.size() > 0 && seq != "-1") {
    Lock lock(this->core->mutex);
    NativeCore::JNIEnvAttachment ctx(this->core);

    this->callback->Call(
      ctx.env->NewStringUTF(data.c_str()),
      ctx.env->NewStringUTF(std::to_string(post.id).c_str())
    );
  } else {
    auto js = this->core->createPost(seq, data, post);
    this->core->EvaluateJavaScript(js);
  }
}

void NativeRequestContext::Finalize (
  NativeCoreSequence seq,
  SSC::String data,
  SSC::Post post
) const {
  this->Send(seq, data, post);
  delete this;
}

void NativeRequestContext::Finalize (
  SSC::String data,
  SSC::Post post
) const {
  this->Finalize(this->seq, data, post);
}

void NativeRequestContext::Finalize (
  NativeCoreSequence seq,
  SSC::String data
) const {
  this->Finalize(seq, data, SSC::Post{});
}

void NativeRequestContext::Finalize (
  SSC::String data
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
  SSC::String address,
  int port,
  bool reuseAddr,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpBind(context->seq, context->id, address, port, reuseAddr, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeUDP::Close (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->close(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeUDP::Connect (
  NativeCoreSequence seq,
  NativeCoreID id,
  SSC::String ip,
  int port,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpConnect(context->seq, context->id, ip, port, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeUDP::Disconnect (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpDisconnect(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeUDP::GetPeerName (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpGetPeerName(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeUDP::GetSockName (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpGetSockName(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeUDP::GetState (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpGetState(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data);
    });
}

void NativeUDP::ReadStart (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpReadStart(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Send(seq, data, post);
    });
}

void NativeUDP::ReadStop (
  NativeCoreSequence seq,
  NativeCoreID id,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
    auto core = reinterpret_cast<SSC::Core *>(context->core);

    core->udpReadStop(context->seq, context->id, [context](auto seq, auto data, auto post) {
      context->Finalize(seq, data, post);
    });
}

void NativeUDP::Send (
  NativeCoreSequence seq,
  NativeCoreID id,
  char *data,
  int16_t size,
  SSC::String address,
  int port,
  bool ephemeral,
  NativeCallbackID callback
) const {
  auto context = this->core->CreateRequestContext(seq, id, callback);
  auto core = reinterpret_cast<SSC::Core *>(context->core);

  core->udpSend(context->seq, context->id, data, size, port, address, ephemeral, [context](auto seq, auto data, auto post) {
    context->Finalize(seq, data);
  });
}
