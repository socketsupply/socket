#ifndef SOCKET_RUNTIME_CORE_MODULE_NETWORK_STATUS_H
#define SOCKET_RUNTIME_CORE_MODULE_NETWORK_STATUS_H

#include "../json.hh"
#include "../module.hh"

namespace SSC {
  class CoreNetworkStatus : public CoreModule {
    public:
      using Observer = CoreModule::Observer<JSON::Object>;
      using Observers = CoreModule::Observers<Observer>;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      dispatch_queue_t queue = nullptr;
      nw_path_monitor_t monitor = nullptr;
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      GNetworkMonitor* monitor = nullptr;
      guint signal = 0;
    #endif

      Observers observers;

      CoreNetworkStatus (Core*);
      ~CoreNetworkStatus ();

      bool start ();
      bool stop ();
      bool addObserver (
        const Observer& observer,
        const Observer::Callback callback = nullptr
      );

      bool removeObserver (const Observer& observer);
  };
}

#endif
