#ifndef SOCKET_RUNTIME_LOOP_H
#define SOCKET_RUNTIME_LOOP_H

#include "platform.hh"
#include "options.hh"

namespace ssc::runtime::loop {
  class Loop {
    public:
      using DispatchCallback = Function<void()>;

      /**
       * A container for libuv primatives.
       */
      struct UV {
        using RunMode = uv_run_mode;

        uv_loop_t loop;
        uv_async_t async;
        Atomic<bool> isInitialized = false;

        void open (Loop*);
        void close ();
        void stop ();
        bool run (uv_run_mode = UV_RUN_DEFAULT);
        int timeout () const;
        int timeout ();
      };

      /**
       * Various states the event loop will be in.
       * even numbers are "final" states, odds are "pending"
       */
      enum class State {
        None = -1, // initial state, uninitialized, _should_ only transition out of this state
        Init = 0, // loop is initialized, waiting to transition into new state
        Idle = 1, // loop is idle, waiting for dispatch, this is a frequent state
        Polling = 2, // loop is polling (dequeue) or currently executing (uv_run)
        Paused = 3, // loop is paused - the loop is doing nothing
        Stopped = 4, // loop is stopped and drained
        Shutdown = 5 // loop is shutdown, it cannot be restored
      };

      /**
       * Various options to configure the event loop
       */
      struct Options : public ssc::runtime::Options {
      #if SOCKET_RUNTIME_PLATFORM_ANDROID || SOCKET_RUNTIME_PLATFORM_WINDOWS || SOCKET_RUNTIME_DESKTOP_EXTENSION
        bool dedicatedThread = true;
      #else
        bool dedicatedThread = false;
      #endif
      };

    #if SOCKET_RUNTIME_PLATFORM_LINUX
      struct GTK {
        GSource* source = nullptr;
        GSourceFuncs functions;
      };

      GTK gtk;
    #endif

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      dispatch_queue_t dispatchQueue = dispatch_queue_create(
        "socket.runtime.core.loop.queue",
        dispatch_queue_attr_make_with_qos_class(
          DISPATCH_QUEUE_SERIAL,
          QOS_CLASS_DEFAULT,
          -1
        )
      );
    #endif

      const Options options;
      Queue<DispatchCallback> queue;
      Thread thread;
      Mutex mutex;
      Atomic<State> state = State::None;
      UV uv;

      Loop () = default;
      Loop (const Options&);
      Loop (const Loop&) = delete;
      Loop (Loop&&) = delete;

      Loop& operator = (const Loop&) = delete;
      Loop& operator = (Loop&&) = delete;

      /**
       * Initializes loop if in a `None` state, transitioning it to a `Init`
       * state. This function returns `true` such that
       * `state >= Init && state < Shutdown` holds true.
       */
      bool init ();
      /**
       * "Kicks" the loop to ensure it is active and running. Depending on the
       * platform, the loop will be active on a dedicated thread, dispatch
       * queue, or activated through a gsource function. This function will
       * return `true` such that all state transitions are successful.
       */
      bool kick ();

      /**
       * Start the loop if it isn't. This function will call `init()` and
       * `kick()` such that all state transitions are successful.
       */
      bool start ();

      /**
       * Stops the loop if it is active. This function will transition to a
       * `Stopped` state and returns `true` if all state transitions are
       * successful.
       */
      bool stop ();

      /**
       * Resumes the loop if it was paused. This function will also start the
       * loop if it hasn't been started yet. This function will result in a
       * `Idle` or `Polling` state for the upon success and returns `true`
       * if all state transitions are successful.
       */
      bool resume ();

      /**
       * Pauses the loop if it is `Idle` or `Polling`. This function will return
       * `false` if the loop is not in an "active" state such that
       * `state > State::Init && state < State::Paused`.
       */
      bool pause ();

      /**
       * Dispatches a callback to be called asynchronously on the loop thread.
       * This function will attempt to start the loop if it hasn't been started
       * such that it is not already in a `Paused` or `Shutdown` state.
       * This function returns `true` if the loop is in an "active" state
       * such that `state > State::Init && state < State::Paused`.
       */
      bool dispatch (const DispatchCallback&);

      /**
       * Shuts down the loop, transitioning it into a state that cannot be
       * transitioned out of.
       */
      bool shutdown ();

      /**
       * Get a pointer to the underlying `uv_loop_t`.
       */
      uv_loop_t* get ();

      /**
       * Returns `true` if the loop is "alive" (active handles and running)
       */
      bool alive () const;

      /**
       * Returns `true` if the loop was "started" and its thread is active,
       * if applicable.
       */
      bool started () const;

      /**
       * Returns `true` if the loop was "stopped" (from a call to `stop()`)
       */
      bool stopped () const;

      /**
       * Returns `true` if the loop was "paused" (from a call to `pause()`)
       */
      bool paused () const;

      /**
       * Get the current loop backend timeout in milliseconds
       */
      int timeout ();
      int timeout () const;

      /**
       * Dispatch to the loop thread and cause it to sleep for `ms` milliseconds
       */
      bool sleep (int64_t ms);
  };
}
#endif
