#ifndef SOCKET_RUNTIME_PLATFORM_ANDROID_LOOPER_H
#define SOCKET_RUNTIME_PLATFORM_ANDROID_LOOPER_H

#include "../types.hh"
#include "environment.hh"
#include "native.hh"

namespace ssc::runtime::android {
  struct Looper {
    using LoopCallback = Function<void()>;
    using DispatchCallback = Function<void()>;

    struct LoopCallbackContext {
      const Looper::LoopCallback callback;
      Looper* looper = nullptr;
      LoopCallbackContext (
        const Looper::LoopCallback callback,
        Looper* looper
      ) : callback(callback),
          looper(looper)
      {}
    };

    struct DispatchCallbackContext {
      const Looper::DispatchCallback callback;
      Looper* looper = nullptr;
      DispatchCallbackContext (
        const Looper::DispatchCallback callback,
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

    ALooper* looper = nullptr;
    Atomic<bool> isReady = false;
    Atomic<bool> isUnique = true;
    int fds[2];
    JVMEnvironment jvm;
    SharedPointer<LoopCallbackContext> context;

    Looper () = default;
    Looper (JVMEnvironment jvm);
    Looper (const Looper&);
    Looper (Looper&&);
    virtual ~Looper ();
    Looper operator= (const Looper&);
    Looper operator= (Looper&&);

    bool acquire (const LoopCallback callback = nullptr);
    void release ();
    void dispatch (const DispatchCallback callback);
    bool acquired () const;
  };
}
#endif
