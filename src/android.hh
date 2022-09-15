#ifndef SSC_ANDROID_H
#define SSC_ANDROID_H

#ifndef __ANDROID__
#  define __ANDROID__
#endif

#ifndef ANDROID
#  define ANDROID 1
#endif

#ifndef DEBUG
#  define DEBUG 0
#endif

#ifndef SETTINGS
#  define SETTINGS ""
#endif

#ifndef VERSION
#  define VERSION ""
#endif

#ifndef VERSION_HASH
#  define VERSION_HASH ""
#endif

// defined before includes below
#if DEBUG
#define debug(format, ...)                                                  \
  {                                                                            \
    __android_log_print(                                                       \
      ANDROID_LOG_DEBUG, __FUNCTION__, format, ##__VA_ARGS__                   \
    );                                                                         \
  }
#else
#define debug(format, ...)
#endif

/**
 * Java Native Interface
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/jniTOC.html
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html
 * @see
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp276
 */
#include <jni.h>

// libc
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

// c++
#include <any>
#include <map>
#include <queue>
#include <semaphore>
#include <string>

// android
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

// SSC
#include "common.hh"
#include "core.hh"

typedef std::string NativeCoreSequence;
typedef uint64_t NativeID;
typedef NativeID NativeCallbackID;
typedef NativeID NativeCoreID;

typedef std::map<std::string, std::string> AppConfig;
typedef std::binary_semaphore BinarySemaphore; // aka `std::counting_semaphore<1>`
typedef std::map<std::string, std::string> EnvironmentVariables;
typedef std::recursive_mutex Mutex;
typedef std::lock_guard<Mutex> Lock;
typedef std::thread Thread;

template <typename T> using Queue = std::queue<T>;
template <int K> using Semaphore = std::counting_semaphore<K>;

// Forward declaration
class NativeCallbackRef;
class NativeCore;
class NativeFileSystem;
class NativeUDP;

/**
 * A file that should contain the contents `OK`. It is used during init
 * time verificatino checks.
 */
#define SSC_VITAL_CHECK_OK_FILE "__ssc_vital_check_ok_file__.txt"

/**
 * Defined by the Socket SDK preprocessor
 */
#define PACKAGE_NAME __BUNDLE_IDENTIFIER__

/**
 * Creates a named native package export for the configured bundle suitable for
 * for definition only.
 * @param name The name of the package function to export
 */
#define exports(namespace, name)                                               \
  JNIEXPORT JNICALL Java___BUNDLE_IDENTIFIER___##namespace##_##name

/**
 * Gets the `NativeCore` binding class from the JNI environment.
 */
#define GetNativeCoreBindingClass(env) env->GetObjectClass(self)

/**
 * Gets `NativeCore` instance pointer from JNI environment.
 */
#define GetNativeCoreFromEnvironment(env)                                      \
  ({                                                                           \
    auto Class = GetNativeCoreBindingClass(env);                               \
    auto pointerField = env->GetFieldID(Class, "pointer", "J");                \
    auto core = (NativeCore *) env->GetLongField(self, pointerField);          \
    if (!core) { debug("NativeCore failed to load in environment"); }          \
    (core);                                                                    \
  })

/**
 * Gets the JNI `Exception` class from environment.
 */
#define GetExceptionClass(env)                                                 \
  ({                                                                           \
    auto Exception = env->FindClass("java/lang/Exception");                    \
    (Exception);                                                               \
  })

/**
 * Calls method on `NativeCore` instance in environment.
 */
#define CallNativeCoreMethodFromEnvironment(                                   \
  env, object, method, signature, ...                                          \
)                                                                              \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    auto _value = env->CallObjectMethod(object, _id, ##__VA_ARGS__);           \
    (_value);                                                                  \
  })

/**
 * Calls `callback(id, data)` method on `NativeCore` instance in environment.
 */
#define CallNativeCoreVoidMethodFromEnvironment(                               \
  env, object, method, signature, ...                                          \
)                                                                              \
  ({                                                                           \
    auto _Class = env->GetObjectClass(object);                                 \
    auto _id = env->GetMethodID(_Class, method, signature);                    \
    env->CallVoidMethod(object, _id, ##__VA_ARGS__);                           \
  })


/**
 * Converts a `jstring` to an ID type
 */
#define GetNativeCoreIDFromJString(env, string)                                \
  (NativeCoreID) std::stoull(NativeString(env, string).str())

/**
 * @TODO
 */
#define EvaluateJavaScriptInEnvironment(env, object, source)                   \
  CallNativeCoreVoidMethodFromEnvironment(                                     \
    env, object, "evaluateJavascript", "(Ljava/lang/String;)V", source         \
  );

/**
 * Generic `Exception` throw helper
 */
#define Throw(env, E)                                                          \
  ({                                                                           \
    env->ThrowNew(GetExceptionClass(env), E);                                  \
    (void) 0;                                                                  \
  });

/**
 * Translate a libuv error to a message suitable for `Throw(...)`
 */
#define UVException(code) uv_strerror(code)

/**
 * Errors thrown from the JNI/NDK bindings
 */
#define AssetManagerIsNotReachableException                                    \
  "AssetManager is not reachable through binding"
#define ExceptionCheckException "ExceptionCheck"
#define JavaScriptPreloadSourceNotInitializedException                         \
  "JavaScript preload source is not initialized"
#define NativeCoreJavaVMNotInitializedException                                \
  "NativeCore JavaVM is not initialized"
#define NativeCoreNotInitializedException "NativeCore is not initialized"
#define NativeCoreRefsNotInitializedException                                  \
  "NativeCore refs are not initialized"
#define RootDirectoryIsNotReachableException                                   \
  "Root directory in file system is not reachable through binding"
#define UVLoopNotInitializedException "UVLoop is not initialized"

/**
 * A container for a JNI string (jstring).
 */
class NativeString {
  JNIEnv *env;
  jstring ref;
  size_t length;
  const char *string;
  jboolean needsRelease;

  public:
  /**
   * `NativeString` class constructors.
   */
  NativeString (JNIEnv *env);
  NativeString (const NativeString &copy);
  NativeString (JNIEnv *env, jstring ref);
  NativeString (JNIEnv *env, std::string string);
  NativeString (JNIEnv *env, const char *string);
  ~NativeString ();

  /**
   * @TODO
   */
  const NativeString &
  operator= (const NativeString &string) {
    *this = string;
    this->needsRelease = false;
    return *this;
  }

  /**
   * Various ways to set the internal value of a `NativeString` instance.
   */
  void Set (std::string string);
  void Set (const char *string);
  void Set (jstring ref);

  /**
   * Releases memory back to the JavaVM is needed. This is called
   * internally by the `NativeString` destructor.
   */
  void Release ();

  /**
   * Various ways to convert a `NativeString` to other string representations.
   */
  const char * c_str ();
  const std::string str ();
  const jstring jstr ();

  /**
   * Returns the computed size of internal string representation.
   */
  const size_t
  size ();
};

/**
 * A structured collection of global/local JNI references
 * for continued persistence in `NativeCore`.
 */
class NativeCoreRefs {
  JNIEnv *env;

  public:
  jobject core;

  NativeCoreRefs (JNIEnv *env) {
    this->env = env;
  }

  void Release ();
};

/**
 * A container for a callback pointer and JVM global ref
 */
class NativeCallbackRef {
  public:
  jobject ref = nullptr;
  NativeCore *core = nullptr;
  NativeCallbackID id = 0;
  std::string name;
  std::string signature;

  NativeCallbackRef () {};
  NativeCallbackRef (
    NativeCore *core,
    NativeCallbackID id,
    std::string name,
    std::string signature
  );

  ~NativeCallbackRef ();

  template <typename ...Args> void Call (Args ...args);
};

class NativeRequestContext {
  public:
  NativeCoreSequence seq = "";
  NativeCoreID id = 0;
  NativeCore *core = nullptr;
  NativeCallbackRef *callback = nullptr;

  ~NativeRequestContext () {
    if (this->callback != nullptr) {
      delete this->callback;
    }
  }

  void Send (std::string data, SSC::Post post) const;
  void Send (
    NativeCoreSequence seq,
    std::string data,
    SSC::Post post
  ) const;

  void Finalize (
    NativeCoreSequence seq,
    std::string data,
    SSC::Post post
  ) const;
  void Finalize (NativeCoreSequence seq, std::string data) const;
  void Finalize (std::string data, SSC::Post) const;
  void Finalize (std::string data) const;
  void Finalize (SSC::Post post) const;
};

class NativeFileSystem {
  NativeCore *core;
  JNIEnv *env;

  public:

  typedef std::map<std::string, std::string> Constants;

  NativeFileSystem (JNIEnv *env, NativeCore *core);

  static const Constants GetConstants () {
    return static_cast<Constants>(SSC::Core::getFSConstantsMap());
  }

  static const std::string GetEncodedConstants () {
    using SSC::encodeURIComponent;

    auto constants = GetConstants();
    std::stringstream stream;

    for (auto const &tuple : constants) {
      auto key = tuple.first;
      auto value = tuple.second;

      stream
        << encodeURIComponent(key) << "="
        << encodeURIComponent(value) << "&";
    }

    return stream.str();
  }

  void Access (
    NativeCoreSequence seq,
    std::string path,
    int mode,
    NativeCallbackID callback
  ) const;

  void Chmod (
    NativeCoreSequence seq,
    std::string path,
    int mode,
    NativeCallbackID callback
  ) const;

  void Close (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void FStat (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void Open (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string path,
    int flags,
    int mode,
    NativeCallbackID callback
  ) const;

  void Read (
    NativeCoreSequence seq,
    NativeCoreID id,
    int len,
    int offset,
    NativeCallbackID callback
  ) const;

  void Stat (
    NativeCoreSequence seq,
    std::string path,
    NativeCallbackID callback
  ) const;

  void Write (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string data,
    int16_t offset,
    NativeCallbackID callback
  ) const;
};

/**
 * An interface for core UDP APIs
 */
class NativeUDP {
  NativeCore *core;
  JNIEnv *env;

  public:

  NativeUDP (JNIEnv *env, NativeCore *core);

  void Bind (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string ip,
    int port,
    NativeCallbackID callback
  ) const;

  void Close (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void Connect (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string ip,
    int port,
    NativeCallbackID callback
  ) const;

  void Disconnect (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void GetPeerName (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void GetSockName (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void GetState (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void ReadStart (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void Send (
    NativeCoreSequence seq,
    NativeCoreID id,
    std::string data,
    int16_t size,
    std::string ip,
    int port,
    bool ephemeral,
    NativeCallbackID callback
  ) const;
};

/**
 * An extended `SSC::Core` class for Android NDK/JNI
 * imeplementation.
 */
class NativeCore : public SSC::Core {
  // special floating variable that points to `refs.core`
  jobject self;

  // JNI
  JavaVM *jvm;
  JNIEnv *env;
  int jniVersion = 0;

  // Native
  NativeCoreRefs refs;
  NativeString rootDirectory;

  // webkti webview
  std::string javaScriptPreloadSource;

  public:
  // thread
  Semaphore<64> semaphore;
  Mutex mutex;

  // window options
  SSC::WindowOptions windowOptions;

  // application
  AppConfig config;
  EnvironmentVariables environmentVariables;

  struct JNIEnvAttachment {
    JNIEnv *env = nullptr;
    JavaVM *jvm = nullptr;
    int status = 0;
    int version = 0;
    bool attached = false;

    JNIEnvAttachment () {}
    JNIEnvAttachment (NativeCore *core) {
      auto jvm = core->GetJavaVM();
      auto version = core->GetJNIVersion();
      this->Attach(jvm, version);
    }

    JNIEnvAttachment (JavaVM *jvm, int version) {
      this->Attach(jvm, version);
    }

    ~JNIEnvAttachment () {
      this->Detach();
    }

    void Attach (JavaVM *jvm, int version) {
      this->jvm = jvm;
      this->version = version;

      if (jvm != nullptr) {
        this->status = this->jvm->GetEnv((void **) &this->env, this->version);

        if (this->status == JNI_EDETACHED) {
          this->attached = this->jvm->AttachCurrentThread(&this->env, 0);
        }
      }
    }

    void Detach () {
      auto jvm = this->jvm;
      auto attached = this->attached;

      if (this->HasException()) {
        this->PrintException();
      }

      this->env = nullptr;
      this->jvm = nullptr;
      this->status = 0;
      this->attached = false;

      if (attached && jvm != nullptr) {
        jvm->DetachCurrentThread();
      }
    }

    inline bool HasException () {
      return this->env != nullptr && this->env->ExceptionCheck();
    }

    inline void PrintException () {
      if (this->env != nullptr) {
        this->env->ExceptionDescribe();
      }
    }
  };

  NativeCore (JNIEnv *env, jobject core);
  ~NativeCore ();

  jboolean ConfigureEnvironment ();
  jboolean ConfigureWebViewWindow ();

  NativeRequestContext * CreateRequestContext (
    NativeCoreSequence seq,
    NativeCoreID id,
    NativeCallbackID callback
  ) const;

  void EvaluateJavaScript (std::string js);
  void EvaluateJavaScript (const char *js);

  void * GetPointer () const;
  JavaVM * GetJavaVM () const;
  AppConfig & GetAppConfig ();

  const NativeString GetAppConfigValue (const char *key) const;
  const NativeString GetAppConfigValue (std::string key) const;
  const EnvironmentVariables & GetEnvironmentVariables () const;

  const NativeFileSystem GetNativeFileSystem () const;
  const NativeString & GetRootDirectory () const;
  const NativeCoreRefs & GetRefs () const;

  AAssetManager * GetAssetManager () const;

  const int GetJNIVersion () const;

  const std::string GetNetworkInterfaces () const;
  const char * GetJavaScriptPreloadSource () const;

  void DNSLookup (
    NativeCoreSequence seq,
    std::string hostname,
    int family,
    NativeCallbackID callback
  ) const;

  inline void Wait () {
    this->semaphore.acquire();
  }

  inline void Signal () {
    this->semaphore.release();
  }
};

/**
 * A structured container for a threaded dispatch queue
 */
class NativeThreadContext {
  public:
    typedef std::map<NativeID, NativeThreadContext*> Contexts;
    typedef std::function<void(NativeThreadContext*, const void*)> Function;

    /**
     * Maximum number of concurrent dispatch/release threads.
     */
    static constexpr int MAX_DISPATCH_CONCURRENCY = 64;
    static constexpr int MAX_RELEASE_CONCURRENCY = 16;

    /**
     * Maximum empty polls for dispatch worker before stopping main loop.
     */
    static constexpr int MAX_EMPTY_POLLS = 1024;

    /**
     * Timeout in milliseconds when polling in dispatch threads.
     */
    static constexpr int DISPATCH_POLL_TIMEOUT = 256; // in milliseconds

    /**
     * Timeout in milliseconds when polling in release threads.
     */
    static constexpr int RELEASE_POLL_TIMEOUT = 256; // in milliseconds

    /**
     * Static/global recursive mutex for threaded atomic operations during
     * dispatch/release.
     */
    static inline Mutex globalMutex;

    /**
     * Dispatch/release threads.
     */
    static inline Thread *globalDispatchThread;
    static inline Thread *globalReleaseThread;

    /**
     * Thread queues
     */
    static inline Queue<NativeID> globalDispatchQueue;
    static inline Queue<NativeID> globalReleaseQueue;

    /**
     * Atomic global queue sizes
     */
    static inline std::atomic<uint64_t> globalDispatchQueueSize = 0;
    static inline std::atomic<uint64_t> globalReleaseQueueSize = 0;

    /**
     * Thread semaphores.
     */
    static inline Semaphore<MAX_DISPATCH_CONCURRENCY> globalDispatchSemaphore;
    static inline Semaphore<MAX_RELEASE_CONCURRENCY> globalReleaseSemaphore;

    /**
     * Atomic thread running state predicates.
     */
    static inline std::atomic<bool> isDispatchThreadRunning = false;
    static inline std::atomic<bool> isReleaseThreadRunning = false;
    static inline std::atomic<bool> didDispatchThreadHaveFirstKick = false;
    static inline std::atomic<bool> didReleaseThreadHaveFirstKick = false;

    /**
     * Static map of all currently dispacthed contexts. Mapping an
     * `id` to a `NativeThreadContext*`
     */
    static inline Contexts contexts;

    /**
     * Atomic state predicates for instance.
     */
    std::atomic<bool> isCancelled = false;
    std::atomic<bool> isDispatched = false;
    std::atomic<bool> isInvoked = false;
    std::atomic<bool> isReleasing = false;
    std::atomic<bool> isRunning = false;
    std::atomic<bool> shouldAutoRelease = false;

    // instance state
    const NativeCore *core = nullptr;
    const void *data = nullptr;
    Function function = nullptr;
    NativeID id = 0;

    // thread state
    BinarySemaphore semaphore;
    Thread *thread = nullptr; // initialized in `Dispatch()`
    Mutex mutex;

    /**
     * `NativeThreadContext` class constructor.
     */
    NativeThreadContext (
      const NativeCore *core,
      NativeID contextId,
      const void *data,
      Function fn
    );

    /**
     * `NativeThreadContext` class destructor.
     * This will call `Release()` internally.
     */
    ~NativeThreadContext ();

    /**
     * Helper function to sleep `ms` milliseconds in the current thread.
     */
    static void SleepInThisThread (int64_t ms) {
      if (ms >= 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
      }
    }

    /**
     * PushContexts a context for dispatch. The context may already be queued, so
     * calling `PushContext()` again will queue the context for execution again.
     */
    static void PushContext (NativeThreadContext *context) {
      SetContext(context);
      {
        Lock lock(globalMutex);
        globalDispatchQueue.push(context->id);
        globalDispatchQueueSize = globalDispatchQueue.size();
      }
      KickThreads();
    }

    /**
     * PopContexts next item in dispatch queue and dispatches it. The
     * dispatched thread
     */
    static void PopContext () {
      NativeThreadContext *ctx = nullptr;

      globalDispatchSemaphore.acquire(); // wait (signaled in ThreadRunner)
      if (globalDispatchQueueSize > 0) {
        Lock lock(globalMutex);
        auto contextId = globalDispatchQueue.front();
        globalDispatchQueue.pop();
        globalDispatchQueueSize = globalDispatchQueue.size();
        ctx = Get(contextId);
      }

      if (ctx == nullptr || !ctx->Dispatch()) {
        globalDispatchSemaphore.release();
        return;
      }

      // max total timeout ~= `DISPATCH_POLL_TIMEOUT * DISPATCH_POLL_TIMEOUT`
      int timeouts = DISPATCH_POLL_TIMEOUT;
      while (isDispatchThreadRunning && !ctx->isInvoked && timeouts-- > 0) {
        SleepInThisThread(DISPATCH_POLL_TIMEOUT);
      }

      if (ctx->shouldAutoRelease) {
        Lock lock(globalMutex);
        globalReleaseQueue.push(ctx->id);
        globalReleaseQueueSize = globalReleaseQueue.size();
      }
    }

    /**
     * Stops all thread contexts.
     */
    static void StopAll () {
      Lock lock(globalMutex);
      for (auto const &tuple : contexts) {
        auto ctx = tuple.second;
        if (ctx != nullptr) {
          ctx->Stop();
        }
      }
    }

    /**
     * Stops and cancels all thread contexts.
     */
    static void CancelAll () {
      Lock lock(globalMutex);
      std::vector<NativeID> ids = {0};
      for (auto const &tuple : contexts) {
        auto ctx = tuple.second;
        if (ctx != nullptr) {
          ctx->Stop();
          ctx->Cancel();
          ids.push_back(ctx->id);
        }
      }

      for (auto const id : ids) {
        ReleaseContext(id);
      }

      while (globalDispatchQueue.size() > 0) {
        globalDispatchQueue.pop();
      }

      globalDispatchQueueSize = 0;
    }

    /**
     * Global thread runner.
     */
    static void StartDispatchThread () {
      isDispatchThreadRunning = true;
      while (isDispatchThreadRunning) {
        SleepInThisThread(DISPATCH_POLL_TIMEOUT);
        PopContext();
      }

      isDispatchThreadRunning = false;
    }

    static void StartReleaseThread () {
      isReleaseThreadRunning = true;
      int emptyPolls = 0;

      while (isReleaseThreadRunning) {
        SleepInThisThread(RELEASE_POLL_TIMEOUT);
        NativeThreadContext *ctx = nullptr;

        while (ctx == nullptr && globalReleaseQueueSize > 0) {
          Lock lock(globalMutex);
          auto contextId = globalReleaseQueue.front();
          globalReleaseQueue.pop();
          globalReleaseQueueSize = globalReleaseQueue.size();
          ctx = Get(contextId);
        }

        if (ctx != nullptr) {
          if (!ctx->isRunning || ctx->isCancelled) {
            globalReleaseSemaphore.acquire();
            ctx->Release();
            globalReleaseSemaphore.release();
          } else {
            Lock lock(globalMutex);
            globalReleaseQueue.push(ctx->id);
            globalReleaseQueueSize = globalReleaseQueue.size();
          }
        }

        SleepInThisThread(DISPATCH_POLL_TIMEOUT);
      }

      isReleaseThreadRunning = false;
    }

    /**
     * Sends stop signal to stop global dispatch thread.
     */
    static void StopDispatchThread () {
      isDispatchThreadRunning = false;
    }

    /**
     * Sends stop signal to stop global release thread.
     */
    static void StopReleaseThread () {
      isReleaseThreadRunning = false;
    }

    /**
     * Creates or recycles the dispatch thread.
     */
    static void KickDispatchThread () {
      if (!didDispatchThreadHaveFirstKick) {
        didDispatchThreadHaveFirstKick = true;
        globalDispatchSemaphore.release();
      }

      if (!isDispatchThreadRunning && globalDispatchThread != nullptr) {
        if (globalDispatchThread->joinable()) {
          isDispatchThreadRunning = false;
          globalDispatchThread->join();
        }

        Lock lock(globalMutex);
        delete globalDispatchThread;
        globalDispatchThread = nullptr;
      }

      if (globalDispatchThread == nullptr) {
        Lock lock(globalMutex);
        isDispatchThreadRunning = true;
        globalDispatchThread = new std::thread(&StartDispatchThread);
      }
    }

    /**
     * Creates or recycles the release thread.
     */
    static void KickReleaseThread () {
      if (!didReleaseThreadHaveFirstKick) {
        didReleaseThreadHaveFirstKick = true;
        globalReleaseSemaphore.release();
      }

      if (!isReleaseThreadRunning && globalReleaseThread != nullptr) {
        if (globalReleaseThread->joinable()) {
          isReleaseThreadRunning = false;
          globalReleaseThread->join();
        }

        Lock lock(globalMutex);
        delete globalReleaseThread;
        globalReleaseThread = nullptr;
      }

      if (globalReleaseThread == nullptr) {
        Lock lock(globalMutex);
        isReleaseThreadRunning = true;
        globalReleaseThread = new std::thread(&StartReleaseThread);
      }
    }

    /**
     * Creates or recycles the dispatch and release threads.
     */
    static void KickThreads () {
      KickDispatchThread();
      KickReleaseThread();
    }

    /**
     * Dispatches worker in `fn` of context `contextId`. If `contextId` is `0` then
     * the dispacted work is auto released after invocation.
     */
    static void Dispatch (const NativeCore *core, Function fn) {
      return Dispatch(core, 0, nullptr, fn);
    }

    static void Dispatch (const NativeCore *core, NativeID contextId, Function fn) {
      return Dispatch(core, contextId, nullptr, fn);
    }

    static void Dispatch (const NativeCore *core, const void *data, Function fn) {
      return Dispatch(core, 0, data, fn);
    }

    static void Dispatch (
      const NativeCore *core,
      NativeID contextId,
      const void *data,
      Function fn
    ) {
      PushContext(
        new NativeThreadContext(core, contextId, data, fn)
      );
    }

    /**
     * Releases work for a given context
     */
    static bool ReleaseContext (NativeThreadContext *context) {
      if (context != nullptr) {
        return ReleaseContext(context->id);
      }

      return false;
    }

    static bool ReleaseContext (NativeID contextId) {
      auto ctx = Get(contextId);

      if (ctx == nullptr) {
        return false;
      }

      RemoveContext(contextId);
      ctx->Cancel();
      ctx->Wait();
      delete ctx;

      return true;
    }

    /**
     * Sets a `NativeThreadContext` indexed by a context id.
     */
    static void SetContext (NativeThreadContext *context) {
      if (context != nullptr) {
        return SetContext(context->id, context);
      }
    }

    static void SetContext (NativeID contextId, NativeThreadContext *context) {
      Lock lock(globalMutex);
      contexts.insert_or_assign(contextId, context);
    }

    /**
     * Gets a known `NativeThreadContext` by `contextId`.
     */
    static NativeThreadContext* Get (NativeID contextId) {
      Lock lock(globalMutex);
      auto search = contexts.find(contextId);
      auto end = contexts.end();

      if (search != end) {
        return search->second;
      }

      return nullptr;
    }

    /**
     * Removes a known `NativeThreadContext` by `contextId`.
     */
    static bool RemoveContext (NativeThreadContext *context) {
      if (context != nullptr) {
        return RemoveContext(context->id);
      }

      return false;
    }

    static bool RemoveContext (NativeID contextId) {
      Lock lock(globalMutex);
      auto search = contexts.find(contextId);
      auto end = contexts.end();

      if (search != end) {
        contexts.erase(contextId);
        return true;
      }

      return false;
    }

    /**
     * A work runner function that is started by and monitored in the
     * `StartThread` function as a child thread for an executed
     * `NativeThreadContext` instance. The thread will execute the work
     * associated with the `NativeThreadContext` and any pending work in the
     * dispatch queue. The thread has a timeout to keep alive for future work
     * that is queued.
     */
    static void ThreadRunner (NativeThreadContext *ctx) {
      auto first = ctx;

      while (isDispatchThreadRunning && ctx != nullptr) {
        ctx->semaphore.acquire();
        if (
          first->isCancelled || ctx->isCancelled ||
          !first->isRunning || !ctx->isRunning
        ) {
          ctx->isRunning = false;

          if (ctx == first) {
            ctx->semaphore.release();
            break;
          }
        } else if (!ctx->isInvoked && (ctx == first || !ctx->isDispatched)) {
          ctx->isInvoked = true;
          ctx->function(ctx, ctx->data);
          if (ctx != first) {
            ctx->isRunning = false;
          }
        }

        ctx->semaphore.release();

        // keep alive (ms) ~= 1024*DISPATCH_POLL_TIMEOUT*DISPATCH_POLL_TIMEOUT
        int timeouts = 1024 * DISPATCH_POLL_TIMEOUT;
        while (
          timeouts-- > 0 &&
          isDispatchThreadRunning &&
          globalDispatchQueueSize == 0
        ) {
          SleepInThisThread(DISPATCH_POLL_TIMEOUT);
        }

        if (globalDispatchQueueSize == 0) {
          debug(
            "Dispatch queue empty - "
            "thread context work runner will eventually exit (id=%lu)",
            ctx->id
          );
          break;
        }

        ctx = nullptr;

        if (isDispatchThreadRunning && globalDispatchQueueSize > 0) {
          Lock lock(globalMutex);
          auto contextId = globalDispatchQueue.front();
          globalDispatchQueue.pop();
          globalDispatchQueueSize = globalDispatchQueue.size();

          ctx = Get(contextId);

          if (ctx != nullptr) {
            ctx->isRunning = true;

            if (ctx->shouldAutoRelease) {
              globalReleaseQueue.push(ctx->id);
              globalReleaseQueueSize = globalReleaseQueue.size();
            }
          }
        }
      }

      // requeue thread if started, but not invoked
      if (ctx != nullptr && ctx->isRunning && !ctx->isInvoked && !ctx->isCancelled) {
        Lock lock(globalMutex);
        globalDispatchQueue.push(ctx->id);
        globalDispatchQueueSize = globalDispatchQueue.size();
      }

      // finally, after draining the queue and max keep alive
      // set first context in run loop as not running to signal
      // `StartThread` that execution in this thread is "done" and that
      // the release thread can garbage collect this thread
      if (first != nullptr) {
        first->isRunning = false;
      }
    }

    /**
     * The function that runs the dispatched worker in the executed thread.
     */
    static void StartThread (NativeThreadContext *ctx) {
      if (ctx == nullptr || ctx->isCancelled || ctx->isInvoked || ctx->isRunning) {
        globalDispatchSemaphore.release(); // signal release for more work
        return;
      }

      // this value is set to `false` upon completion of work in the
      // `ThreadRunner` function thread and again upon termination of this one
      ctx->isRunning = true;
      Thread work(&ThreadRunner, ctx);

      // monitor work on `ThreadRunner` function thread until it is no longer
      // running (or stopped), or cancelled
      while (isDispatchThreadRunning && ctx->isRunning && !ctx->isCancelled) {
        SleepInThisThread(DISPATCH_POLL_TIMEOUT);
      }

      if (!ctx->isCancelled && work.joinable()) {
        work.join();
      }

      // release thread for more work
      ctx->isRunning = false;
      globalDispatchSemaphore.release(); // signal release for more work
    }

    /**
     * Sets dispatch worker function for context id
     */
    void Set (NativeID contextId, Function fn);
    void Set (Function fn);

    /**
     * Atomic dispatch worker thread functions.
     */
    void Cancel ();
    bool Dispatch ();
    bool Release ();
    void Stop ();
    void Wait ();
};

#endif
