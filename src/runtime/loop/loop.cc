#include "../debug.hh"
#include "../loop.hh"

namespace ssc::runtime::loop {
  // in milliseconds
  constexpr int EVENT_LOOP_POLL_TIMEOUT = 32;

#if SOCKET_RUNTIME_PLATFORM_LINUX
  struct UVSource {
    GSource base; // should ALWAYS be first member
    gpointer tag;
    Loop* loop = nullptr;
  };
#endif

  static void onAsync (uv_async_t* async) {
    auto loop = reinterpret_cast<Loop*>(async->data);

    while (true) {
      Loop::DispatchCallback callback = nullptr;

      do {
        Lock lock(loop->mutex);

        if (loop->queue.size() > 0) {
          callback = std::move(loop->queue.front());
          loop->queue.pop();
        }
      } while (0);

      if (callback == nullptr) {
        break;
      }

      callback();
    }
  }

  static void onLoopThread (Loop* loop) {
    loop->state = Loop::State::Polling;

    while (loop->started() && loop->alive()) {
      if (!loop->uv.run(UV_RUN_DEFAULT)) {
        break;
      }
    }

    loop->state = Loop::State::Idle;
  }

  void Loop::UV::open (Loop* loop) {
    this->close();
    uv_loop_init(&this->loop);
    uv_loop_set_data(&this->loop, reinterpret_cast<void*>(loop));
    uv_async_init(&this->loop, &this->async, onAsync);
    this->async.data = reinterpret_cast<void*>(loop);

    this->isInitialized = true;
  }

  void Loop::UV::close () {
    if (this->isInitialized) {
      uv_stop(&this->loop);
      uv_run(&this->loop, UV_RUN_NOWAIT);
      auto handle = reinterpret_cast<uv_handle_t*>(&async);
      uv_close(handle, nullptr);
      uv_run(&this->loop, UV_RUN_NOWAIT);
      uv_loop_close(&this->loop);
    }

    this->isInitialized = false;
  }

  void Loop::UV::stop () {
    if (this->isInitialized) {
      uv_stop(&this->loop);
      uv_run(&this->loop, UV_RUN_NOWAIT);
    }
  }

  int Loop::UV::timeout () {
    uv_update_time(&this->loop);
    return uv_backend_timeout(&this->loop);
  }

  int Loop::UV::timeout () const {
    return uv_backend_timeout(&this->loop);
  }

  bool Loop::UV::run (uv_run_mode mode) {
    if (mode == UV_RUN_DEFAULT) {
      if (uv_run(&this->loop, mode) != 0) {
        return uv_run(&this->loop, UV_RUN_NOWAIT);
      }
    }

    if (mode == UV_RUN_NOWAIT || mode == UV_RUN_ONCE) {
      return uv_run(&this->loop, mode) != -1;
    }

    return false;
  }

  Loop::Loop (const Options& options)
    : options(options)
  {}

  bool Loop::init () {
    if (this->state < State::Idle) {
      this->state = State::Idle;
      this->uv.open(this);

    #if SOCKET_RUNTIME_PLATFORM_LINUX
      if (this->gtk.source) {
        const auto id = g_source_get_id(this->gtk.source);
        if (id > 0) {
          g_source_remove(id);
        }

        g_object_unref(this->gtk.source);
        this->gtk.source = nullptr;
      }

      // @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
      this->gtk.functions.prepare = [](GSource *source, gint *timeout) -> gboolean {
        auto loop = reinterpret_cast<UVSource*>(source)->loop;

        if (!loop->started()) {
          return false;
        }

        if (!loop->alive()) {
          return true;
        }

        *timeout = loop->timeout();
        return *timeout == 0;
      };

      this->gtk.functions.check = [](GSource* source) -> gboolean {
        const auto loop = reinterpret_cast<UVSource*>(source)->loop;
        const auto tag = reinterpret_cast<UVSource *>(source)->tag;
        const auto timeout = loop->timeout();

        if (timeout == 0) {
          return true;
        }

        const auto condition = g_source_query_unix_fd(source, tag);
        return (
          ((condition & G_IO_IN) == G_IO_IN) ||
          ((condition & G_IO_OUT) == G_IO_OUT)
        );
      };

      this->gtk.functions.dispatch = [](
        GSource *source,
        GSourceFunc callback,
        gpointer user_data
      ) -> gboolean {
        const auto loop = reinterpret_cast<UVSource*>(source)->loop;
        loop->run();
        return G_SOURCE_CONTINUE;
      };
    #endif
      return true;
    }

    return false;
  }

  bool Loop::shutdown () {
    if (this->state > State::None && this->state < State::Shutdown) {
      if (this->stop()) {
        this->uv.close();
        this->state = State::Shutdown;
        return true;
      }

      debug("Loop::start(): '* -> Shutdown' failed");
    }

    return false;
  }

  bool Loop::start () {
    if (this->state == State::Idle) {
      this->state = State::Starting;
      if (this->run()) {
        this->state = State::Started;
        return true;
      }

      debug("Loop::start(): 'Starting -> Started' failed");
    }

    return false;
  }

  bool Loop::stop () {
    if (this->state > State::Stopped && this->state < State::Shutdown) {
      Lock lock(this->mutex);
      this->state = State::Stoping;
      this->uv.stop();
      if (this->thread.joinable()) {
        this->thread.join();
      }
      this->state = State::Stopped;
      return true;
    }

    return false;
  }

  bool Loop::resume () {
    if (this->state < State::Resuming && this->state > State::None) {
      this->state = State::Resuming;
      if (this->run()) {
        this->state = State::Resumed;
        return true;
      }

      debug("Loop::resume(): 'Resuming -> Resumed' failed");
    }

    return false;
  }

  bool Loop::pause () {
    Lock lock(this->mutex);
    this->state = State::Pausing;
    this->uv.stop();

    // clean up old thread if still running
    if (this->thread.joinable()) {
      this->thread.join();
    }

    this->state = State::Paused;

    return false;
  }

  bool Loop::dispatch (const DispatchCallback& callback) {
    if (this->state < State::Starting) {
      return false;
    }

    do {
      Lock lock(this->mutex);
      this->queue.push(callback);
    } while (0);

    uv_async_send(&this->uv.async);
    return true;
  }

  bool Loop::run () {
    if (this->state > State::Idle) {
      return false;
    }

    Lock lock(this->mutex);

    this->init();

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    dispatch_async(this->dispatchQueue, ^{
      onLoopThread(this);
    });
  #else
    // clean up old thread if still running
    if (this->thread.joinable()) {
      this->uv.stop();
      this->thread.join();
    }

    this->thread = Thread(&onLoopThread, this);
  #endif

    return true;
  }

  uv_loop_t* Loop::get () {
    return &this->uv.loop;
  }

  bool Loop::alive () const {
    return uv_loop_alive(&this->uv.loop);
  }

  bool Loop::started () const {
    return this->state > State::Paused && this->state < State::Shutdown;
  }

  bool Loop::stopped () const {
    return this->state <= State::Stopped && this->state > State::None;
  }

  bool Loop::paused () const {
    return this->state == State::Pausing && this->state == State::Paused;
  }

  int Loop::timeout () {
    return this->uv.timeout();
  }

  int Loop::timeout () const {
    return this->uv.timeout();
  }

  bool Loop::sleep (int64_t ms) {
    return this->dispatch([ms]() {
      msleep(ms);
    });
  }
}
