#ifndef SOCKET_RUNTIME_CONCURRENT_H
#define SOCKET_RUNTIME_CONCURRENT_H

#include "crypto.hh"

namespace ssc::runtime::concurrent {
  class Queue {
    public:
      using ID = uint64_t;

      struct Entry {
        ID id = crypto::rand64();
      };

      struct Options {
        size_t limit = 1;
      };

      types::Queue<SharedPointer<Entry>> entries;
      ConditionVariableAny condition;
      Semaphore<8> semaphore;
      Atomic<bool> isDestroyed = false;
      Atomic<size_t> limit = 0;
      mutable Mutex mutex;

      Queue (const Options&);
      virtual ~Queue ();

      Queue (const Queue&) = delete;
      Queue (Queue&&) = delete;
      Queue& operator = (const Queue&) = delete;
      Queue& operator = (Queue&&) = delete;

      virtual bool destroyed () const;
      virtual void destroy ();
      virtual size_t push (const Entry&);
      virtual size_t size () const;
      virtual bool empty () const;
  };

  class WorkerQueue : public Queue {
    public:
      using Options = Queue::Options;
      using WorkHandler = Function<void()>;
      using WorkCallback = Function<void()>;

      struct Entry : public Queue::Entry {
        WorkHandler work = nullptr;
        WorkCallback callback = nullptr;
      };

      Thread thread;

      WorkerQueue (const Options&);
      ~WorkerQueue () override;

      WorkerQueue (const WorkerQueue&) = delete;
      WorkerQueue (WorkerQueue&&) = delete;
      WorkerQueue& operator = (const WorkerQueue&) = delete;
      WorkerQueue& operator = (WorkerQueue&&) = delete;

      size_t push (const Entry&);
      size_t push (const WorkHandler&, const WorkCallback& = nullptr);
      void destroy () override;
  };

  class AbortController;
  class AbortSignal {
    public:
      const AbortController* controller = nullptr;
      AbortSignal () = default;
      AbortSignal (const AbortController*);
      AbortSignal (const AbortSignal&);
      AbortSignal (AbortSignal&&);
      AbortSignal& operator = (const AbortSignal&);
      AbortSignal& operator = (AbortSignal&&);
      bool aborted () const;
  };

  class AbortController {
    public:
      AbortSignal signal;
      Atomic<bool> isAborted = false;
      AbortController (const AbortController&) = delete;
      AbortController (AbortController&&) = delete;
      AbortController& operator = (const AbortController&) = delete;
      AbortController& operator = (AbortController&&) = delete;
      void abort ();
  };
}
#endif
