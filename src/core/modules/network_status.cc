#include "network_status.hh"

namespace SSC {
  CoreNetworkStatus::CoreNetworkStatus (Core* core)
    : CoreModule(core)
  {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    dispatch_queue_attr_t attrs = dispatch_queue_attr_make_with_qos_class(
      DISPATCH_QUEUE_SERIAL,
      QOS_CLASS_UTILITY,
      DISPATCH_QUEUE_PRIORITY_DEFAULT
    );

    this->queue = dispatch_queue_create(
      "socket.runtime.queue.ipc.network-status",
      attrs
    );

  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    this->monitor = g_network_monitor_get_default();
  #endif
  }

  CoreNetworkStatus::~CoreNetworkStatus () {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    this->stop();
    dispatch_release(this->queue);
    this->monitor = nullptr;
    this->queue = nullptr;
  #endif
  }

  bool CoreNetworkStatus::start () {
    this->stop();
  #if SOCKET_RUNTIME_PLATFORM_APPLE
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
    nw_path_monitor_start(this->monitor);
    return true;
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    this->signal = g_signal_connect(
      this->monitor,
      "network-changed",
      G_CALLBACK((+[](GNetworkMonitor* monitor, gboolean networkAvailable, gpointer userData) {
        auto coreNetworkStatus = reinterpret_cast<CoreNetworkStatus*>(userData);
        if (coreNetworkStatus) {
          const auto json = JSON::Object::Entries {
            {"name", networkAvailable ? "online" : "offline"},
            {"message", networkAvailable ? "Network is usable" : "Network is not usable"}
          };

          coreNetworkStatus->observers.dispatch(json);
        }
      })),
      this
    );
    return true;
  #endif
    return false;
  }

  bool CoreNetworkStatus::stop () {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->monitor) {
      nw_path_monitor_cancel(this->monitor);
      nw_release(this->monitor);
    }
    this->monitor = nullptr;
    return true;
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    if (this->signal) {
      g_signal_handler_disconnect(this->monitor, this->signal);
      this->signal = 0;
      return true;
    }
  #endif
    return false;
  }

  bool CoreNetworkStatus::addObserver (const Observer& observer, const Observer::Callback callback) {
    return this->observers.add(observer, callback);
  }

  bool CoreNetworkStatus::removeObserver (const Observer& observer) {
    return this->observers.remove(observer);
  }
}
