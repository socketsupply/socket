#include "../core.hh"

namespace ssc::runtime::context {
  DispatchContext::DispatchContext (RuntimeContext& context)
    : context(context)
  {}

  DispatchContext::DispatchContext (const DispatchContext& context)
    : context(context.context)
  {}

  Dispatcher::Dispatcher (RuntimeContext& context)
    : DispatchContext(context)
  {}

  bool Dispatcher::dispatch (const Callback callback) {
    if (callback == nullptr) {
      return false;
    }

  #if SOCKET_RUNTIME_PLATFORM_LINUX
    g_main_context_invoke(
      nullptr,
      +[](gpointer userData) -> gboolean {
        const auto callback = reinterpret_cast<DispatchCallback*>(userData);
        if (*callback != nullptr) {
          (*callback)();
          delete callback;
        }
        return G_SOURCE_REMOVE;
      },
      new DispatchCallback(std::move(callback))
    );

    return true;
  #elif SOCKET_RUNTIME_PLATFORM_APPLE
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        callback();
      });
    });

    return true;
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    static auto mainThread = GetCurrentThreadId();
    auto threadCallback = (LPARAM) new DispatchCallback(std::move(callback));

    if (this->flags.ready.load(std::memory_order_relaxed)) {
      PostThreadMessage(mainThread, WM_APP, 0, threadCallback);
      return;
    }

    Thread t([&, threadCallback] {
      // TODO(trevnorris,jwerle): Need to also check a shouldExit so this doesn't run forever in case
      // the rest of the application needs to exit before isReady is set.
      while (!this->flags.ready.load(std::memory_order_relaxed) {
        msleep(16);
      }

      PostThreadMessage(mainThread, WM_APP, 0, threadCallback);
    });

    t.detach();
    return true;
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    this->looper.dispatch([this, callback = std::move(callback)] () {
      const auto attachment = Android::JNIEnvironmentAttachment(this->jvm);
      callback();
    });
    return true;
  #endif
    return false;
  }
}
