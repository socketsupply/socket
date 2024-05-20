#ifndef SOCKET_RUNTIME_PLATFORM_ANDROID_LOOPER_H
#define SOCKET_RUNTIME_PLATFORM_ANDROID_LOOPER_H

#include "../platform.hh"
#include "environment.hh"

namespace SSC::Android {
  struct Looper {
    using LoopCallback = Function<void()>;
    using DispatchCallback = Function<void()>;

    struct LoopCallbackContext {
      const Looper::LoopCallback callback;
      Looper* looper = nullptr;
      LoopCallbackContext (
        const Looper::LoopCallback& callback,
        Looper* looper
      ) : callback(callback),
          looper(looper)
      {}
    };

    struct DispatchCallbackContext {
      const Looper::DispatchCallback callback;
      Looper* looper = nullptr;
      DispatchCallbackContext (
        const Looper::DispatchCallback& callback,
        Looper* looper
      ) : callback(callback),
          looper(looper)
      {}

      DispatchCallbackContext () = delete;
      DispatchCallbackContext (const DispatchCallbackContext&) = delete;
      DispatchCallbackContext (DispatchCallbackContext&&) = delete;
      DispatchCallbackContext operator= (const DispatchCallbackContext&) = delete;
      DispatchCallbackContext operator= (DispatchCallbackContext&&) = delete;
      virtual ~DispatchCallbackContext() {}
    };

    ALooper* looper;
    int fds[2];
    JVMEnvironment jvm;
    SharedPointer<LoopCallbackContext> context;

    Looper (JVMEnvironment jvm);
    bool acquire (const LoopCallback& callback = nullptr);
    void release ();
    void dispatch (const DispatchCallback& callback);
    bool isAcquired () const;
  };
}
#endif
