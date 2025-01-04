#include "../debug.hh"
#include "../loop.hh"

#if SOCKET_RUNTIME_PLATFORM_LINUX
struct UVSource {
  GSource base; // should ALWAYS be first member
  gpointer tag;
  ssc::runtime::loop::Loop* loop = nullptr;
};
#endif

namespace ssc::runtime::loop {
  // in milliseconds
  constexpr int EVENT_LOOP_POLL_TIMEOUT = 32;

  // async work is dispatched here which will cause the loop state
  // to transitions to `State::Polling` while in a dequeue loop and
  // then finally back to `State::Idle`
  static void onAsyncThread (uv_async_t* async) {
    auto loop = reinterpret_cast<Loop*>(async->data);
    Loop::DispatchCallback callback = nullptr;
    // transition to `State::Polling` while waiting
    loop->state = Loop::State::Polling;

    while (true) {
      do {
        Lock lock(loop->mutex);
        // dequeue dispatched callback
        if (loop->queue.size() > 0) {
          callback = std::move(loop->queue.front());
          loop->queue.pop();
        }
      } while (0);

      if (callback == nullptr) {
        break;
      }

      callback();
      callback = nullptr;
    }

    if (loop->state == Loop::State::Polling) {
      // transition back to `State::Idle` if was just polling
      loop->state = Loop::State::Idle;
    }
  }

  // `onLoopThread` is used by android/darwin/win32 while linux uses a gsoure
  static void onLoopThread (Loop* loop) {
    while (loop->started() && loop->alive()) {
      // transition to `State::Polling` while waiting
      loop->state = Loop::State::Polling;
      if (!loop->uv.run(UV_RUN_DEFAULT)) {
        break;
      }
    }

    // if the loop was in an `Idle` or `Polling` state, then transition
    // back to the `Init` state so `kick()` can kick of the thread again
    if (loop->state == Loop::State::Idle || loop->state == Loop::State::Polling) {
      // transition back to `State::Init` so `kick()` will dispatch
      // this loop thread again
      loop->state = Loop::State::Init;
    }
  }

  void Loop::UV::open (Loop* loop) {
    // deinit/close if already opened
    this->close();
    // init uv loop
    uv_loop_init(&this->loop);
    // set internal pointer to `Loop`
    uv_loop_set_data(&this->loop, reinterpret_cast<void*>(loop));
    // init async thread
    uv_async_init(&this->loop, &this->async, onAsyncThread);
    // set internal pointer to `Loop`
    this->async.data = reinterpret_cast<void*>(loop);
    this->isInitialized = true;
  }

  void Loop::UV::close () {
    // we want to make sure the loop is entirely stopped, drained, and closed
    if (this->isInitialized) {
      // stop and drain the loop
      uv_stop(&this->loop);
      uv_run(&this->loop, UV_RUN_NOWAIT);
      // close async handle
      auto handle = reinterpret_cast<uv_handle_t*>(&async);
      uv_close(handle, nullptr);
      // drain the loop one more, and then close
      uv_run(&this->loop, UV_RUN_NOWAIT);
      uv_loop_close(&this->loop);
    }
    this->isInitialized = false;
  }

  void Loop::UV::stop () {
    if (this->isInitialized) {
      // signal stop
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
        return uv_run(&this->loop, UV_RUN_NOWAIT) > 0;
      }
    }

    if (mode == UV_RUN_NOWAIT || mode == UV_RUN_ONCE) {
      return uv_run(&this->loop, mode) > 0;
    }

    return false;
  }

  Loop::Loop (const Options& options)
    : options(options)
  {}

  bool Loop::init () {
    if (this->state == State::None) {
      this->state = State::Init;
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
        loop->state = Loop::State::Polling;
        loop->uv.run(UV_RUN_NOWAIT);
        loop->state = Loop::State::Idle;
        return G_SOURCE_CONTINUE;
      };

      this->gtk.source = g_source_new(&this->gtk.functions, sizeof(UVSource));
      auto uvsource = reinterpret_cast<UVSource*>(this->gtk.source);
      uvsource->loop = this;
      uvsource->tag = g_source_add_unix_fd(
        this->gtk.source,
        uv_backend_fd(this->get()),
        (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
      );

      g_source_set_priority(this->gtk.source, G_PRIORITY_HIGH);
      g_source_attach(this->gtk.source, nullptr);

    #endif
    }

    return this->state >= State::Init && this->state < State::Shutdown;
  }

  bool Loop::kick () {
    if (this->state >= State::Idle) {
      return true;
    }

    if (this->state == State::None) {
      if (!this->init()) {
        return false;
      }
    }

    this->state = State::Idle;
    Lock lock(this->mutex);

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    dispatch_async(this->dispatchQueue, ^{
      onLoopThread(this);
    });
  #else
    if (this->options.dedicatedThread) {
      // clean up old thread if still running
      if (this->thread.joinable()) {
        this->uv.stop();
        this->thread.join();
      }

      this->thread = Thread(&onLoopThread, this);
    }
  #endif

    return this->state >= State::Idle;
  }

  bool Loop::start () {
    return this->init() && this->kick();
  }

  bool Loop::stop () {
    if (this->state == State::Stopped) {
      return true;
    } else if (this->state < State::Idle || this->state == State::Shutdown) {
      return false;
    }

    this->state = State::Stopped;
    this->uv.stop();

    Lock lock(this->mutex);
    if (this->options.dedicatedThread) {
      if (this->thread.joinable()) {
        this->thread.join();
      }
    }

    return this->state == State::Stopped;
  }

  bool Loop::resume () {
    if (this->state == State::Idle || this->state == State::Polling) {
      return true;
    } else if (this->state < State::Idle) {
      return this->start();
    } else if (this->state == State::Paused) {
      this->state = State::Init;
      return this->start();
    }

    debug("Loop::resume(): 'Resuming -> Resumed' failed");
    return false;
  }

  bool Loop::pause () {
    if (this->state == State::Paused) {
      return true;
    } else if (this->state != State::Idle && this->state != State::Polling) {
      return false;
    }

    this->state = State::Paused;
    this->uv.stop();

    Lock lock(this->mutex);
    if (this->options.dedicatedThread) {
      // clean up old thread if still running
      if (this->thread.joinable()) {
        this->thread.join();
      }
    }

    return this->state == State::Paused;
  }

  bool Loop::dispatch (const DispatchCallback& callback) {
    if (callback == nullptr) {
      return false;
    } else if (this->state > State::Polling) {
      return false;
    } else if (this->state < State::Idle && !this->kick()) {
      return false;
    }

    do {
      Lock lock(this->mutex);
      this->queue.push(callback);
    } while (0);

    uv_async_send(&this->uv.async);
    return this->state == State::Idle || this->state == State::Polling;
  }

  bool Loop::shutdown () {
    if (this->state > State::None && this->state < State::Shutdown) {
      if (this->stop()) {
        this->state = State::Shutdown;
        this->uv.close();
        return true;
      }

      debug("Loop::start(): '* -> Shutdown' failed");
    }

    return false;
  }

  uv_loop_t* Loop::get () {
    return &this->uv.loop;
  }

  bool Loop::alive () const {
    if (this->state == State::Shutdown) {
      return false;
    } else if (this->state < State::Idle) {
      return false;
    }

    return uv_loop_alive(&this->uv.loop);
  }

  bool Loop::started () const {
    return this->state > State::Init && this->state < State::Shutdown;
  }

  bool Loop::stopped () const {
    return this->state == State::Stopped;
  }

  bool Loop::paused () const {
    return this->state == State::Paused;
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
