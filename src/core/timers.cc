#include "core.hh"

namespace SSC {
  const Core::Timers::ID Core::Timers::setTimeout (uint64_t timeout, const Callback callback) {
    Lock lock(this->mutex);

    auto id = rand64();
    auto handle = Timeout { id, callback };
    auto loop = this->core->getEventLoop();

    this->handles.insert_or_assign(handle.id, Timeout {
      id,
      callback
    });

    this->core->dispatchEventLoop([=, this]() {
      Lock lock(this->mutex);
      if (this->handles.contains(id)) {
        auto& handle = this->handles.at(id);
        uv_handle_set_data((uv_handle_t*) &handle.timer, &handle);
        uv_timer_init(loop, &handle.timer);
        uv_timer_start(
          &handle.timer,
          [](uv_timer_t* timer) {
            auto handle = reinterpret_cast<Timeout*>(uv_handle_get_data((uv_handle_t*) timer));
            if (handle != nullptr) {
              handle->callback();
            }
          },
          timeout,
          0
        );
      }
    });

    return id;
  }

  bool Core::Timers::clearTimeout (const ID id) {
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

  const Core::Timers::ID Core::Timers::setInterval (uint64_t interval, const Callback callback) {
    Lock lock(this->mutex);

    auto id = rand64();
    auto handle = Interval { id, callback };
    auto loop = this->core->getEventLoop();

    this->handles.insert_or_assign(handle.id, Timeout {
      id,
      callback,
      true
    });

    this->core->dispatchEventLoop([=, this]() {
      Lock lock(this->mutex);
      if (this->handles.contains(id)) {
        auto& handle = this->handles.at(id);
        uv_handle_set_data((uv_handle_t*) &handle.timer, &handle);
        uv_timer_init(loop, &handle.timer);
        uv_timer_start(
          &handle.timer,
          [](uv_timer_t* timer) {
            auto handle = reinterpret_cast<Timeout*>(uv_handle_get_data((uv_handle_t*) timer));
            if (handle != nullptr) {
              handle->callback();
            }
          },
         interval,
         interval
        );
      }
    });

    return id;
  }

  bool Core::Timers::clearInterval (const ID id) {
    return this->clearTimeout(id);
  }
}
