#ifndef SOCKET_RUNTIME_CORE_MODULE_H
#define SOCKET_RUNTIME_CORE_MODULE_H

#include "../platform/platform.hh"
#include "json.hh"
#include "post.hh"

namespace SSC {
  // forward
  class Core;
  class CoreModule {
    public:
      using Callback = Function<void(const String, JSON::Any, Post)>;

      struct RequestContext {
        String seq;
        CoreModule::Callback callback;
        RequestContext () = default;
        RequestContext (String seq, const CoreModule::Callback& callback) {
          this->seq = seq;
          this->callback = callback;
        }
      };

      template <typename... Types>
      class Observer {
        public:
          using Callback = Function<void(Types...)>;
          uint64_t id = 0;
          Callback callback;

          Observer () {
            this->id = rand64();
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

          Observer (const Callback& callback)
            : callback(callback)
          {
            this->id = rand64();
          }

          Observer (uint64_t id, const Callback& callback)
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
              }

              if (iterator->id == observer.id) {
                iterator = this->observers.erase(iterator);
                return true;
              }

            } while (
              iterator != this->observers.end() &&
              ++iterator != this->observers.end()
            );

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
              }

              iterator++;
            }

            return dispatched;
          }
      };

      Core *core = nullptr;
      CoreModule (Core* core)
        : core(core)
      {}
  };
}
#endif
