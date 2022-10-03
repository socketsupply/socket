#include "core.hh"

namespace SSC {
  static Timer releaseWeakDescriptors = {
    .timeout = 256, // in milliseconds
    .invoke = [](uv_timer_t *handle) {
      auto core = reinterpret_cast<Core *>(handle->data);
      std::vector<uint64_t> ids;
      SSC::String msg = "";

      std::lock_guard<std::recursive_mutex> descriptorsGuard(core->descriptorsMutex);
      for (auto const &tuple : core->descriptors) {
        ids.push_back(tuple.first);
      }

      for (auto const id : ids) {
        std::lock_guard<std::recursive_mutex> descriptorsGuard(core->descriptorsMutex);
        auto desc = core->descriptors.at(id);

        if (desc == nullptr) {
          core->descriptors.erase(id);
          continue;
        }

        if (desc->isRetained() || !desc->isStale()) {
          continue;
        }

        if (desc->isDirectory()) {
          core->fsClosedir("", id, [](auto seq, auto msg, auto post) {});
        } else if (desc->isFile()) {
          core->fsClose("", id, [](auto seq, auto msg, auto post) {});
        } else {
          // free
          core->descriptors.erase(id);
          delete desc;
        }
      }
    }
  };

  void Core::initTimers () {
    if (didTimersInit) {
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(timersMutex);

    auto loop = getEventLoop();

    std::vector<Timer *> timersToInit = {
      &releaseWeakDescriptors
    };

    for (const auto& timer : timersToInit) {
      uv_timer_init(loop, &timer->handle);
      timer->handle.data = (void *) this;
    }

    didTimersInit = true;
  }

  void Core::startTimers () {
    std::lock_guard<std::recursive_mutex> guard(timersMutex);

    std::vector<Timer *> timersToStart = {
      &releaseWeakDescriptors
    };

    for (const auto &timer : timersToStart) {
      if (timer->started) {
        uv_timer_again(&timer->handle);
      } else {
        timer->started = 0 == uv_timer_start(
          &timer->handle,
          timer->invoke,
          timer->timeout,
          !timer->repeated
            ? 0
            : timer->interval > 0
              ? timer->interval
              : timer->timeout
        );
      }
    }

    didTimersStart = false;
  }

  void Core::stopTimers () {
    if (didTimersStart == false) {
      return;
    }

    std::lock_guard<std::recursive_mutex> guard(timersMutex);

    std::vector<Timer *> timersToStop = {
      &releaseWeakDescriptors
    };

    for (const auto& timer : timersToStop) {
      if (timer->started) {
        uv_timer_stop(&timer->handle);
      }
    }

    didTimersStart = false;
  }
}
