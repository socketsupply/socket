#include "core.hh"

namespace SSC {
  void msleep (uint64_t ms) {
    std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }

  const Core::Timers::ID Core::Timers::createTimer (uint64_t timeout, uint64_t interval, const Callback callback) {
    Lock lock(this->mutex);

    auto id = rand64();
    auto loop = this->core->getEventLoop();
    auto handle = Timer {
      this,
      id,
      callback
    };

    if (interval > 0) {
      handle.repeat = true;
    }

    this->handles.insert_or_assign(handle.id, handle);

    this->core->dispatchEventLoop([=, this]() {
      Lock lock(this->mutex);
      if (this->handles.contains(id)) {
        auto& handle = this->handles.at(id);
        uv_handle_set_data((uv_handle_t*) &handle.timer, &handle);
        uv_timer_init(loop, &handle.timer);
        uv_timer_start(
          &handle.timer,
          [](uv_timer_t* timer) {
            auto handle = reinterpret_cast<Timer*>(uv_handle_get_data((uv_handle_t*) timer));
            if (handle != nullptr) {
              handle->callback([handle] () {
                handle->timers->core->dispatchEventLoop([handle]() {
                  handle->timers->cancelTimer(handle->id);
                });
              });

              do {
                Lock lock(handle->timers->mutex);
                if (!handle->repeat) {
                  if (handle->timers->handles.contains(handle->id)) {
                    handle->timers->handles.erase(handle->id);
                  }
                }
              } while (0);
            }
          },
          timeout,
          interval
        );
      }
    });

    return id;
  }

  bool Core::Timers::cancelTimer (const ID id) {
    Lock lock(this->mutex);

    if (!this->handles.contains(id)) {
      return false;
    }

    auto& handle = this->handles.at(id);
    handle.cancelled = true;
    uv_timer_stop(&handle.timer);
    this->handles.erase(id);
    return true;
  }

  const Core::Timers::ID Core::Timers::setTimeout (uint64_t timeout, const TimeoutCallback callback) {
    return this->createTimer(timeout, 0, [callback] (auto _) {
      callback();
    });
  }

  bool Core::Timers::clearTimeout (const ID id) {
    return this->cancelTimer(id);
  }

  const Core::Timers::ID Core::Timers::setInterval (uint64_t interval, const IntervalCallback callback) {
    return this->createTimer(interval, interval, callback);
  }

  bool Core::Timers::clearInterval (const ID id) {
    return this->cancelTimer(id);
  }

  const Core::Timers::ID Core::Timers::setImmediate (const ImmediateCallback callback) {
    return this->createTimer(0, 0, [callback] (auto _) {
      callback();
    });
  }

  bool Core::Timers::clearImmediate (const ID id) {
    return this->clearTimeout(id);
  }
}
