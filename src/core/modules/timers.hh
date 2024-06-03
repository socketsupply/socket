#ifndef SOCKET_RUNTIME_CORE_MODULE_TIMERS_H
#define SOCKET_RUNTIME_CORE_MODULE_TIMERS_H

#include "../module.hh"

namespace SSC {
  class Core;
  class CoreTimers : public CoreModule {
    public:
      using ID = uint64_t;
      using CancelCallback = Function<void()>;
      using Callback = Function<void(CancelCallback)>;
      using TimeoutCallback = Function<void()>;
      using IntervalCallback = Callback;
      using ImmediateCallback = TimeoutCallback;

      struct Timer {
        CoreTimers* timers = nullptr;
        ID id = 0;
        Callback callback = nullptr;
        bool repeat = false;
        bool cancelled = false;
        uv_timer_t timer;
        Timer (CoreTimers* timers, ID id, Callback callback);
      };

      using Handles = std::map<ID, SharedPointer<Timer>>;

      Handles handles;
      Mutex mutex;

      CoreTimers (Core* core)
        : CoreModule (core)
      {}

      const ID createTimer (
        uint64_t timeout,
        uint64_t interval,
        const Callback& callback
      );

      const ID setTimeout (uint64_t timeout, const TimeoutCallback& callback);
      const ID setInterval (uint64_t interval, const IntervalCallback& callback);
      const ID setImmediate (const ImmediateCallback& callback);

      bool cancelTimer (const ID id);
      bool clearTimeout (const ID id);
      bool clearInterval (const ID id);
      bool clearImmediate (const ID id);
  };
}
#endif
