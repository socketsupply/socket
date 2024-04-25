#ifndef SSC_CORE_NETWORK_STATUS_H
#define SSC_CORE_NETWORK_STATUS_H

#include "json.hh"
#include "platform.hh"
#include "types.hh"
#include "module.hh"

namespace SSC {
  // forward
  class Core;
  class NetworkStatus;
}

namespace SSC {
  class NetworkStatus : public Module {
    public:
      using Observer = Module::Observer<JSON::Object>;
      using Observers = Module::Observers<Observer>;

    #if SSC_PLATFORM_APPLE
      dispatch_queue_t queue;
      nw_path_monitor_t monitor;
    #endif

      Observers observers;

      NetworkStatus (Core*);
      ~NetworkStatus ();
      bool start ();
      bool addObserver (const Observer&, const Observer::Callback callback = nullptr);
      bool removeObserver (const Observer&);
  };
}

#endif
