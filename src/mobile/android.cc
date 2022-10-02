#include "../core/android.hh"

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
    auto core = GetNativeCoreFromEnvironment(env);
    int err = 0;

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return;
    }

    core->runEventLoop();
  }

  /**
   * Signal a stop of the core event loop.
   */
  void exports(NativeCore, stopEventLoop)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);
    int err = 0;

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return;
    }

    core->stopEventLoop();
  }

  /**
   * Resume all bound/connected peers
   */
  void exports(NativeCore, resumeAllPeers)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);
    int err = 0;

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return;
    }

    core->resumeAllPeers();
  }

  /**
   * Pause all bound/connected peers
   */
  void exports(NativeCore, pauseAllPeers)(
    JNIEnv *env,
    jobject self
  ) {
    auto core = GetNativeCoreFromEnvironment(env);
    int err = 0;

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return;
    }

    core->pauseAllPeers();
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
    jstring id,
    jstring params,
    jstring headers,
    jbyteArray bytes
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      Throw(env, NativeCoreNotInitializedException);
      return nullptr;
    }

    auto coreId = GetNativeCoreIDFromJString(env, id);
    if (core->hasPost(coreId)) {
      auto post = core->getPost(coreId);
      auto size = env->GetArrayLength(bytes);
      auto body = new char[size];
      if (post.bodyNeedsFree && post.body) {
        delete (char *) post.body;
      }

      post.body = body;
      post.length = size;
      post.bodyNeedsFree = true;

      auto js = core->createPost(
        NativeString(env, seq).str(),
        NativeString(env, params).str(),
        post
      );

      return env->NewStringUTF(js.c_str());
    } else {
      auto size = env->GetArrayLength(bytes);
      auto body = new char[size];
      env->GetByteArrayRegion(bytes, 0, size, (jbyte *) body);

      auto js = core->createPost(
        NativeString(env, seq).str(),
        NativeString(env, params).str(),
        SSC::Post{
          .id = coreId,
          .ttl = 0,
          .body = body,
          .length = size,
          .headers = NativeString(env, headers).str(),
          .bodyNeedsFree = true
        }
      );

      return env->NewStringUTF(js.c_str());
    }
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

  void exports(NativeCore, bufferSize)(
    JNIEnv *env,
    jobject self,
    jstring seq,
    jstring id,
    jint size,
    jint buffer,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    core->BufferSize(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      (int) size,
      (int) buffer,
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

  void exports(NativeCore, udpReadStop)(
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

    udp.ReadStop(
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
    jbyteArray data,
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

    size = env->GetArrayLength(data);
    auto udp = NativeUDP(env, core);
    auto bytes = new char[size > 0 ? size : 1]{0}; // free'd in `fs.Write()'
    env->GetByteArrayRegion(data, 0, size, (jbyte *) bytes);

    udp.Send(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      bytes,
      (int) size,
      NativeString(env, address).str(),
      (int) port,
      (bool) ephemeral,
      (NativeCallbackID) callback
    );

    delete [] bytes;
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
    jbyteArray data,
    int64_t offset,
    jstring callback
  ) {
    auto core = GetNativeCoreFromEnvironment(env);

    if (!core) {
      return Throw(env, NativeCoreNotInitializedException);
    }

    auto fs = NativeFileSystem(env, core);
    auto size = env->GetArrayLength(data);
    auto bytes = new char[size > 0 ? size : 1]{0}; // free'd in `fs.Write()'
    env->GetByteArrayRegion(data, 0, size, (jbyte *) bytes);

    fs.Write(
      NativeString(env, seq).str(),
      GetNativeCoreIDFromJString(env, id),
      bytes, // copied in `fsWrite`
      size,
      offset,
      (NativeCallbackID) callback
    );

    delete [] bytes;
  }
}
