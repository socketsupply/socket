#ifndef SOCKET_RUNTIME_CORE_SERVICES_TIMERS_H
#define SOCKET_RUNTIME_CORE_SERVICES_TIMERS_H

#include "../../core.hh"

namespace ssc::runtime::core::services {
  class Timers : public core::Service {
    public:
      using ID = uint64_t;
      using CancelCallback = Function<void()>;
      using Callback = Function<void(CancelCallback)>;
      using TimeoutCallback = Function<void()>;
      using IntervalCallback = Callback;
      using ImmediateCallback = TimeoutCallback;

      struct Timer {
        enum class Type { Timeout, Interval, Immediate };

        Timers* timers = nullptr;
        ID id = 0;
        Callback callback = nullptr;
        bool repeat = false;
        bool cancelled = false;
        uv_timer_t timer;
        Type type;
        Timer (Timers* timers, ID id, Callback callback);
      };

      using Handles = Map<ID, SharedPointer<Timer>>;

      Handles handles;
      Mutex mutex;

      Timers (const Options& options)
        : core::Service(options)
      {}

      const ID setTimeout (uint64_t, const TimeoutCallback);
      const ID setInterval (uint64_t, const IntervalCallback);
      const ID setImmediate (const ImmediateCallback);

      bool cancelTimer (const ID id);
      bool clearTimeout (const ID id);
      bool clearInterval (const ID id);
      bool clearImmediate (const ID id);
      const ID createTimer (uint64_t, uint64_t, const Callback);
  };
}
#endif
