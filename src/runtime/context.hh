#ifndef SOCKET_RUNTIME_CONTEXT_H
#define SOCKET_RUNTIME_CONTEXT_H

#include "webview/webview.hh"
#include "queued_response.hh"
#include "platform.hh"
#include "loop.hh"

namespace ssc::runtime {
  class Runtime;
}

namespace ssc::runtime::context {
  struct Context {
    Mutex mutex;

  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    android::ContentResolver contentResolver;
    android::JVMEnvironment jvm;
    android::Activity activity = nullptr;
    android::Looper looper;
    bool isEmulator = false;
  #endif

    Context () = default;
    Context (const Context&) = delete;
    Context (Context&&) = delete;

    Context& operator = (const Context&) = delete;
    Context& operator = (Context&&) = delete;

  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    void Platform::configureAndroid (
      android::JVMEnvironment jvm,
      android::Activity activity
    );
  #endif
  };

  /**
   * The `RuntimeContext` struct represents a base class for `Runtime` or
   * other suitable candidates can extend from. This base class will contain
   * public state for the execution context in which it is active in.
   * This includes platform dependent information, APIs, and references.
   */
  struct RuntimeContext : public Context {
    // fka 'Posts' - this is a container for "queued" responses, most likely
    // sent to a webview through a javascript evaluation and a
    // `ipc://queuedResponse` XHR request
    QueuedResponses queuedResponses;
    loop::Loop loop;
    Mutex mutex;

    RuntimeContext () = default;
    RuntimeContext (const RuntimeContext&) = delete;
    RuntimeContext (RuntimeContext&&) = delete;

    RuntimeContext& operator = (const RuntimeContext&) = delete;
    RuntimeContext& operator = (RuntimeContext&&) = delete;

    RuntimeContext* getRuntimeContext ();
    Runtime* getRuntime ();
    String createQueuedResponse (
      const String& seq,
      const String& params,
      QueuedResponse queuedResponse
    );

  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    void Platform::configureAndroid (
      android::JVMEnvironment jvm,
      android::Activity activity
    );
  #endif
  };

  struct DispatchContext : public Context {
    RuntimeContext& context;
    DispatchContext () = delete;
    DispatchContext (RuntimeContext&);
    DispatchContext (const DispatchContext&) = delete;
    DispatchContext (DispatchContext&&) = delete;
    virtual ~DispatchContext ();
    DispatchContext& operator = (const DispatchContext&) = delete;
    DispatchContext& operator = (DispatchContext&&) = delete;
  };

  class Dispatcher : public DispatchContext {
    public:
      using Callback = Function<void()>;
      Dispatcher (RuntimeContext&);
      bool dispatch (const Callback);
  };

  using DispatchCallback = Dispatcher::Callback;
}
#endif
