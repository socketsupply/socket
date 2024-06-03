#include "core.hh"
#include "modules/fs.hh"

namespace SSC {
  Post Core::getPost (uint64_t id) {
    if (this->posts.find(id) == this->posts.end()) {
      return Post{};
    }

    return posts.at(id);
  }

  void Core::shutdown () {
  #if SOCKET_RUNTIME_PLATFORM_DESKTOP
    this->childProcess.shutdown();
  #endif
    this->stopEventLoop();
  }

  bool Core::hasPost (uint64_t id) {
    return posts.find(id) != posts.end();
  }

  bool Core::hasPostBody (const char* body) {
    if (body == nullptr) return false;
    for (const auto& tuple : posts) {
      auto post = tuple.second;
      if (post.body.get() == body) return true;
    }
    return false;
  }

  void Core::expirePosts () {
    Lock lock(this->postsMutex);
    const auto now = std::chrono::system_clock::now()
      .time_since_epoch()
      .count();

    Vector<uint64_t> ids;
    for (auto const &tuple : posts) {
      auto id = tuple.first;
      auto post = tuple.second;

      if (post.ttl < now) {
        ids.push_back(id);
      }
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }

  void Core::putPost (uint64_t id, Post p) {
    Lock lock(this->postsMutex);
    p.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(32 * 1024)
    )
      .time_since_epoch()
      .count();

    this->posts.insert_or_assign(id, p);
  }

  void Core::removePost (uint64_t id) {
    Lock lock(this->postsMutex);

    if (this->posts.find(id) == this->posts.end()) {
      return;
    }
    posts.erase(id);
  }

  String Core::createPost (String seq, String params, Post post) {
    if (post.id == 0) {
      post.id = rand64();
    }

    auto sid = std::to_string(post.id);
    auto js = createJavaScript("post-data.js",
      "const globals = await import('socket:internal/globals');              \n"
      "const id = `" + sid + "`;                                             \n"
      "const seq = `" + seq + "`;                                            \n"
      "const workerId = `" + post.workerId + "`.trim() || null;              \n"
      "const headers = `" + trim(post.headers) + "`                          \n"
      "  .trim()                                                             \n"
      "  .split(/[\\r\\n]+/)                                                 \n"
      "  .filter(Boolean)                                                    \n"
      "  .map((header) => header.trim());                                    \n"
      "                                                                      \n"
      "let params = `" + params + "`;                                        \n"
      "                                                                      \n"
      "try {                                                                 \n"
      "  params = JSON.parse(params);                                        \n"
      "} catch (err) {                                                       \n"
      "  console.error(err.stack || err, params);                            \n"
      "}                                                                     \n"
      "                                                                      \n"
      "globals.get('RuntimeXHRPostQueue').dispatch(                          \n"
      "  id,                                                                 \n"
      "  seq,                                                                \n"
      "  params,                                                             \n"
      "  headers,                                                            \n"
      "  { workerId }                                                        \n"
      ");                                                                    \n"
    );

    putPost(post.id, post);
    return js;
  }

  void Core::removeAllPosts () {
    Lock lock(this->postsMutex);
    std::vector<uint64_t> ids;

    for (auto const &tuple : posts) {
      auto id = tuple.first;
      ids.push_back(id);
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }

#if SOCKET_RUNTIME_PLATFORM_LINUX
  struct UVSource {
    GSource base; // should ALWAYS be first member
    gpointer tag;
    Core *core;
  };

    // @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
  static GSourceFuncs loopSourceFunctions = {
    .prepare = [](GSource *source, gint *timeout) -> gboolean {
      auto core = reinterpret_cast<UVSource *>(source)->core;
      if (!core->isLoopRunning) {
        return false;
      }

      if (!core->isLoopAlive()) {
        return true;
      }

      *timeout = core->getEventLoopTimeout();
      return *timeout == 0;
    },

    .check = [](GSource* source) -> gboolean {
      auto core = reinterpret_cast<UVSource *>(source)->core;
      auto tag = reinterpret_cast<UVSource *>(source)->tag;
      const auto timeout = core->getEventLoopTimeout();

      if (timeout == 0) {
        return true;
      }

      const auto condition = g_source_query_unix_fd(source, tag);
      return (
        ((condition & G_IO_IN) == G_IO_IN) ||
        ((condition & G_IO_OUT) == G_IO_OUT)
      );
    },

    .dispatch = [](
      GSource *source,
      GSourceFunc callback,
      gpointer user_data
    ) -> gboolean {
      auto core = reinterpret_cast<UVSource *>(source)->core;
      auto loop = core->getEventLoop();
      Lock lock(core->loopMutex);
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
    Lock lock(this->loopMutex);
    uv_loop_init(&eventLoop);
    eventLoopAsync.data = (void *) this;

    uv_async_init(&eventLoop, &eventLoopAsync, [](uv_async_t *handle) {
      auto core = reinterpret_cast<Core*>(handle->data);

      while (true) {
        Function<void()> dispatch = nullptr;

        do {
          Lock lock(core->loopMutex);
          if (core->eventLoopDispatchQueue.size() == 0) break;

          dispatch = core->eventLoopDispatchQueue.front();
          core->eventLoopDispatchQueue.pop();
        } while (0);

        if (dispatch == nullptr) {
          break;
        }

        dispatch();
      }
    });

  #if SOCKET_RUNTIME_PLATFORM_LINUX
    GSource *source = g_source_new(&loopSourceFunctions, sizeof(UVSource));
    UVSource *uvSource = (UVSource *) source;
    uvSource->core = this;
    uvSource->tag = g_source_add_unix_fd(
      source,
      uv_backend_fd(&eventLoop),
      (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
    );

    g_source_set_priority(source, G_PRIORITY_HIGH);
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
  #if SOCKET_RUNTIME_PLATFORM_ANDROID || SOCKET_RUNTIME_PLATFORM_WINDOWS
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
    {
      Lock lock(this->loopMutex);
      eventLoopDispatchQueue.push(callback);
    }

    signalDispatchEventLoop();
  }

  void pollEventLoop (Core *core) {
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

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    Lock lock(this->loopMutex);
    dispatch_async(eventLoopQueue, ^{ pollEventLoop(this); });
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID || SOCKET_RUNTIME_PLATFORM_WINDOWS
    Lock lock(this->loopMutex);
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

  struct Timer {
    uv_timer_t handle;
    bool repeated = false;
    bool started = false;
    uint64_t timeout = 0;
    uint64_t interval = 0;
    uv_timer_cb invoke;
  };

  static Timer releaseStrongReferenceDescriptors = {
    .repeated = true,
    .timeout = 1024, // in milliseconds
    .invoke = [](uv_timer_t *handle) {
      auto core = reinterpret_cast<Core *>(handle->data);
      Vector<CoreFS::ID> ids;
      String msg = "";

      {
        Lock lock(core->fs.mutex);
        for (auto const &tuple : core->fs.descriptors) {
          ids.push_back(tuple.first);
        }
      }

      for (auto const id : ids) {
        Lock lock(core->fs.mutex);
        auto desc = core->fs.descriptors.at(id);

        if (desc == nullptr) {
          core->fs.descriptors.erase(id);
          continue;
        }

        if (desc->isRetained() || !desc->isStale()) {
          continue;
        }

        if (desc->isDirectory()) {
          core->fs.closedir("", id, [](auto seq, auto msg, auto post) {});
        } else if (desc->isFile()) {
          core->fs.close("", id, [](auto seq, auto msg, auto post) {});
        } else {
          // free
          core->fs.descriptors.erase(id);
        }
      }
    }
  };

  static Timer releaseStrongReferenceSharedPointerBuffers = {
    .repeated = true,
    .timeout = 16, // in milliseconds
    .invoke = [](uv_timer_t *handle) {
      auto core = reinterpret_cast<Core *>(handle->data);
      Lock lock(core->mutex);
      for (int i = 0; i < core->sharedPointerBuffers.size(); ++i) {
        auto& entry = core->sharedPointerBuffers[i];
        // expired
        if (entry.ttl <= 16) {
          entry.pointer = nullptr;
          entry.ttl = 0;
          if (i == core->sharedPointerBuffers.size() - 1) {
            core->sharedPointerBuffers.pop_back();
            break;
          }
        } else {
          entry.ttl = entry.ttl - 16;
        }
      }

      if (core->sharedPointerBuffers.size() == 0) {
        uv_timer_stop(&releaseStrongReferenceSharedPointerBuffers.handle);
      }
    }
  };

  void Core::initTimers () {
    if (didTimersInit) {
      return;
    }

    Lock lock(this->timersMutex);

    auto loop = getEventLoop();

    Vector<Timer *> timersToInit = {
      &releaseStrongReferenceDescriptors,
      &releaseStrongReferenceSharedPointerBuffers
    };

    for (const auto& timer : timersToInit) {
      uv_timer_init(loop, &timer->handle);
      timer->handle.data = (void *) this;
    }

    didTimersInit = true;
  }

  void Core::startTimers () {
    Lock lock(this->timersMutex);

    Vector<Timer *> timersToStart = {
      &releaseStrongReferenceDescriptors,
      &releaseStrongReferenceSharedPointerBuffers
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

    didTimersStart = true;
  }

  void Core::stopTimers () {
    if (didTimersStart == false) {
      return;
    }

    Lock lock(this->timersMutex);

    Vector<Timer *> timersToStop = {
      &releaseStrongReferenceDescriptors,
      &releaseStrongReferenceSharedPointerBuffers
    };

    for (const auto& timer : timersToStop) {
      if (timer->started) {
        uv_timer_stop(&timer->handle);
      }
    }

    didTimersStart = false;
  }

  const CoreTimers::ID Core::setTimeout (
    uint64_t timeout,
    const CoreTimers::TimeoutCallback& callback
  ) {
    return this->timers.setTimeout(timeout, callback);
  }

  const CoreTimers::ID Core::setImmediate (
    const CoreTimers::ImmediateCallback& callback
  ) {
    return this->timers.setImmediate(callback);
  }

  const CoreTimers::ID Core::setInterval (
    uint64_t interval,
    const CoreTimers::IntervalCallback& callback
  ) {
    return this->timers.setInterval(interval, callback);
  }

  bool Core::clearTimeout (const CoreTimers::ID id) {
    return this->timers.clearTimeout(id);
  }

  bool Core::clearImmediate (const CoreTimers::ID id) {
    return this->timers.clearImmediate(id);
  }

  bool Core::clearInterval (const CoreTimers::ID id) {
    return this->timers.clearInterval(id);
  }

  void Core::retainSharedPointerBuffer (
    SharedPointer<char[]> pointer,
    unsigned int ttl
  ) {
    if (pointer == nullptr) {
      return;
    }

    Lock lock(this->mutex);
    for (auto& entry : this->sharedPointerBuffers) {
      if (entry.ttl == 0 && entry.pointer == nullptr) {
        entry.ttl = ttl;
        entry.pointer = pointer;
        return;
      }
    }

    this->sharedPointerBuffers.emplace_back(SharedPointerBuffer {
      pointer,
      ttl
    });

    uv_timer_again(&releaseStrongReferenceSharedPointerBuffers.handle);
  }

  void Core::releaseSharedPointerBuffer (SharedPointer<char[]> pointer) {
    if (pointer == nullptr) {
      return;
    }

    Lock lock(this->mutex);
    for (auto& entry : this->sharedPointerBuffers) {
      if (entry.pointer.get() == pointer.get()) {
        entry.pointer = nullptr;
        entry.ttl = 0;
        return;
      }
    }
  }
}
