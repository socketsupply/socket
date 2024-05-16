#ifndef SSC_CORE_MODULE_H
#define SSC_CORE_MODULE_H

#include "platform.hh"
#include "types.hh"
#include "json.hh"
#include "post.hh"

namespace SSC {
  uint64_t rand64 ();
  // forward
  class Core;
  class Module {
    public:
      using Callback = Function<void(String, JSON::Any, Post)>;

      struct RequestContext {
        String seq;
        Module::Callback cb;
        RequestContext () = default;
        RequestContext (String seq, Module::Callback cb) {
          this->seq = seq;
          this->cb = cb;
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

          Observer (const Callback& callback)
            : callback(callback)
          {
            this->id = rand64();
          }

          Observer (uint64_t id, const Callback& callback)
            : id(id),
              callback(callback)
        {}
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
            int cursor = -1;
            while (cursor++ < this->observers.size()) {
              if (this->observers.at(cursor).id == observer.id) {
                break;
              }
            }

            if (cursor >= 0 && cursor < this->observers.size()) {
              this->observers.erase(this->observers.begin() + cursor);
              return true;
            }

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
              bool dispatched = false;
              for (auto& observer : this->observers) {
                if (observer.callback != nullptr) {
                  observer.callback(arguments...);
                  dispatched = true;
                }
              }
              return dispatched;
            }
      };

      Core *core = nullptr;
      Module (Core* core);
  };
}
#endif
