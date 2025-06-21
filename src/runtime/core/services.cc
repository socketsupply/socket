#include "services.hh"

namespace ssc::runtime::core {
  Services::Services (
    context::RuntimeContext& context,
    const Options& options
  )
    : ai({ context, options.features.useAI, options.dispatcher, context.loop, *this }),
      conduit({ context, options.features.useConduit, options.dispatcher, context.loop, *this }),
      broadcastChannel({ context, options.features.useBroadcashChannel, options.dispatcher, context.loop, *this }),
      dns({ context, options.features.useDNS, options.dispatcher, context.loop, *this }),
      diagnostics({ context, options.features.useDiagnostics, options.dispatcher, context.loop, *this }),
      fs({ context, options.features.useFS, options.dispatcher, context.loop, *this }),
      geolocation({ context, options.features.useGeolocation, options.dispatcher, context.loop, *this }),
      mediaDevices({ context, options.features.useMediaDevices, options.dispatcher, context.loop, *this }),
      networkStatus({ context, options.features.useNetworkStatus, options.dispatcher, context.loop, *this }),
      notifications({ context, options.features.useNotifications, options.dispatcher, context.loop, *this }),
      os({ context, options.features.useOS, options.dispatcher, context.loop, *this }),
      permissions({ context, options.features.usePermissions, options.dispatcher, context.loop, *this }),
      process({ context, options.features.useProcess, options.dispatcher, context.loop, *this }),
      platform({ context, options.features.usePlatform, options.dispatcher, context.loop, *this }),
      timers({ context, options.features.useTimers, options.dispatcher, context.loop, *this }),
      udp({ context, options.features.useUDP, options.dispatcher, context.loop, *this })
  {}

  bool Services::start () {
    Lock lock(this->mutex);
    const auto services = Vector<Service*> {
      &this->ai,
      &this->conduit,
      &this->broadcastChannel,
      &this->dns,
      &this->diagnostics,
      &this->fs,
      &this->geolocation,
      &this->mediaDevices,
      &this->networkStatus,
      &this->notifications,
      &this->os,
      &this->permissions,
      &this->platform,
      &this->process,
      &this->timers,
      &this->udp
    };

    for (const auto& service : services) {
      if (service->enabled && !service->start()) {
        return false;
      }
    }
    
    debug(">>> ALL SERVICES STARTED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");

    return true;
  }

  bool Services::stop () {
    Lock lock(this->mutex);
    const auto services = Vector<Service*> {
      &this->ai,
      &this->conduit,
      &this->broadcastChannel,
      &this->dns,
      &this->diagnostics,
      &this->fs,
      &this->geolocation,
      &this->mediaDevices,
      &this->networkStatus,
      &this->notifications,
      &this->os,
      &this->permissions,
      &this->platform,
      &this->process,
      &this->timers,
      // &this->udp
    };

    for (const auto& service : services) {
      if (service->enabled && !service->stop()) {
        return false;
      }
    }

    debug(">>> ALL SERVICES STOPPED >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");

    return true;
  }
}
