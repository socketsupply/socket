#ifndef SOCKET_RUNTIME_CORE_SERVICES_NETWORK_STATUS_H
#define SOCKET_RUNTIME_CORE_SERVICES_NETWORK_STATUS_H

#include "../../core.hh"

namespace ssc::runtime::core::services {
  class NetworkStatus : public core::Service {
    public:
      using Observer = Observer<JSON::Object>;
      using Observers = Observers<Observer>;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      dispatch_queue_t queue = nullptr;
      nw_path_monitor_t monitor = nullptr;
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      GNetworkMonitor* monitor = nullptr;
      guint signal = 0;
    #endif

      Observers observers;

      NetworkStatus (const Options&);
      ~NetworkStatus ();

      bool start () override;
      bool stop () override;
      bool addObserver (
        const Observer& observer,
        const Observer::Callback callback = nullptr
      );

      bool removeObserver (const Observer& observer);
  };
}

#endif
