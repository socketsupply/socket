#include "core.hh"

namespace SSC {
#if defined(__linux__) && !defined(__ANDROID__)
  struct UVSource {
    GSource base; // should ALWAYS be first member
    gpointer tag;
    Core *core;
  };

    // @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
  static GSourceFuncs loopSourceFunctions = {
    .prepare = [](GSource *source, gint *timeout) -> gboolean {
      auto core = reinterpret_cast<UVSource *>(source)->core;
      if (!core->isLoopAlive() || !core->isLoopRunning) {
        return false;
      }

      *timeout = core->getEventLoopTimeout();
      return 0 == *timeout;
    },

    .dispatch = [](GSource *source, GSourceFunc callback, gpointer user_data) -> gboolean {
      std::lock_guard<std::recursive_mutex> lock(loopMutex);
      auto core = reinterpret_cast<UVSource *>(source)->core;
      auto loop = core->getEventLoop();
      uv_run(loop, UV_RUN_NOWAIT);
      return G_SOURCE_CONTINUE;
    }
  };
#endif

  void Core::initEventLoop () {
    if (didLoopInit) {
      return;
    }

    didLoopInit = true;
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    uv_loop_init(&eventLoop);
    eventLoopAsync.data = (void *) this;
    uv_async_init(&eventLoop, &eventLoopAsync, [](uv_async_t *handle) {
      auto core = reinterpret_cast<SSC::Core  *>(handle->data);
      while (true) {
        std::lock_guard<std::recursive_mutex> lock(core->loopMutex);
        if (core->eventLoopDispatchQueue.size() == 0) break;
        auto dispatch = core->eventLoopDispatchQueue.front();
        if (dispatch != nullptr) dispatch();
        core->eventLoopDispatchQueue.pop();
      }
    });

#if defined(__linux__) && !defined(__ANDROID__)
    GSource *source = g_source_new(&loopSourceFunctions, sizeof(UVSource));
    UVSource *uvSource = (UVSource *) source;
    uvSource->core = this;
    uvSource->tag = g_source_add_unix_fd(
      source,
      uv_backend_fd(&eventLoop),
      (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
    );

    g_source_attach(source, nullptr);
#endif
  }

  uv_loop_t* Core::getEventLoop () {
    initEventLoop();
    return &eventLoop;
  }

  int Core::getEventLoopTimeout () {
    auto loop = getEventLoop();
    uv_update_time(loop);
    return uv_backend_timeout(loop);
  }

  bool Core::isLoopAlive () {
    return uv_loop_alive(getEventLoop());
  }

  void Core::stopEventLoop() {
    isLoopRunning = false;
    uv_stop(&eventLoop);
#if defined(__APPLE__)
    // noop
#elif defined(__ANDROID__) || !defined(__linux__)
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    if (eventLoopThread != nullptr) {
      if (eventLoopThread->joinable()) {
        eventLoopThread->join();
      }

      delete eventLoopThread;
      eventLoopThread = nullptr;
    }
#endif
  }

  void Core::sleepEventLoop (int64_t ms) {
    if (ms > 0) {
      auto timeout = getEventLoopTimeout();
      ms = timeout > ms ? timeout : ms;
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
  }

  void Core::sleepEventLoop () {
    sleepEventLoop(getEventLoopTimeout());
  }

  void Core::signalDispatchEventLoop () {
    initEventLoop();
    runEventLoop();
    uv_async_send(&eventLoopAsync);
  }

  void Core::dispatchEventLoop (EventLoopDispatchCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    eventLoopDispatchQueue.push(callback);
    signalDispatchEventLoop();
  }

  static void pollEventLoop (auto core) {
    auto loop = core->getEventLoop();

    while (core->isLoopRunning) {
      core->sleepEventLoop(EVENT_LOOP_POLL_TIMEOUT);

      do {
        uv_run(loop, UV_RUN_DEFAULT);
      } while (core->isLoopRunning && core->isLoopAlive());
    }

    core->isLoopRunning = false;
  }

  void Core::runEventLoop () {
    if (isLoopRunning) {
      return;
    }

    isLoopRunning = true;

    initEventLoop();
    dispatchEventLoop([=, this]() {
      initTimers();
      startTimers();
    });

#if defined(__APPLE__)
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    dispatch_async(eventLoopQueue, ^{ pollEventLoop(this); });
#elif defined(__ANDROID__) || !defined(__linux__)
    std::lock_guard<std::recursive_mutex> lock(loopMutex);
    // clean up old thread if still running
    if (eventLoopThread != nullptr) {
      if (eventLoopThread->joinable()) {
        eventLoopThread->join();
      }

      delete eventLoopThread;
      eventLoopThread = nullptr;
    }

    eventLoopThread = new std::thread(&pollEventLoop, this);
#endif
  }
}
