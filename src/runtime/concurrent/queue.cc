#include "../debug.hh"
#include "../concurrent.hh"

namespace ssc::runtime::concurrent {
  static void runWorkerQueueThread (WorkerQueue* queue) {
    while (true) {
      SharedPointer<WorkerQueue::Entry> entry;
      do {
        UniqueLock lock(queue->mutex);
        queue->condition.wait(lock, [queue]() {
          return queue->destroyed() || !queue->empty();
        });

        // queue is empty and was destroyed, bail early
        if (queue->destroyed() && queue->size() == 0) {
          return;
        }

        const auto head = queue->entries.front();;
        entry = SharedPointer<WorkerQueue::Entry>(
          head,
          reinterpret_cast<WorkerQueue::Entry*>(head.get())
        );
        queue->entries.pop();
      } while (0);

      if (entry == nullptr || entry->work == nullptr) {
        continue;
      }

      queue->semaphore.acquire();

      // dispatch entry work to detached thread
      auto thread = Thread([queue, entry](){
        if (entry->work != nullptr) {
          try {
            entry->work();
          } catch (const Exception& e) {
            debug("WorkerQueue::Thread work handler exception: %s", e.what());
          }
        }

        if (entry->callback != nullptr) {
          try {
            entry->callback();
          } catch (const Exception& e) {
            debug("WorkerQueue::Thread callback handler exception: %s", e.what());
          }
        }

        queue->semaphore.release();
      });

      thread.detach();
    }
  }

  Queue::Queue (const Options& options)
    : limit(options.limit),
      semaphore(options.limit)
  {}

  Queue::~Queue () {
    this->destroy();
  }

  bool Queue::destroyed () const {
    return this->isDestroyed.load(std::memory_order_relaxed);
  }

  void Queue::destroy () {
    Lock lock(this->mutex);
    this->isDestroyed = true;
    this->condition.notify_all();
  }

  size_t Queue::size () const {
    Lock lock(this->mutex);
    return this->entries.size();
  }

  bool Queue::empty () const {
    return this->size () == 0;
  }

  size_t Queue::push (const Entry& entry) {
    Lock lock(this->mutex);
    this->entries.push(std::make_shared<Entry>(entry));
    return this->entries.size();
  }

  WorkerQueue::WorkerQueue (const Options& options)
    : Queue(options)
  {
    this->thread = Thread(&runWorkerQueueThread, this);
  }

  WorkerQueue::~WorkerQueue () {
    this->destroy();
  }

  size_t WorkerQueue::push (const Entry& entry) {
    do {
      UniqueLock lock(this->mutex);
      this->entries.push(std::make_shared<Entry>(entry));
    } while (0);
    this->condition.notify_one();
    return this->size();
  }

  size_t WorkerQueue::push (const WorkHandler& work, const WorkCallback& callback) {
    return this->push(Entry { crypto::rand64(), work, callback });
  }

  void WorkerQueue::destroy () {
    Queue::destroy();
    if (this->thread.joinable()) {
      this->thread.join();
    }
  }
}
