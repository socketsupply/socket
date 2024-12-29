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
        None = -1,
        Idle = 0,
        Stoping = 1,
        Stopped = 2,
        Pausing = 3,
        Paused = 4,
        Resuming = 5,
        Resumed = 6,
        Starting = 7,
        Started = 8,
        Polling = 9,
        Shutdown = 20
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

      bool init ();
      bool shutdown ();
      bool start ();
      bool stop ();
      bool resume ();
      bool pause ();
      bool dispatch (const DispatchCallback&);
      bool run ();

      uv_loop_t* get ();
      bool alive () const;
      bool started () const;
      bool stopped () const;
      bool paused () const;
      int timeout ();
      int timeout () const;
      bool sleep (int64_t ms);
  };
}
#endif
