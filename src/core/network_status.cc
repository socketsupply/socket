#include "network_status.hh"
#include "platform.hh"
#include "core.hh"

namespace SSC {
  NetworkStatus::NetworkStatus (Core* core) : Module(core) {
  #if SSC_PLATFORM_APPLE
    dispatch_queue_attr_t attrs = dispatch_queue_attr_make_with_qos_class(
      DISPATCH_QUEUE_SERIAL,
      QOS_CLASS_UTILITY,
      DISPATCH_QUEUE_PRIORITY_DEFAULT
    );

    this->queue = dispatch_queue_create(
      "socket.runtime.queue.ipc.network-status",
      attrs
    );

    this->monitor = nw_path_monitor_create();

    nw_path_monitor_set_queue(this->monitor, this->queue);
    nw_path_monitor_set_update_handler(this->monitor, ^(nw_path_t path) {
      if (path == nullptr) {
        return;
      }

      nw_path_status_t status = nw_path_get_status(path);

      String name;
      String message;

      switch (status) {
        case nw_path_status_invalid: {
          name = "offline";
          message = "Network path is invalid";
          break;
        }
        case nw_path_status_satisfied: {
          name = "online";
          message = "Network is usable";
          break;
        }
        case nw_path_status_satisfiable: {
          name = "online";
          message = "Network may be usable";
          break;
        }
        case nw_path_status_unsatisfied: {
          name = "offline";
          message = "Network is not usable";
          break;
        }
      }

      const auto json = JSON::Object::Entries {
        {"name", name},
        {"message", message}
      };

      this->observers.dispatch(json);
    });
  #endif
  }

  NetworkStatus::~NetworkStatus () {
  #if SSC_PLATFORM_APPLE
    dispatch_release(this->queue);
    nw_release(this->monitor);
    this->monitor = nullptr;
    this->queue = nullptr;
  #endif
  }

  bool NetworkStatus::start () {
  #if SSC_PLATFORM_APPLE
    nw_path_monitor_start(this->monitor);
    return true;
  #endif
    return false;
  }

  bool NetworkStatus::addObserver (const Observer& observer, const Observer::Callback callback) {
    return this->observers.add(observer, callback);
  }

  bool NetworkStatus::removeObserver (const Observer& observer) {
    return this->observers.remove(observer);
  }
}
