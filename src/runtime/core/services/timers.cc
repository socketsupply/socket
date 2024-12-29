#include "timers.hh"

namespace ssc::runtime::core::services {
  struct TimerToken {
    Timers* timers = nullptr;
    Timers::ID id = 0;
  };

  Timers::Timer::Timer (Timers* timers, ID id, Callback callback)
    : timers(timers),
      id(id),
      callback(callback)
  {}

  const Timers::ID Timers::createTimer (
    uint64_t timeout,
    uint64_t interval,
    const Callback callback
  ) {
    Lock lock(this->mutex);

    auto id = rand64();
    auto loop = this->loop.get();
    auto handle = std::make_shared<Timer>(
      this,
      id,
      callback
    );

    if (interval > 0) {
      handle->repeat = true;
    }

    this->handles.emplace(handle->id, handle);

    this->loop.dispatch([=, this]() {
      Lock lock(this->mutex);
      if (this->handles.contains(id)) {
        auto handle = this->handles.at(id);
        const auto token = new TimerToken{ this, id };
        uv_handle_set_data((uv_handle_t*) &handle->timer, (void*) token);
        uv_timer_init(loop, &handle->timer);
        uv_timer_start(
          &handle->timer,
          [](uv_timer_t* timer) {
            const auto token = reinterpret_cast<TimerToken*>(uv_handle_get_data(reinterpret_cast<uv_handle_t*>(timer)));

            // bad state
            if (token == nullptr) {
              uv_timer_stop(timer);
              return;
            }

            Lock lock(token->timers->mutex);
            const auto id = token->id;

            // cancelled (removed from 'handles')
            if (!token->timers->handles.contains(id)) {
              delete token;
              uv_timer_stop(timer);
              return;
            }

            auto handle = token->timers->handles.at(id);

            // bad ref
            if (handle == nullptr) {
              delete token;
              uv_timer_stop(timer);
              return;
            }

            // `callback` to timer callback is a "cancel" function
            handle->callback([=] () {
              token->timers->cancelTimer(id);
              delete token;
            });

            if (!handle->repeat) {
              if (token->timers->handles.contains(id)) {
                token->timers->handles.erase(id);
                delete token;
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

  bool Timers::cancelTimer (const ID id) {
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

  const Timers::ID Timers::setTimeout (
    uint64_t timeout,
    const TimeoutCallback callback
  ) {
    Lock lock(this->mutex);
    const auto id = this->createTimer(timeout, 0, [callback] (auto _) {
      callback();
    });

    if (this->handles.contains(id)) {
      this->handles.at(id)->type = Timer::Type::Timeout;
    }

    return id;
  }

  bool Timers::clearTimeout (const ID id) {
    return this->cancelTimer(id);
  }

  const Timers::ID Timers::setInterval (
    uint64_t interval,
    const IntervalCallback callback
  ) {
    Lock lock(this->mutex);

    const auto id = this->createTimer(interval, interval, callback);

    if (this->handles.contains(id)) {
      this->handles.at(id)->type = Timer::Type::Interval;
    }

    return id;
  }

  bool Timers::clearInterval (const ID id) {
    return this->cancelTimer(id);
  }

  const Timers::ID Timers::setImmediate (const ImmediateCallback callback) {
    Lock lock(this->mutex);

    const auto id = this->createTimer(0, 0, [callback] (auto _) {
      callback();
    });

    if (this->handles.contains(id)) {
      this->handles.at(id)->type = Timer::Type::Immediate;
    }

    return id;
  }

  bool Timers::clearImmediate (const ID id) {
    return this->clearTimeout(id);
  }
}
