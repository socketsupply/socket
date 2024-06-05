#include "../core.hh"
#include "timers.hh"

namespace SSC {
  CoreTimers::Timer::Timer (CoreTimers* timers, ID id, Callback callback)
    : timers(timers),
      id(id),
      callback(callback)
  {}

  const CoreTimers::ID CoreTimers::createTimer (
    uint64_t timeout,
    uint64_t interval,
    const Callback& callback
  ) {
    Lock lock(this->mutex);

    auto id = rand64();
    auto loop = this->core->getEventLoop();
    auto handle = std::make_shared<Timer>(
      this,
      id,
      callback
    );

    if (interval > 0) {
      handle->repeat = true;
    }

    this->handles.emplace(handle->id, handle);

    this->core->dispatchEventLoop([=, this]() {
      Lock lock(this->mutex);
      if (this->handles.contains(id)) {
        auto handle = this->handles.at(id);
        uv_handle_set_data((uv_handle_t*) &handle->timer, (void*) id);
        uv_timer_init(loop, &handle->timer);
        uv_timer_start(
          &handle->timer,
          [](uv_timer_t* timer) {
            auto loop = uv_handle_get_loop(reinterpret_cast<uv_handle_t*>(timer));
            auto core = reinterpret_cast<Core*>(uv_loop_get_data(loop));
            auto id = reinterpret_cast<ID>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(timer)));

            // bad state
            if (core == nullptr) {
              uv_timer_stop(timer);
              return;
            }

            Lock lock(core->timers.mutex);

            // cancelled (removed from 'handles')
            if (!core->timers.handles.contains(id)) {
              uv_timer_stop(timer);
              return;
            }

            auto handle = core->timers.handles.at(id);

            // bad ref
            if (handle == nullptr) {
              uv_timer_stop(timer);
              return;
            }

            // `callback` to timer callback is a "cancel" function
            handle->callback([=] () {
              core->timers.cancelTimer(id);
            });

            if (!handle->repeat) {
              if (core->timers.handles.contains(id)) {
                core->timers.handles.erase(id);
              }
            }
          },
          timeout,
          interval
        );
      }
    });

    return id;
  }

  bool CoreTimers::cancelTimer (const ID id) {
    Lock lock(this->mutex);

    if (!this->handles.contains(id)) {
      return false;
    }

    auto handle = this->handles.at(id);
    handle->cancelled = true;
    uv_timer_stop(&handle->timer);
    this->handles.erase(id);
    return true;
  }

  const CoreTimers::ID CoreTimers::setTimeout (
    uint64_t timeout,
    const TimeoutCallback& callback
  ) {
    return this->createTimer(timeout, 0, [callback] (auto _) {
      callback();
    });
  }

  bool CoreTimers::clearTimeout (const ID id) {
    return this->cancelTimer(id);
  }

  const CoreTimers::ID CoreTimers::setInterval (
    uint64_t interval,
    const IntervalCallback& callback
  ) {
    return this->createTimer(interval, interval, callback);
  }

  bool CoreTimers::clearInterval (const ID id) {
    return this->cancelTimer(id);
  }

  const CoreTimers::ID CoreTimers::setImmediate (const ImmediateCallback& callback) {
    return this->createTimer(0, 0, [callback] (auto _) {
      callback();
    });
  }

  bool CoreTimers::clearImmediate (const ID id) {
    return this->clearTimeout(id);
  }
}
