#ifndef SOCKET_RUNTIME_CORE_H
#define SOCKET_RUNTIME_CORE_H

#include "queued_response.hh"
#include "concurrent.hh"
#include "context.hh"
#include "json.hh"
#include "loop.hh"

namespace ssc::runtime::core {
  class Services;

  struct DispatchContext : public context::DispatchContext {
    struct StateFlags {
      Atomic<bool> ready = false;
    };

    StateFlags flags;
  };

  class Service {
    public:
      using Callback = Function<void(const String, JSON::Any, QueuedResponse)>;

      struct RequestContext {
        String seq;
        Callback callback;
      };

      template <typename... Types>
      class Observer {
        public:
          using Callback = Function<void(Types...)>;
          uint64_t id = 0;
          Callback callback;

          Observer () {
            this->id = crypto::rand64();
          }

          Observer (const Observer& observer) {
            this->id = observer.id;
            this->callback = observer.callback;
          }

          Observer (Observer&& observer) {
            this->id = observer.id;
            this->callback = observer.callback;
            observer.id = 0;
            observer.callback = nullptr;
          }

          Observer (const Callback callback)
            : callback(callback)
          {
            this->id = crypto::rand64();
          }

          Observer (uint64_t id, const Callback callback)
            : id(id),
              callback(callback)
          {}

          Observer& operator = (const Observer& observer) {
            this->id = observer.id;
            this->callback = observer.callback;
          }

          Observer& operator = (Observer&& observer) {
            this->id = observer.id;
            this->callback = observer.callback;
            observer.id = 0;
            observer.callback = nullptr;
            return *this;
          }

          JSON::Object json () const {
            return JSON::Object::Entries {
              {"id", this->id}
            };
          }
      };

      template <class Observer>
      class Observers {
        public:
          Vector<Observer> observers;
          Mutex mutex;

          bool add (const Observer& observer, const typename Observer::Callback callback = nullptr) {
            Lock lock(this->mutex);
            if (this->has(observer)) {
              auto& existing = this->get(observer.id);
              existing.callback = callback;
              return true;
            } else if (callback != nullptr) {
              this->observers.push_back({ observer.id, callback });
              return true;
            } else if (observer.callback != nullptr) {
              this->observers.push_back(observer);
              return true;
            }

            return false;
          }

          bool remove (const Observer& observer) {
            Lock lock(this->mutex);

            if (this->observers.begin() == this->observers.end()) {
              return false;
            }

            auto iterator = this->observers.begin();

            do {
              if (iterator->id == 0) {
                iterator = this->observers.erase(iterator);
              } else {
                iterator++;
              }

              if (iterator->id == observer.id) {
                iterator = this->observers.erase(iterator);
                return true;
              }
            } while (iterator != this->observers.end());

            return false;
          }

          bool has (const Observer& observer) {
            Lock lock(this->mutex);
            for (const auto& existing : this->observers) {
              if (existing.id == observer.id) {
                return true;
              }
            }

            return false;
          }

          Observer& get (const uint64_t id) {
            Lock lock(this->mutex);
            for (auto& existing : this->observers) {
              if (existing.id == id) {
                return existing;
              }
            }

            throw std::out_of_range("Observer for ID does not exist");
          }

          template <typename... Types>
          bool dispatch (Types... arguments) {
            Lock lock(this->mutex);

            if (this->observers.begin() == this->observers.end()) {
              return false;
            }

            bool dispatched = false;
            auto iterator = this->observers.begin();

            while (iterator != this->observers.end()) {
              if (iterator->id == 0) {
                iterator = this->observers.erase(iterator);
              } else if (iterator->callback != nullptr) {
                iterator->callback(arguments...);
                dispatched = true;
                iterator++;
              }
            }

            return dispatched;
          }

          JSON::Array json () const {
            return this->observers;
          }
      };

      struct Options {
        context::RuntimeContext& context;
        bool enabled = false;
        context::Dispatcher& dispatcher;
        loop::Loop& loop;
        Services& services;
        concurrent::WorkerQueue::Options workerQueue;
      };

      concurrent::WorkerQueue queue;
      context::RuntimeContext& context;
      context::Dispatcher& dispatcher;
      loop::Loop& loop;

      // ref back to `Services` container
      Services& services;

      // state
      Atomic<bool> enabled = false;

      Service (const Options&);
      Service () = delete;
      Service (const Service&) = delete;
      Service (Service&&) = delete;
      virtual ~Service () noexcept;
      Service& operator = (const Service&) = delete;
      Service& operator = (Service&&) = delete;
      virtual bool start ();
      virtual bool stop ();
      bool dispatch (const context::Dispatcher::Callback);
      context::RuntimeContext* getRuntimeContext ();
  };
}
#endif
